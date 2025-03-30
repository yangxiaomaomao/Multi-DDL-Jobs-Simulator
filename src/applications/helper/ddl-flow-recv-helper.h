#ifndef DDL_FLOW_RECV_HELPER_H
#define DDL_FLOW_RECV_HELPER_H
#include <ns3/application-helper.h>
#include <ns3/udp-client.h>
#include <ns3/udp-server.h>
#include <ns3/udp-trace-client.h>

#include <stdint.h>

namespace ns3
{
class DdlFlowRecvHelper : public ApplicationHelper
{
  public:
    DdlFlowRecvHelper(uint16_t port);
};
} // namespace ns3

#endif // DDL_RECV_HELPER_H