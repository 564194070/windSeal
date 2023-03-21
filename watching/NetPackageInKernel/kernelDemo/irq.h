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
	void		        (*irq_enable)(struct irq_data *data);	//使能 不同IRQ的使能需要写入寄存器不同的bits,用于使能单个IRQ
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
	struct cpumask				*percpu_enabled;
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