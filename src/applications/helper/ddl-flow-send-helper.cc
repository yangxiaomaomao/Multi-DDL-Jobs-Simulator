#include "ddl-flow-send-helper.h"

#include <ns3/address-utils.h>
#include <ns3/string.h>
#include <ns3/uinteger.h>

namespace ns3
{
DdlFlowSendHelper::DdlFlowSendHelper(const Address& ip, uint16_t port)
    : ApplicationHelper("ns3::DdlFlowSendApplication")
{
    m_factory.Set("ddlRemote", AddressValue(ip));
    m_factory.Set("ddlPort", UintegerValue(port));
}

DdlFlowSendHelper::DdlFlowSendHelper(uint16_t port)
    : ApplicationHelper("ns3::DdlFlowSendApplication")
{
    m_factory.Set("ddlPort", UintegerValue(port));
    // SetAttribute("ddlPort", UintegerValue(port));
}
} // namespace ns3