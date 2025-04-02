#ifndef CXL_MC_H
#define CXL_MC_H
#include "memory_hierarchy.h"
#include "stats.h"
#include "ddr_mem.h"

class CXLMemoryController : public MemObject{
    private:
    public:
        uint64_t cxl_port_handle_latency;
        MemObject * _cxl_memory; 
};

#endif