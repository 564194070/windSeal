// 位置:include/linux
// 对中断的控制
struct irq_chip {
	// 祖先硬件设备
	struct device	    *parent_device;
	// /proc/interupts看到的中断控制器名称
	const char	        *name;

	// 不同芯片的中断控制器对其挂接的IRQ有不同的控制方法，下面都是回调函数，指向系统实际中断控制器所使用的控制方式的函数指针构成。
	unsigned int	    (*irq_startup)(struct irq_data *data);	//启动
	void		        (*irq_shutdown)(struct irq_data *data);	//关闭
	void		        (*irq_enable)(struct irq_data *data);	//使能 不同IRQ的使能需要写入寄存器不同的bits,用于使能单个IRQ 对所有CPU有效
	void		        (*irq_disable)(struct irq_data *data);	//禁止

	void		        (*irq_ack)(struct irq_data *data);		//响应中断，清除当前中断使可以接受下个中断
	void		        (*irq_mask)(struct irq_data *data);		//屏蔽中断源
	void		        (*irq_mask_ack)(struct irq_data *data);	//屏蔽和响应中断
	void		        (*irq_unmask)(struct irq_data *data);	//开启中断源头
	void		        (*irq_eoi)(struct irq_data *data);

	int		            (*irq_set_affinity)(struct irq_data *data, const struct cpumask *dest, bool force);
	int		            (*irq_retrigger)(struct irq_data *data);
	int		            (*irq_set_type)(struct irq_data *data, unsigned int flow_type);			//将引脚设置为终端类型
	int		            (*irq_set_wake)(struct irq_data *data, unsigned int on);

	void		        (*irq_bus_lock)(struct irq_data *data);
	void		        (*irq_bus_sync_unlock)(struct irq_data *data);

	void		        (*irq_cpu_online)(struct irq_data *data);
	void		        (*irq_cpu_offline)(struct irq_data *data);

	void		        (*irq_suspend)(struct irq_data *data);
	void		        (*irq_resume)(struct irq_data *data);
	void		        (*irq_pm_shutdown)(struct irq_data *data);

	void		        (*irq_calc_mask)(struct irq_data *data);

	void		        (*irq_print_chip)(struct irq_data *data, struct seq_file *p);
	int		            (*irq_request_resources)(struct irq_data *data);
	void		        (*irq_release_resources)(struct irq_data *data);	//释放中断服务函数

	void		        (*irq_compose_msi_msg)(struct irq_data *data, struct msi_msg *msg);
	void		        (*irq_write_msi_msg)(struct irq_data *data, struct msi_msg *msg);

	int		            (*irq_get_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool *state);
	int		            (*irq_set_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool state);

	int		            (*irq_set_vcpu_affinity)(struct irq_data *data, void *vcpu_info);

	void		        (*ipi_send_single)(struct irq_data *data, unsigned int cpu);			//单独发给一个CPU
	void		        (*ipi_send_mask)(struct irq_data *data, const struct cpumask *dest);	//发给mask内所有CPU

	unsigned long	    flags;
};

//相关硬件的中断号码在
//arch/arm/mach-s3c24xx/include/mach/irqs.h中
// 0-15用作软件中断 CPU通过中断控制器编号+irq编号识别真实中断信息



// 位置: include/linux/irqdesc.h
// 对中断的描述
struct irq_desc {
	struct irq_common_data		irq_common_data;
	struct irq_data				irq_data;
	unsigned int 				__percpu	*kstat_irqs;
	irq_flow_handler_t			handle_irq;
#ifdef CONFIG_IRQ_PREFLOW_FASTEOI
	irq_preflow_handler_t		preflow_handler;
#endif
	// 是IRQ对应的中断处理函数(ISR) 理论上该指向函数，实际上指向了struct irqaction
	struct irqaction			*action;	/* IRQ action list */
	unsigned int				status_use_accessors;
	unsigned int				core_internal_state__do_not_mess_with_it;
	// 关闭该IRQ的嵌套深度 >0表示禁用中断 0表示可启用中断
	unsigned int				depth;		/* nested irq disables */
	unsigned int				wake_depth;	/* nested wake enables */
	unsigned int				tot_count;
	unsigned int				irq_count;	/* For detecting broken IRQs */
	unsigned long				last_unhandled;	/* Aging timer for unhandled count */
	unsigned int				irqs_unhandled;
	atomic_t					threads_handled;
	int							threads_handled_last;
	raw_spinlock_t				lock;
	struct cpumask				*percpu_enabled;		//位域 控制单个IRQ对单个CPU有效
	const struct cpumask		*percpu_affinity;
#ifdef CONFIG_SMP
	const struct cpumask		*affinity_hint;
	struct irq_affinity_notify  *affinity_notify;
#ifdef CONFIG_GENERIC_PENDING_IRQ
	cpumask_var_t				pending_mask;
#endif
#endif
	unsigned long				threads_oneshot;
	atomic_t					threads_active;
	wait_queue_head_t       	wait_for_threads;
#ifdef CONFIG_PM_SLEEP
	unsigned int				nr_actions;
	unsigned int				no_suspend_depth;
	unsigned int				cond_suspend_depth;
	unsigned int				force_resume_depth;
#endif
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry		*dir;
#endif
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	struct dentry				*debugfs_file;
	const char					*dev_name;
#endif
#ifdef CONFIG_SPARSE_IRQ
	struct rcu_head				rcu;
	struct kobject				kobj;
#endif
	struct mutex				request_mutex;
	int							parent_irq;
	struct module				*owner;
	// /proc/interupts 中断的名称
	const char		*name;		
} ____cacheline_internodealigned_in_smp;


