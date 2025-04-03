#include "mc.h"
#include "ddr_mem.h"
#include "zsim.h"
#include<iostream>
#include "g_std/g_vector.h"

MemoryController::MemoryController(g_string &name, uint32_t frequency, uint32_t domain, Config &config)
 : _name(name)
{
    max_dram_range = config.get<uint64_t>("sys.tiered_mem.dram_range",4)*1024*1024*1024;// Default 4GB
    cxl_bridge.set_min_addr(max_dram_range);
    cxl_bridge.valid_switches = config.get<int>("sys.tiered_mem.switch_count",0); // dram_counts > 8 
    cxl_bridge.valid_ports_per_switch = config.get<int>("sys.tiered_mem.port_counts",8);
    cxl_memory_expand_counts = config.get<int>("sys.tiered_mem.exact_dram_counts",1);

    g_vector<Address> cxl_m_addresses;
    for (int i = 0; i < cxl_memory_expand_counts; ++i) {
        std::ostringstream key;
        key << "sys.tiered_mem.cxl_m" << i;
        cxl_m_addresses.push_back(config.get<uint64_t>(key.str(), 0));
    }

    
};