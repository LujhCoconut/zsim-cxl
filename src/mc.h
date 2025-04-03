#ifndef MC_H
#define MC_H

#include "memory_hierarchy.h"
#include "stats.h"
#include "cxl_bridge.h"
#include "config.h"

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
 * 
 */

enum ReqType
{
    LOAD = 0,
    STORE
};

enum CacheScheme
{

};

enum CXLPromotingPolicy
{

};

class Way
{
    public:
    Address tag;
    bool valid;
    bool dirty;
};

class Set
{
   public:
    Way * ways;
    uint32_t num_ways;

    uint32_t getEmptyWay()
    {
        for(uint32_t i = 0; i < num_ways; i++)
        {
            if(!ways[i].valid)
            return i;
        }
         return num_ways;
    };

    bool hasEmptyWay(){return getEmptyWay() < num_ways;};
}; 

// class TagBufferEntry

class MemoryController : public MemObject{
    private:
    DDRMemory * BuildDDRMemory(Config& config, uint32_t frequency, uint32_t domain, g_string name, const std::string& prefix, uint32_t tBL, double timing_scale);
    g_string _name;
    public:
        CXLBridge cxl_bridge;
        MemObject * dram;
        Address max_dram_range;
        MemoryController(g_string& name, uint32_t frequency, uint32_t domain, Config& config);
        uint64_t access(MemReq& req);
        const char * getName() { return _name.c_str(); };
        void initStats(AggregateStat* parentStat); 
};


#endif