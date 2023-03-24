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