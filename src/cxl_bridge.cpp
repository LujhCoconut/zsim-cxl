#include "cxl_bridge.h"
#include <iostream>
#include "config.h"  
#include "contention_sim.h"
#include "event_recorder.h"
#include "timing_event.h"
#include "zsim.h"

#define DEBUG(args...)

/** 
 * @author Jiahao Lu
 * Architecture is as following: 
 * **********************************************************************************************************
 *          CPU
 *           |    
 *           | 
 *  Top-Level Controller 
 *           | 
 *     ------ -------                           MC0  MC1  MC2  MC3              MC0  MC1  MC2  MC3
 *     |      |     |                           |    |    |    |                |    |    |    |
 *  DRAMCtrl  |     |              CXLBridge    ----------------    CXLBridge    ---------------
 *            |  PCIe [CXL Port] --------------|  CXL-Switch-0 | ---------------|  CXL-Switch-1 |----------...
 *            |                   Bus & Retimer ----------------  Bus & Retimer  ---------------
 *           Disk                               |    |    |    |                |    |    |    | 
 *                                              MC4  MC5  MC6  MC7              MC4  MC5  MC6  MC7
 * 
 * ***********************************************************************************************************
 * Req -> CXLPort & Bridge (Analyze which type the request is, select specific bus to transfer, caculate latency)
 * 
 * /


/**
 * @brief analyze CXL request type
 */
int CXLBridge::protocal_analyze_bound_phase(MemReq& req, int data_size)
{
    Address cxl_address = req.lineAddr;
    
    if(!check_cxl_range(cxl_address)) 
    {
        DEBUG("Not in cxl range!\n");
        return CXLReq::InvalidRangeReq;
    }

    switch (req.type) {
        case PUTS:
        case PUTX:
            *req.state = I;
            break;
        case GETS:
            *req.state = req.is(MemReq::NOEXCL) ? S : E;
            break;
        case GETX:
            *req.state = M;
            break;
        default: panic("!?");
    }

    // protocal and bus select
    ReqType type = (req.type == GETS || req.type == GETX) ? LOAD : STORE;
    int cxl_type = (type == LOAD) ? CXLReq::M2SReq : CXLReq:: M2SRwD;
    return cxl_type;
}

/**
 * @brief according to switches_ranges to decide specific cxl_switch
 */
int CXLBridge::switch_select(MemReq& req)
{
    Address cxl_address = req.lineAddr;
    int cxl_switch_idx = -1;
    for(int i = 0; i<switches_ranges.size(); i++)
    {
        if(cxl_address >= switches_ranges[i].first)
        {
            cxl_switch_idx = i;
            break;
        }
    }
    assert(-1 != cxl_switch_idx);
    return cxl_switch_idx;
}

/**
 * @brief: Accumulate data transfer (and retimer) latency based on the location of the CXL switch.
 */
uint64_t CXLBridge::transfer_delay_in_PCIe(int switches)
{
    return (switches + 1) * retimer_latency; // + switchToBusCycle
}
 

bool CXLBridge::check_cxl_range(Address address)
{
    return (address < max_address_range && address >= min_address_range)? true : false; 
}

/**
 * @brief: forward req to specific cxl_memory port
 */
uint64_t CXLBridge::forward_to_detaild_port(MemReq& req, int data_size)
{
    int cxl_req_type = protocal_analyze_bound_phase(req, data_size);
    if(cxl_req_type == CXLReq::InvalidRangeReq) return req.cycle + check_range_latency;

    req.cycle += check_range_latency;

    int cxl_switch = switch_select(req);
    int forward_in_bus_latency = transfer_delay_in_PCIe(cxl_switch);
    req.cycle += forward_in_bus_latency;

    req.cycle = cxl_switches[cxl_switch].forward_to_detailed_cxl_memory(req);
    return req.cycle;
}

/**
 * @brief access <=> forward requests
 */
uint64_t CXLBridge::access(MemReq& req)
{
    int data_size = 4;
    return forward_to_detaild_port(req,data_size);
}




