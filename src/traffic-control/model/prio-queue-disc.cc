/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "prio-queue-disc.h"

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"

#include <algorithm>
#include <iterator>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PrioQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(PrioQueueDisc);

ATTRIBUTE_HELPER_CPP(Priomap);

std::ostream&
operator<<(std::ostream& os, const Priomap& priomap)
{
    std::copy(priomap.begin(), priomap.end() - 1, std::ostream_iterator<uint16_t>(os, " "));
    os << priomap.back();
    return os;
}

std::istream&
operator>>(std::istream& is, Priomap& priomap)
{
    for (int i = 0; i < 16; i++)
    {
        if (!(is >> priomap[i]))
        {
            NS_FATAL_ERROR("Incomplete priomap specification ("
                           << i << " values provided, 16 required)");
        }
    }
    return is;
}

TypeId
PrioQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PrioQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<PrioQueueDisc>()
            .AddAttribute("Priomap",
                          "The priority to band mapping.",
                          PriomapValue(Priomap{{1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}}),
                          MakePriomapAccessor(&PrioQueueDisc::m_prio2band),
                          MakePriomapChecker());
    return tid;
}

PrioQueueDisc::PrioQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::NO_LIMITS)
{
    NS_LOG_FUNCTION(this);
}

PrioQueueDisc::~PrioQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
PrioQueueDisc::SetBandForPriority(uint8_t prio, uint16_t band)
{
    NS_LOG_FUNCTION(this << prio << band);

    NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");

    m_prio2band[prio] = band;
}

uint16_t
PrioQueueDisc::GetBandForPriority(uint8_t prio) const
{
    NS_LOG_FUNCTION(this << prio);

    NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");

    return m_prio2band[prio];
}

bool
PrioQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    uint32_t band = m_prio2band[0];

    int32_t ret = Classify(item);

    if (ret == PacketFilter::PF_NO_MATCH)
    {
        NS_LOG_DEBUG("No filter has been able to classify this packet, using priomap.");

        SocketPriorityTag priorityTag;
        if (item->GetPacket()->PeekPacketTag(priorityTag))
        {
            band = m_prio2band[priorityTag.GetPriority() & 0x0f];
            // std::cout << "m_priobandList: " << m_prio2band << std::endl; 
            // priorityTag.Print(std::cout);
            // std::cout << "Priority Band: " << band << ", Tag Band: " << (priorityTag.GetPriority() & 0x0f) << std::endl;
        }
    }
    else
    {
        NS_LOG_DEBUG("Packet filters returned " << ret);

        if (ret >= 0 && static_cast<uint32_t>(ret) < GetNQueueDiscClasses())
        {
            band = ret;
        }
    }

    NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
    // because QueueDisc::AddQueueDiscClass sets the drop callback

    NS_LOG_LOGIC("Number packets band " << band << ": "
                                        << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets());

    return retval;
}

Ptr<QueueDiscItem>
PrioQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Dequeue()))
        {
            NS_LOG_LOGIC("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
PrioQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Peek()))
        {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
PrioQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("PrioQueueDisc cannot have internal queues");
        return false;
    }

    if (GetNQueueDiscClasses() == 0)
    {
        // create 3 fifo queue discs
        ObjectFactory factory;
        factory.SetTypeId("ns3::FifoQueueDisc");
        for (uint8_t i = 0; i < 2; i++)
        {
            Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
            qd->Initialize();
            Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
            c->SetQueueDisc(qd);
            AddQueueDiscClass(c);
        }
    }

    if (GetNQueueDiscClasses() < 2)
    {
        NS_LOG_ERROR("PrioQueueDisc needs at least 2 classes");
        return false;
    }

    return true;
}

void
PrioQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
