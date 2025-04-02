#ifndef CXL_BRIDGE_H
#define CXL_BRIDGE_H

#include "memory_hierarchy.h"
#include "stats.h"
#include "cxl_switch.h"

enum CXLReq{
    M2SReq, // CXL Request Channel Request (read)
    M2SRwD, // CXL Data Channel Request (write)
    BIReq, // CXL Back-Invalidation Request 
    InvalidRangeReq, 
};

enum CXLResp{
    S2MDRS, // Data Response (read resp)
    S2MNDR, // No Data Response (write resp)
    BISnp, // Back-Invalidtion Snoop
    InvalidRangeResp
};

enum ReqType
{
	LOAD = 0,
	STORE
};

class CXLBridge : public MemObject{
    private:
        uint64_t check_range_latency; 
        uint64_t retimer_latency; // Pond (ASPLOS'23) 30ns
        int valid_switches;
        int valid_ports_per_switch;
        uint64_t PCIeFreqKHz; // PCIe3.0 [4GHz] ; PCIe4.0 [8GHz] ; PCIe5.0 [16GHz] ; PCIe6.0 [32GHz] ; [same as CXL]
        Address max_address_range;
        Address min_address_range; // Exact DRAM MAX RANGE
        g_vector<std::pair<Address,Address>> switches_ranges; // Each CXL-Switch's Memory Range (from low-index to high index)
        g_vector<CXLSwitch> cxl_switches; // CXLSwitch Objects Collection
        // do we really need queue here ??


    public:
        int protocal_analyze_bound_phase(MemReq& req, int data_size);
        int switch_select(MemReq& req);
        uint64_t transfer_delay_in_PCIe(int switches);
        uint64_t forward_to_detaild_port(MemReq& req, int data_size); // depend on `switch_select` & `protocal_analyze_bound_phase`
        Address get_max_range(){return max_address_range;};
        bool check_cxl_range(Address addr);
        int return_max_ports(){return valid_ports_per_switch;};
};

#endif