// 位置: include\linux\irqaction
// 中断函数结构体
// 硬件设备卸载时，对应的中断处理函数也从IRQ链表中移除，
struct irqaction {
	// 历史遗留问题，早期处理器中硬中断很少，有些编号被分配给了标准系统组件(keyboard)
	// 为了服务于更多设备,共享有限的IRQ中断号，一个中断号对应多个不同处理函数，处理函数通过单链表串联
	// 中断发生时，IRQ链表上所有irqaction的handler都要被执行，判断是否是自己设备产生的中断。读设备中断状态寄存器，共享中断时，不是你产生的也会调用你的handler，发现不是就尽快退出
	
	// 中断处理函数
	irq_handler_t		handler;
	// 挂接在一个IRQ上不同设备的标识，方便分辨卸载谁
	void			*dev_id;
	void __percpu		*percpu_dev_id;
	// 共享中断IRQ队列中下一个中断处理函数
	struct irqaction	*next;
	irq_handler_t		thread_fn;
	struct task_struct	*thread;
	struct irqaction	*secondary;
	unsigned int		irq;
	unsigned int		flags;
	unsigned long		thread_flags;
	unsigned long		thread_mask;
	const char		*name;
	struct proc_dir_entry	*dir;
} ____cacheline_internodealigned_in_smp;


// 位置 : linux/linux/irq.h
// irq_chip的函数指针都要加该参数，与中断控制密切相关
// 在irq_desc中与中断控钱文琦666制器紧密联系这部分被单独提取，构成irq_data结构体
struct irq_data {
	u32						mask;
	unsigned int			irq;
	unsigned long			hwirq;
	struct irq_common_data	*common;
	// 指向了这个IRQ挂接的中断控制器
	// 使用irq_set_chip绑定后，可以利用irq_chip提供的各种函数32
	struct irq_chip			*chip;
	struct irq_domain		*domain;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_data			*parent_data;
#endif
	void					*chip_data;
};


// 位置:include/linux/irqdomain.h
// 将中断控制器local的物理中断号转换成Linux全局虚拟中断号的机制
// 虚拟：和具体的硬件连接没有关系，只是单纯变成了一个数字而已  虚拟->物理 映射  物理->虚拟 逆向映射
struct irq_domain {
	struct list_head link;
	const char *name;
	// 具体的映射过程
	const struct irq_domain_ops *ops;
	void *host_data;
	unsigned int flags;
	unsigned int mapcount;

	/* Optional data */
	struct fwnode_handle *fwnode;
	enum irq_domain_bus_token bus_token;
	struct irq_domain_chip_generic *gc;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_domain *parent;
#endif
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	struct dentry		*debugfs_file;
#endif

	/* reverse map data. The linear map gets appended to the irq_domain */
	irq_hw_number_t hwirq_max;
	// 对硬件映射有意义
	unsigned int revmap_direct_max_irq;
	// 对线性映射有意义
	unsigned int revmap_size;
	struct radix_tree_root revmap_tree;
	// 只对radix_tree 起作用
	struct mutex revmap_tree_mutex;
	// 中断从外部来，传递物理中断号，逆向映射，根据映射规则获取虚拟中断号
	// 大于256上radix tree 小于256上线性映射(假如只有两个中断0和63 但是还是要使用长度63的数组)
	// key是hwirq Value是irq
	unsigned int linear_revmap[];
};

