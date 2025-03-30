#ifndef DDL_FLOW_SEND_HELPER_H
#define DDL_FLOW_SEND_HELPER_H
#include <ns3/application-helper.h>
#include <ns3/udp-client.h>
#include <ns3/udp-server.h>
#include <ns3/udp-trace-client.h>

#include <stdint.h>

namespace ns3
{

class DdlFlowSendHelper: public ApplicationHelper
{
  public:
  DdlFlowSendHelper(const Address& ip, uint16_t port);
  DdlFlowSendHelper(uint16_t port);

};

} // namespace ns3

#endif // DDL_WORKLOAD_HELPER_H