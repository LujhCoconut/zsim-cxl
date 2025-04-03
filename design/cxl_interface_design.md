# Interface

```c
@author:Jiahao Lu  // Feel free to contact me via email:lujhcoconut@foxmail.com
```



## CXLBridge

Within this interface, it receives memory access requests from the top-level controller, analyzes the request type, and determines which CXL-Switch to use to identify the appropriate bus path. The top-level controller should have already properly filtered out requests with illegal address ranges.

在此接口中，用于接收来自顶层控制器的访存请求并分析请求类型和确定哪一个CXL-Switch以确定请求该走哪条总线，顶层控制器应当已经正确过滤了非法地址范围。



### **Primary member variables**

```c++
g_vector<std::pair<Address,Address>> switches_ranges; 
```

* Register the physical memory address range for each CXL-Switch. Note: CXL supports insertion of memory modules with different generations and capacities.
* 将每个CXL-Switch的物理内存地址范围注册。注意：CXL支持插入不同代和容量的内存。



```c++
g_vector<CXLSwitch> cxl_switches;
```

* Multiple CXL-Switch objects are instantiated.
* 定义了多个CXL-Switch对象



### Primary Function

* `protocal_analyze_bound_phase`
  * Used for analyzing request types
  *  用于分析请求类型
* `switch_select`
  * Determines which CXL-Switch the request address belongs to
  * 根据请求的地址分析处于哪一个CXL-Switch
* `transfer_delay_in_PCIe`
  * Determines the transmission latency on the link based on the CXL-Switch index
  * 根据CXL-Switch的索引确定在链路上传输的延迟
* `forward_to_detaild_port`
  * Routes the specific request to the corresponding CXL port of the target CXL-Switch for processing
  * 将具体的请求转发给对应的CXL-Switch的对应CXL端口进行处理
* `access` 直接调用``forward_to_detaild_port``





## CXL-Switch

This interface defines the following key functionalities for CXL-Switch operation:

* On-chip network transmission latency parameters
* Physical memory address ranges registered for each CXL-Port on the CXL-Switch
* Request analysis and forwarding logic to the appropriate CXL-Port

这个接口定义了CXL-Switch的片上网络传输延迟，以及对应各个CXL-Port在CXL-Switch上注册的物理内存地址范围，并分析请求，将请求转发给对应的CXL-Port。



### **Primary member variables**

```c++
g_vector<std::pair<Address,Address>> switch_ports_ranges;
```

* The physical address ranges are registered on the CXL-Switch
* 在CXL-Switch上注册了对应的物理地址范围



```c++
g_vector<CXLMemoryController> cxl_memory_ctrls;
```

* Multiple CXL-Port (CXL-MC) objects are instantiated.
* 定义了多个CXL-Port(即CXL-MC)对象



### Primary Function

* `port_select`
  * Routes to the corresponding port based on the registered memory address range.
  * 根据注册的内存地址范围找到对应的端口
* `forward_to_detailed_cxl_memory`
  * Routes the corresponding request to the specific CXL-Port's memory for access.
  * 将对应的请求转发到具体CXL-Port的Memory进行访问



## CXL-MC

Functionally, this acts as the CXL-Port's dedicated memory controller, managing access latency and physical memory transactions.

实际上就是连接CXL-Port的Memory Controller，负责Port到Memory的处理延迟以及和实际内存的交互。

> Note: CXL-based memory modules may differ in frequency, capacity, and other timing parameters.





## Top-level Memory Controller

`src/mc.cpp`

The top-level controller distributes memory requests to either DRAM or CXL-based memory, while implementing memory management policies and performing basic computations.

最顶层的控制器，负责分发内存请求到DRAM或者CXL-Based Memory，负责一些内存管理的策略和简单计算。

All memory initialization procedures are implemented within this file.

所有内存初始化步骤均在此文件中实现。