// 位置: include/linux/irqdomain.h
// 具体的映射过程

struct irq_domain_ops {
	// 判断dst中由node描述的中断控制器是否和由d指向的irq_domain相匹配
	int (*match)(struct irq_domain *d, struct device_node *node,
		     enum irq_domain_bus_token bus_token);
	int (*select)(struct irq_domain *d, struct irq_fwspec *fwspec,
		      enum irq_domain_bus_token bus_token);
	//将物理中断号"翻译"成虚拟中断号
	int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
	void (*unmap)(struct irq_domain *d, unsigned int virq);
	// translate 根据dst的信息生成hwirq node设备节点信息,out_hwirq翻译后形成的中断号
	int (*xlate)(struct irq_domain *d, struct device_node *node,
		     const u32 *intspec, unsigned int intsize,
		     unsigned long *out_hwirq, unsigned int *out_type);
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	/* extended V2 interfaces to support hierarchy irq_domains */
	int (*alloc)(struct irq_domain *d, unsigned int virq,
		     unsigned int nr_irqs, void *arg);
	void (*free)(struct irq_domain *d, unsigned int virq,
		     unsigned int nr_irqs);
	int (*activate)(struct irq_domain *d, struct irq_data *irqd, bool reserve);
	void (*deactivate)(struct irq_domain *d, struct irq_data *irq_data);
	int (*translate)(struct irq_domain *d, struct irq_fwspec *fwspec,
			 unsigned long *out_hwirq, unsigned int *out_type);
#endif
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	void (*debug_show)(struct seq_file *m, struct irq_domain *d,
			   struct irq_data *irqd, int ind);
#endif
};


struct softirq_action
{
	void	(*action)(struct softirq_action *);
};

// kernel/softirq.c
// 作用：在内核加载之初 管理softirq中断源,在编译时确定中断源，不能在运行时动态增加。
static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

// 软中断，执行延迟处理的任务，不像硬中断，直接就要拿到CPU
do_softirq() 函数中，会遍历所有的软中断，执行相应的软中断处理程序。过多的软中断可能导致系统性能下降，过少的软中断可能无法满足系统需求
HI_SOFTIRQ=0,			//高优先级软中断的数量为 0，也就是说，系统中不存在需要高优先级处理的软中断
TIMER_SOFTIRQ =  1, 	//定时器软中断
NET_TX_SOFTIRQ = 2,		//网络传输软中断
NET_RX_SOFTIRQ = 3,		//网络接收软中断
BLOCK_SOFTIRQ =  4,		//块设备软中断 用于处理IO
IRQ_POLL_SOFTIRQ=5,		//IRQ轮询软中断，处理依赖软件轮询而不是中断的程序
TASKLET_SOFTIRQ =6,		//任务软中断，用于处理轻量级的中断处理任务
SCHED_SOFTIRQ =  7,		//调度软中断,内核线程上下文中执行的需要延迟处理的文件
HRTIMER_SOFTIRQ =8,		//高精度定时器软中断，处理高分辨率定时器任务
RCU_SOFTIRQ  =   9,		//RCU机制软中断，用于负责内存释放
NR_SOFTIRQS =   10,		//软中断的数量


// Linux收到网络包，分配到特定的softirq上下文，适当时机上下文处理数据包，这时候数据包就存在CPU的softnet_data队列中
// softnet_data 设计的原因是提高多核系统的网络吞吐量，每个CPU都有自己的软中断上下文处理。更快的处理收到的数据包，降低了网络延迟和提高了网络性能。
// 存储软件网络中接收到的数据包，per-CPU的数据结构，每个CPU都有自己的。
struct softnet_data {
	struct list_head	poll_list;				// 网络设备轮询列表，需要轮询的网络设备的列表
	struct sk_buff_head	process_queue;			// 正在处理的数据包队列，从input_pkt_queue移动到process_queue 然后处理并发往协议栈

