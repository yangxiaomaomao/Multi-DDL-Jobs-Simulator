#ifndef DDL_FLOW_SEND_H
#define DDL_FLOW_SEND_H
#include "seq-ts-size-header.h"
#include "source-application.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/event-id.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <iostream>
using namespace std;

namespace ns3
{
class DdlApplication;
class DdlFlowSendApplication : public SourceApplication
{
  public:
    static TypeId GetTypeId();

    DdlFlowSendApplication();
    ~DdlFlowSendApplication() override;
    void PrintInfo();
    void setUpstreamFinishStatesAllFalse();
    void setUpstreamFinishStatesAllTrue();
    void setUpstreamFinishState(uint32_t flowId, bool state);
    void setUpstreamFlowIds(std::vector<uint32_t> upstreamFlowIds);

    void ddlStop()
    {
        StopApplication();
    }

    void setParentDdlApp(DdlApplication* parentDdlApp)
    {
        m_parentDdlApp = parentDdlApp;
    }

  private:
    void StartApplication() override;
    void StopApplication() override;

    // send packets
    void SendPacket();
    // ensure the receiver has recv the packets
    void HandleAck(Ptr<Socket> socket);
    // send the fragment packets to avoid the overflow of somewhere
    void SendFragmentPacket();
    void detailedLog(std::string info);

    // connection related params
    Ptr<Socket> m_socket;

    Address m_ddlRemote;
    uint16_t m_ddlPort;
    Address m_peer;

    Ptr<Socket> m_ackSocket;
    Address m_ackAddr;

    bool m_waitingAck;

    TypeId m_tid;

    // ddl related param
    uint16_t m_jobid;
    uint16_t m_flowid;
    uint16_t m_nodeid;

    uint16_t m_prio;
    uint32_t m_iternum;
    Time m_comptime;
    uint32_t m_commsize;
    uint32_t m_send_cnt;

    std::vector<uint32_t> m_upstreamFlowIds;
    std::map<uint32_t, bool> m_upstreamFinishState;

    // to compute the sender node's GPU utilization
    uint32_t m_startTime;

    DdlApplication* m_parentDdlApp;
};

} // namespace ns3

#endif