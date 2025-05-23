# Bound and Weave

在内存系统建模中，**Bound阶段**和**Weave阶段**是模拟内存访问请求处理流程的两个关键阶段，主要用于分离 **延迟计算** 和 **请求调度** 的逻辑。



## **Bound Phase**

**核心作用**

- **快速计算最小理论延迟**：在不考虑实际资源冲突的情况下，根据内存通道的基本参数（如行缓冲命中、Bank冲突等）返回一个理论最小延迟值。
- **无副作用**：仅计算延迟，不修改内存通道状态（如不实际占用Bank或总线资源）。

**典型行为**

- 输入一个内存请求（`MemReq`），输出该请求的 **最小可能延迟**（例如：`tRCD + tCL`）。
- 适用于快速路径（如CPU流水线前端）的预调度决策。



## **Weave Phase**

**核心作用**

- **实际调度请求**：根据内存通道的实时状态（如Bank忙、总线占用等）和调度策略（FR-FCFS、优先级等），确定请求的实际执行时间。
- **有副作用**：更新内存控制器状态（如标记Bank为忙、记录请求队列）。

**典型行为**

- 接收请求和Bound阶段计算的延迟，结合当前资源竞争情况，分配一个具体的执行时间戳（`tick`）。
- 处理冲突（如仲裁多个请求对同一Bank的访问）。



## B&W in ZSim

* `memory->access(MemReq &req)` 是内存子系统bound阶段的核心功能，以返回最小理论延迟
  * 根据请求类型（读/写）返回预定义的 `minRdDelay` 或 `minWrDelay`。
  * **不修改内存通道状态**，仅用于前端快速预测。
* 在`Boyu Tian`在其`zsim-ndp`中关于`weave`阶段的设计是`acceptAccEvent`和`respondAccEvent`两个函数。从命名上，看起来和`gem5`里的`sendTimingReq`以及`sentTimingResp`类似。



## AccEvent

这个是代码中定义的变量名的一部分，原生`ZSim`的`ddr_mem.h`中就有`DDRMemoryAccEvent`，在`zsim-ndp`代码中也有类似的`MemChannelAccEvent`。

在`ddr_mem.cpp`中，`DDRMemoryAccEvent`被用在`enqueue`函数中；在`zsim-ndp/mem_channel.h`代码中，`MemChannelAccEvent`被用在`acceptAccEvent`和`respondAccEvent`。

* 从注释的角度来看，`AccEvent`是指向响应事件的指针。且该变量一般对应的对象名为`ev`

  * ```c++
    AccEvent *ev;
    ```


`AccEvent`直接继承了`TimingEvent`