	/* stats */
	unsigned int		processed;				//已处理数据包的数量
	//记录下一次处理软中断的时间戳。具体来说，如果当前时间戳小于 time_squeeze，则 softirq 将被延迟处理；否则，softirq 将会立即执行。
	unsigned int		time_squeeze;			//处理网络软中断（softirq）时间延迟问题的一种机制 限制网络中断处理的时间和频率
	//将接收到的网络数据包分配到多个CPU核心上去处理，提高系统并发性和网络处理能力，网络数据包会先由硬件接收，然后通过软件进行分配，最终由多个 CPU 核心进行处理
	unsigned int		received_rps;			//已接受RPS(接收数据包分配)数据包数量
#ifdef CONFIG_RPS
	struct softnet_data	*rps_ipi_list;			//多核心网络接受负载均衡器
#endif
#ifdef CONFIG_NET_FLOW_LIMIT
	struct sd_flow_limit __rcu *flow_limit;		//保存软件定义的流量限制规则，用于控制网络设备接收数据包的速率，以避免过度拥塞或流量爆发，从而保护网络稳定性和可靠性
#endif
	// Qdisc网络数据包的调度和排队
	struct Qdisc		*output_queue;			//当前网络接口的出口队列，通常被多个进程共享，管理出站数据包排队和调度
	struct Qdisc		**output_queue_tailp;	//指向网络接口的出口队列尾部的指针
	struct sk_buff		*completion_queue;		//链表，管理异步套接字操作的完成状态
#ifdef CONFIG_XFRM_OFFLOAD
	struct sk_buff_head	xfrm_backlog;			//积压数据包队列，存放因为负载过高无法及时处理的数据包，暂存避免被丢弃
#endif
	/* written and read only by owning cpu: */
	struct {
		u16 recursion;
		u8  more;
	} xmit;
#ifdef CONFIG_RPS
	/* input_queue_head should be written by cpu owning this struct,
	 * and only read by other cpus. Worth using a cache line.
	 */
	unsigned int		input_queue_head ____cacheline_aligned_in_smp;

	/* Elements below can be accessed between CPUs for RPS/RFS */
	call_single_data_t	csd ____cacheline_aligned_in_smp;
	struct softnet_data	*rps_ipi_next;				//协调多个 CPU 核心对网络接口收到的数据包进行处理
	unsigned int		cpu;						//表示CPU编号
	unsigned int		input_queue_tail;			//输入队列的尾部
#endif
	unsigned int		dropped;
	struct sk_buff_head	input_pkt_queue;			//输入数据包队列，存放需要处理的数据包
	struct napi_struct	backlog;					//帮助系统处理高速网络数据包接收和处理，提高网络性能和可靠性

};

// 用于实现网络设备的数据包调度和排队
struct Qdisc {
	int 			(*enqueue)(struct sk_buff *skb,
					   struct Qdisc *sch,
					   struct sk_buff **to_free);
	struct sk_buff *	(*dequeue)(struct Qdisc *sch);
	unsigned int		flags;
#define TCQ_F_BUILTIN		1
#define TCQ_F_INGRESS		2
#define TCQ_F_CAN_BYPASS	4
#define TCQ_F_MQROOT		8
#define TCQ_F_ONETXQUEUE	0x10 /* dequeue_skb() can assume all skbs are for
				      * q->dev_queue : It can test
				      * netif_xmit_frozen_or_stopped() before
				      * dequeueing next packet.
				      * Its true for MQ/MQPRIO slaves, or non
				      * multiqueue device.
				      */
#define TCQ_F_WARN_NONWC	(1 << 16)
#define TCQ_F_CPUSTATS		0x20 /* run using percpu statistics */
#define TCQ_F_NOPARENT		0x40 /* root of its hierarchy :
				      * qdisc_tree_decrease_qlen() should stop.
				      */
#define TCQ_F_INVISIBLE		0x80 /* invisible by default in dump */
#define TCQ_F_NOLOCK		0x100 /* qdisc does not require locking */
#define TCQ_F_OFFLOADED		0x200 /* qdisc is offloaded to HW */
	u32			limit;
	const struct Qdisc_ops	*ops;
	struct qdisc_size_table	__rcu *stab;
	struct hlist_node       hash;
	u32			handle;
	u32			parent;


	//用于网络设备队列管理
	struct netdev_queue	*dev_queue;

	struct net_rate_estimator __rcu *rate_est;
	struct gnet_stats_basic_cpu __percpu *cpu_bstats;
	struct gnet_stats_queue	__percpu *cpu_qstats;
	int			pad;
	refcount_t		refcnt;

	/*
	 * For performance sake on SMP, we put highly modified fields at the end
	 */
	struct sk_buff_head	gso_skb ____cacheline_aligned_in_smp;
	struct qdisc_skb_head	q;
	struct gnet_stats_basic_packed bstats;
	seqcount_t		running;
	struct gnet_stats_queue	qstats;
	unsigned long		state;
	struct Qdisc            *next_sched;
	struct sk_buff_head	skb_bad_txq;

