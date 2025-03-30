#include "ddl-flow-recv-helper.h"

#include <ns3/address-utils.h>
#include <ns3/string.h>
#include <ns3/uinteger.h>

namespace ns3
{
DdlFlowRecvHelper::DdlFlowRecvHelper(uint16_t port)
    : ApplicationHelper("ns3::DdlFlowRecvApplication")
{
    m_factory.Set("ddlPort", UintegerValue(port));
}
} // namespace ns3