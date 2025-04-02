#ifndef CXL_SWITCH_H
#define CXL_SWITCH_H

#include "memory_hierarchy.h"
#include "stats.h"
#include "cxl_mc.h"

class CXLSwitch : public MemObject{
    private:
        int valid_ports_active_in_this_switch;
        g_vector<std::pair<Address,Address>> switch_ports_ranges;// Each port's memory range (from low-index to high-index)
        std::pair<Address,Address> this_switch_range;
        uint64_t network_on_chip_latency;
        uint64_t end_port_processing_latency;
        g_vector<CXLMemoryController> cxl_memory_ctrls;


    public:
        int port_select(MemReq& req);
        uint64_t forward_to_detailed_cxl_memory(MemReq& req);
};

#endif