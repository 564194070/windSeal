// 位置:
// 对软中断的控制
struct irq_chip {
	// 祖先硬件设备
	struct device	    *parent_device;
	// /proc/interupts看到的中断控制器名称
	const char	        *name;

	// 不同芯片的中断控制器对其挂接的IRQ有不同的控制方法，下面都是回调函数，指向系统实际中断控制器所使用的控制方式的函数指针构成。
	unsigned int	    (*irq_startup)(struct irq_data *data);	//启动
	void		        (*irq_shutdown)(struct irq_data *data);	//关闭
	void		        (*irq_enable)(struct irq_data *data);	//使能
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



