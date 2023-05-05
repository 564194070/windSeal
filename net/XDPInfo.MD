
XDP程序的上下文数据结构
struct xdp_md {
	__u32 data;                 //指向数据包数据的指针，即数据包在内存中的起始地址
	__u32 data_end;             //指向数据包数据的尾端的指针，即数据包在内存中的结束地址
	__u32 data_meta             //元数据，可以用来传递额外的信息
	__u32 ingress_ifindex;      //表示数据包进入的网络设备的 ifindex
	__u32 rx_queue_index;       //表示接收队列的索引
	__u32 egress_ifindex;       //表示当前运行的 XDP 程序的 ID
};

XDP动作
XDP_PASS，将数据包立即传递到下一个网络协议层，不对数据包做任何修改
XDP_DROP：丢弃数据包，不做任何处理。
XDP_ABORTED：放弃数据包，并向内核报告错误状态（由驱动程序决定错误类型）。
XDP_TX：将数据包发送到网络设备的发送队列中，类似于传统的 sk_buff 发送方式。
XDP_REDIRECT：将数据包重定向到指定的网络设备上进行处理。
XDP_TX_EGRESS：将数据包发送到 egress （出口）网络设备的发送队列中。

