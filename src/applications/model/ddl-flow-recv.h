#ifndef DDL_FLOW_RECV_H
#define DDL_FLOW_RECV_H

#include "seq-ts-size-header.h"
#include "source-application.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/event-id.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ddl-app.h"

namespace ns3
{
class Socket;
class DdlApplication;

class DdlFlowRecvApplication : public SinkApplication
{
  public:
    static TypeId GetTypeId();
    DdlFlowRecvApplication();
    ~DdlFlowRecvApplication() override;
    void setParentDdlApp(DdlApplication *parentDdlApp);
    void ddlStop()
    {
        StopApplication();
    }

  private:
    void StartApplication() override;
    void StopApplication() override;
    void HandleRead(Ptr<Socket> socket);
    void detailedLog(std::string info);
    Ptr<Socket> m_socket;
    Address m_local;
    uint16_t m_ddlPort;
    
    Ptr<Socket> m_ackSocket;
    Address m_ackAddr;

    uint32_t m_receivedBytes;
    uint32_t m_expectedBytes;

    uint16_t m_jobid;
    uint16_t m_flowid;
    uint16_t m_nodeid;
    TypeId m_tid;

    uint32_t m_iterCnt;
    uint32_t m_iterNum;
    bool m_isLastFlow;

    DdlApplication *m_parentDdlApp;
};
} // namespace ns3

#endif // DDL_RECV_H