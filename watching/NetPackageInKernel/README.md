数据包内核态流程监控，主要知识脉络来自于《深入理解Linux网络一书》

irq_chip 中断控制器
irq_desc 中断描述信息
    irq_data 与中断控制器紧密连接
irq_action 中断处理函数
irq_domain 物理中断和逻辑中断源

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
    从内核角度来看，中断上下文是执行ISR时的上下文，确保不会出现线程的切换，内核设计理念，并不是一定要遵守。request_threaded_irq的前身是request_irq，增加新参数thread_fn上，用于中断线程化。如果IRQ链表没有irq_action说明尚未安装驱动程序，也就没有设备在使用这条IRQ线，没有安装ISR的中断，应该屏蔽。否则设备可能会由于自己中断请求得不到服务，一致中断CPU。free_irq可以接触安装，没有任何驱动设备安装在irq线上，IRQ应该被关闭。
    [注] dts 设备树源文件，用来描述设备信息

4.ISR执行流程
    中断经过中断控制器到达CPU后，先irq_find_mapping(),根据物理终端hwirq查找映射数组，得到虚拟中断号irq。

5.softirq的类型
    tasklet的实现是建立在softirq之上的，内核2.3版本引入的，work queue，内核2.5版本引入
    tasklet HI_SOFTIRQ,TASKLET_SOFTIRQ
    网络的发送和接收操作 NET_TX_SOFTIRQ,NET_RX_SOFTIRQ
    实现SMP系统上周期性负载均衡的调度器 SCHED_SOFTIRQ
    启用高分辨率定时器需要HRTIMER_SOFTIRQ
    
6.softirq调用流程
    void open_softirq(int nr, void (*action)(struct softirq_action *)) 
    向softirq中断源添加新的中断源，使中断源和执行函数绑定

    完成了HI_SOFTIRQ和TASKLET_SOFTIRQ的执行函数的注册
    void __init softirq_init() 
    -- open_softirq(TASKLET_SOFTIRQ, tasklet_action);
    -- open_softirq(HI_SOFTIRQ, tasklet_hi_action);

    其他softirq是在各自模块中初始化的
    void __init init_timers(void)
    -- init_timer_cpus()
    -- open_softirq(TIMER_SOFTIRQ，run_timer_softirq);
    前半段中断结束后，调用raise_softirq() 设置softirq的pengding图， 由__softirq_pending的per-CPU形式的变量表示，
    让内核将指定的软中断加入到全局队列中，从而触发软中断处理程序的执行

    /softirq相关序号定义在include/linux/interrupt中,


ksoftirqd线程创建过程
    位置: /kernel/smpboot.c
    函数: int smpboot_register_percpu_thread(struct smp_hotplug_thread *plug_thread)
    1.启动内核->2.添加本地线程->3.处理软中断
    内核代码:
    // softirq_threads 内核中的一组内核线程，用于处理软中断(网络中断，磁盘IO中断)
    // Linux为每个CPU都维护了一个软中断队列，事件发生时，内核将事件添加到队列中，软中断线程处理队列中的事件，在CPU空闲时。
    static struct smp_hotplug_thread softirq_threads = {
        .store			    = &ksoftirqd,
        .thread_should_run	= ksoftirqd_should_run,
        .thread_fn		    = run_ksoftirqd,
        .thread_comm		= "ksoftirqd/%u",
    };

    static __init int spawn_ksoftirqd(void)
    {
        cpuhp_setup_state_nocalls(CPUHP_SOFTIRQ_DEAD, "softirq:dead", NULL,
                    takeover_tasklets);
        BUG_ON(smpboot_register_percpu_thread(&softirq_threads));
        // BUG_ON 内核中的宏定义,运行时检查条件是否满足,条件不满足就触发内核崩溃并输出错误
        // smpboot_register_percpu_thread 用于将一个特定的线程注册为多处理器系统中本地处理器CPU的专用线程 只运行在指定CPU上
        // 每个CPU都有自己内核线程 用于处理本地中断和软中断, 在多核系统中，这可以有效地利用每个 CPU 的处理能力，提高系统的并发性能。

        return 0;
    }
    // spawn_ksoftirqd 是ksoftirqd的守护进程,管理内核系统的软件中断处理程序,自动调整softirq_threads的内核线程
    early_initcall(spawn_ksoftirqd);

    ksoftirqd创建后会进入线程循环函数ksoftirq_should_run 和 run_ksoftirq

网络子模块初始化
    位置:/net/core/dev.c

    //初始化网络模块
    subsys_initcall初始化网络子系统
    //遍历系统中每个可能存在的CPU核心,在每个CPU核心上执行一次循环体中的代码
    for_each_possible_cpu(i)
    
    // 创建延迟执行任务，将比较重的任务磁盘IO，网络IO推迟到空闲时间执行，避免对系统性能影响
	struct work_struct *flush = per_cpu_ptr(&flush_works, i);
    // 处理接收数据包的软中断上下文信息
	struct softnet_data *sd = &per_cpu(softnet_data, i);

    // 注册发送和接收的中断处理函数
    open_softirq(NET_TX_SOFTIRQ, net_tx_action);
	open_softirq(NET_RX_SOFTIRQ, net_rx_action);










参考文章:https://zhuanlan.zhihu.com/p/83709066

