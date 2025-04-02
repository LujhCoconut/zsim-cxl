#include "cxl_switch.h"
#include <iostream>
#include "config.h"  
#include "contention_sim.h"
#include "event_recorder.h"
#include "timing_event.h"
#include "zsim.h"

/**
 * @brief: switch the detailed port to forward current req
 */
int CXLSwitch::port_select(MemReq& req)
{
    Address port_address = req.lineAddr;
    assert(port_address >= this_switch_range.first);

    int port_idx = -1;
    for(int i = 0; i < switch_ports_ranges.size(); i++)
    {
        if(port_address >= switch_ports_ranges[i].first)
        {
            port_idx = i;
            break;
        }
    }
    assert(-1 != port_idx);
    return port_idx;
}

/**
 * @brief: forward this request to detailed cxl memory controller (bound phase interface)
 */
uint64_t CXLSwitch::forward_to_detailed_cxl_memory(MemReq& req)
{
    int forward_port = port_select(req);
    // detailed access is handled by ddr_mem
    req.cycle += (network_on_chip_latency + 2*end_port_processing_latency);
    req.cycle = cxl_memory_ctrls[forward_port]._cxl_memory->access(req); // ddr->access(req) [banshee ddr->access(req,type,data_size)]
    return req.cycle;
}