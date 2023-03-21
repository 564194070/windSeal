数据包内核态流程监控，主要知识脉络来自于《深入理解Linux网络一书》

1.从软中断开始监控 
    外设速度低于CPU,为了在必要的时候获得CPU，在外设和CPU之间提供了一种中断的机制，现代OS也来时钟驱动也需要中断支持。
    在中断到达CPU之前，首先会经过中断控制器，根据中断源的优先级对中断进行派发。多核系统中，中断控制器还需要根据CPU预先设定的规则，将某个中断送入指定CPU，实现中断负载均衡。
中断控制器控制相关数据结构 struct irq_chip 具体解析详见 kernelDemo/irq.h
中断控制器描述相关数据结构 struct irq_desc 具体解析详见 kernekDemo/irq.h

2.软中断和硬中断
    IRQ有irq和hwirq两种编号。irq软件使用的中断号，hwirq是硬件中断号或者说物理中断号。
    引入irq_domain概念后，操作系统使用的IRQ与具体的硬件连线关系被解耦，实现了对底层中断控制器细节的隐藏

3.中断处理机制
    IRQ对应的设备驱动程序需要在中断到来之前安装/注册该中断的处理函数，ISR。
    ISR的安装分为两级,第一级针对IRQ线，generic handler,第二级针对挂载在这个IRQ线上不同设备的，称为specific handler
    第一级针对IRQ的挂载在初始化的时候由irq_set_handler(irq,handle)添加。irq:IRQ号码，handler第一级处理函数。第一步分配irqAction,创建中断函数结构体，填写相关配置;第二步，将中断处理函数添加到IRQ链表
    request是指中断向CPU请求服务，对于级联中断控制器，外侧中断控制器不能直接向CPU提交请求，但是可以和与之相连的内测中断控制器请求服务。
    对IRQ队列,interrupt-parent标识所连接的中断控制器。对于中断控制器，interrupt-parent标识标识级联模式下，其所连接的这个内测的中断控制器。
    request_threaded_irq() 主要用于填写中断函数结构体，包括第二级处理函数handler和thread_fn,区分共享IRQ的不同设备的dev_id和devname,涉及到irqaction的分配，可能引起睡眠和调度，不能在中断上下文中使用。
    从内核角度来看，中断上下文是执行ISR时的上下文，确保不会出现线程的切换，内核设计理念，并不是一定要遵守。request_threaded_irq的前身是request_irq，增加新参数thread_fn上，用于中断线程化。如果IRQ链表没有irq_action说明尚未安装驱动程序，也就没有设备在使用这条IRQ线，没有安装ISR的中断，应该屏蔽。否则设备可能会由于自己中断请求得不到服务，一致中断CPU。
    [注] dts 设备树源文件，用来描述设备信息

4.ISR执行流程
    






















参考文章:https://zhuanlan.zhihu.com/p/83709066