	spinlock_t		busylock ____cacheline_aligned_in_smp;
	spinlock_t		seqlock;

	/* for NOLOCK qdisc, true if there are no enqueued skbs */
	bool			empty;
	struct rcu_head		rcu;

	/* private data */
	long privdata[] ____cacheline_aligned;
};


struct sock {
	/*
	 * Now struct inet_timewait_sock also uses sock_common, so please just
	 * don't add nothing before this first member (__sk_common) --acme
	 */
	struct sock_common	__sk_common;
#define sk_node			__sk_common.skc_node
#define sk_nulls_node		__sk_common.skc_nulls_node
#define sk_refcnt		__sk_common.skc_refcnt
#define sk_tx_queue_mapping	__sk_common.skc_tx_queue_mapping
#ifdef CONFIG_SOCK_RX_QUEUE_MAPPING
#define sk_rx_queue_mapping	__sk_common.skc_rx_queue_mapping
#endif

#define sk_dontcopy_begin	__sk_common.skc_dontcopy_begin
#define sk_dontcopy_end		__sk_common.skc_dontcopy_end
#define sk_hash			__sk_common.skc_hash
#define sk_portpair		__sk_common.skc_portpair
#define sk_num			__sk_common.skc_num
#define sk_dport		__sk_common.skc_dport
#define sk_addrpair		__sk_common.skc_addrpair
#define sk_daddr		__sk_common.skc_daddr
#define sk_rcv_saddr		__sk_common.skc_rcv_saddr
#define sk_family		__sk_common.skc_family
#define sk_state		__sk_common.skc_state
#define sk_reuse		__sk_common.skc_reuse
#define sk_reuseport		__sk_common.skc_reuseport
#define sk_ipv6only		__sk_common.skc_ipv6only
#define sk_net_refcnt		__sk_common.skc_net_refcnt
#define sk_bound_dev_if		__sk_common.skc_bound_dev_if
#define sk_bind_node		__sk_common.skc_bind_node
#define sk_prot			__sk_common.skc_prot
#define sk_net			__sk_common.skc_net
#define sk_v6_daddr		__sk_common.skc_v6_daddr
#define sk_v6_rcv_saddr	__sk_common.skc_v6_rcv_saddr
#define sk_cookie		__sk_common.skc_cookie
#define sk_incoming_cpu		__sk_common.skc_incoming_cpu
#define sk_flags		__sk_common.skc_flags
#define sk_rxhash		__sk_common.skc_rxhash

	socket_lock_t		sk_lock;
	atomic_t		sk_drops;
	int			sk_rcvlowat;
	// 错误消息队列
	struct sk_buff_head	sk_error_queue;
	// 套接字通用缓冲区
	struct sk_buff		*sk_rx_skb_cache;
	// 套接字接收缓冲区
	struct sk_buff_head	sk_receive_queue;
	/*
	 * The backlog queue is special, it is always used with
	 * the per-socket spinlock held and requires low latency
	 * access. Therefore we special case it's implementation.
	 * Note : rmem_alloc is in this structure to fill a hole
	 * on 64bit arches, not because its logically part of
	 * backlog.
	 */
	struct {
		atomic_t	rmem_alloc;
		int		len;
		struct sk_buff	*head;
		struct sk_buff	*tail;
	} sk_backlog;
	// 套接字挂起的数据队列，未能及时处理的消息
#define sk_rmem_alloc sk_backlog.rmem_alloc

	// 已经被分配但尚未被发送的空间大小
	int			sk_forward_alloc;
#ifdef CONFIG_NET_RX_BUSY_POLL
	// 数据包第一次到到达协议栈的时延，用来计算时延
	unsigned int		sk_ll_usec;
	/* ===== mostly read cache line ===== */
	//网络接口在处理中断时所使用的NAPI软件中断处理器的ID
	unsigned int		sk_napi_id;
#endif
	// TCP套接字接收缓冲区的大小 setsockopt函数来设置
	int			sk_rcvbuf;

	struct sk_filter __rcu	*sk_filter;
	union {
		// 套接字等待队列
		struct socket_wq __rcu	*sk_wq;
		/* private: */
		struct socket_wq	*sk_wq_raw;
		/* public: */
	};
#ifdef CONFIG_XFRM
	struct xfrm_policy __rcu *sk_policy[2];
#endif
	//目的地址缓存项的结构体
	//缓存路由选择和数据包转发
	struct dst_entry	*sk_rx_dst;
	//路由选择的结果会被缓存到目的地址缓存项中
	struct dst_entry __rcu	*sk_dst_cache;
	atomic_t		sk_omem_alloc;
	int			sk_sndbuf;

	/* ===== cache line for TX ===== */
	//记录网络套接字当前已经排队等待发送的数据量的计数器
	// 如果发送缓冲区的空间不足以存储需要发送的数据，那么这些数据就会被排队等待发送。此时，sk_wmem_queued 的值就会增加，以记录当前已经排队等待发送的数据量
	int			sk_wmem_queued;
	//为网络套接字的发送缓冲区分配一定大小的内存空间
	refcount_t		sk_wmem_alloc;
	unsigned long		sk_tsq_flags;
	union {
		// 指向网络套接字发送队列的第一个数据包
		struct sk_buff	*sk_send_head;
		// TCP重传队列
		struct rb_root	tcp_rtx_queue;
	};

	// 发送队列中的最后一包数据的指针
	struct sk_buff		*sk_tx_skb_cache;
	//记录发送队列的指针
	struct sk_buff_head	sk_write_queue;
	__s32			sk_peek_off;
	int			sk_write_pending;
	__u32			sk_dst_pending_confirm;
	u32			sk_pacing_status; /* see enum sk_pacing */
	long			sk_sndtimeo;
	struct timer_list	sk_timer;
	__u32			sk_priority;
	__u32			sk_mark;
	unsigned long		sk_pacing_rate; /* bytes per second */
	unsigned long		sk_max_pacing_rate;
	struct page_frag	sk_frag;
	netdev_features_t	sk_route_caps;
	netdev_features_t	sk_route_nocaps;
	netdev_features_t	sk_route_forced_caps;
	int			sk_gso_type;
	unsigned int		sk_gso_max_size;
	gfp_t			sk_allocation;
	__u32			sk_txhash;

	/*
	 * Because of non atomicity rules, all
	 * changes are protected by socket lock.
	 */
	u8			sk_padding : 1,
				sk_kern_sock : 1,
				sk_no_check_tx : 1,
				sk_no_check_rx : 1,
				sk_userlocks : 4;
	u8			sk_pacing_shift;
	u16			sk_type;
	u16			sk_protocol;
	u16			sk_gso_max_segs;
	unsigned long	        sk_lingertime;
	struct proto		*sk_prot_creator;
	rwlock_t		sk_callback_lock;
	int			sk_err,
				sk_err_soft;
	u32			sk_ack_backlog;
	u32			sk_max_ack_backlog;
	kuid_t			sk_uid;
#ifdef CONFIG_NET_RX_BUSY_POLL
	u8			sk_prefer_busy_poll;
	u16			sk_busy_poll_budget;
#endif
	struct pid		*sk_peer_pid;
	const struct cred	*sk_peer_cred;
	long			sk_rcvtimeo;
	ktime_t			sk_stamp;
#if BITS_PER_LONG==32
	seqlock_t		sk_stamp_seq;
#endif
	u16			sk_tsflags;
	u8			sk_shutdown;
	u32			sk_tskey;
	atomic_t		sk_zckey;

	u8			sk_clockid;
	u8			sk_txtime_deadline_mode : 1,
				sk_txtime_report_errors : 1,
				sk_txtime_unused : 6;

	struct socket		*sk_socket;
	void			*sk_user_data;
#ifdef CONFIG_SECURITY
	void			*sk_security;
#endif
	struct sock_cgroup_data	sk_cgrp_data;
	struct mem_cgroup	*sk_memcg;
	void			(*sk_state_change)(struct sock *sk);
	void			(*sk_data_ready)(struct sock *sk);
	void			(*sk_write_space)(struct sock *sk);
	void			(*sk_error_report)(struct sock *sk);
	int			(*sk_backlog_rcv)(struct sock *sk,
						  struct sk_buff *skb);
#ifdef CONFIG_SOCK_VALIDATE_XMIT
	struct sk_buff*		(*sk_validate_xmit_skb)(struct sock *sk,
							struct net_device *dev,
							struct sk_buff *skb);
#endif
	void                    (*sk_destruct)(struct sock *sk);
	struct sock_reuseport __rcu	*sk_reuseport_cb;
#ifdef CONFIG_BPF_SYSCALL
	struct bpf_local_storage __rcu	*sk_bpf_storage;
#endif
	struct rcu_head		sk_rcu;
};

