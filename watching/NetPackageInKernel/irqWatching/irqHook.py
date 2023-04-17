from bcc import BPF

IRQ_Switch = True
nrFilter = 2

PORT_Switch = True
srcPort = 5641
dstPort = 5642


# Kprobe for irq_find_mapping
bpf_text = """
#include <include/linux/interrupt.h>

struct processInfo
{
    u32 nr;             // 中断号 不涉及中断的流程为 -1
    char irqName[30];   // 中断类型, 不涉及中断的为空

    u32 uid;            // 用户信息
    u32 processID;      // 进程号
    u32 threadID;       // 线程号
    u32 threadGroupID;  // 线程组号
    u32 ppid;           // 父进程号
};

static inline void  getInfoFromSkaddr(struct pt_regs *ctx, struct sockaddr *skaddr, struct data_t data)
{
        struct sockaddr_in *sockaddrin;

        sockaddrin = (struct sockaddr_in *)(&skaddr);
        data.sPort = bpf_ntohs(sockaddrin->sin_port);
        data.saddr = sockaddrin->sin_addr.s_addr;
        data.protocal = sockaddrin->sin_family;
        events.perf_submit(ctx, &data, sizeof(data));

}
static inline void  getInfoFromSkbuff(struct pt_regs *ctx,struct sk_buff *skb, struct data_t data)
{
        struct iphdr *iphdrOutput;
        struct tcphdr *tcphdrOutput;
        iphdrOutput = (struct iphdr *)(skb->head+ skb->network_header);
        tcphdrOutput = (struct tcphdr*)(skb->head + skb->transport_header);
        data.pid = bpf_get_current_pid_tgid() >> 32;
        data.ts = bpf_ktime_get_ns();
        bpf_get_current_comm(&data.comm, sizeof(data.comm));
        data.saddr = iphdrOutput->saddr;
        data.daddr = iphdrOutput->daddr;
        data.sPort = bpf_ntohs(tcphdrOutput->source);
        data.dPort = bpf_ntohs(tcphdrOutput->dest);
        data.protocal = skb->protocol;
        events.perf_submit(ctx, &data, sizeof(data));        
}


static inline void getInfoFromSk(struct pt_regs *ctx, struct sock *sk, struct data_t data)
{
        //服务端 套接字的发送端口就是数据包的目标端口 套接字的源地址就是数据包的目标地址 因为所处位置不同
        data.pid = bpf_get_current_pid_tgid() >> 32;
        data.ts = bpf_ktime_get_ns();
        bpf_get_current_comm(&data.comm, sizeof(data.comm));
        data.daddr = sk->__sk_common.skc_rcv_saddr;
        data.saddr = sk->__sk_common.skc_daddr;
        data.dPort = bpf_ntohs(sk->__sk_common.skc_num);
        data.sPort = bpf_ntohs(sk->__sk_common.skc_dport);
        data.protocal = sk->__sk_common.skc_family;
        events.perf_submit(ctx, &data, sizeof(data));
}

static void inline GetProcessInfo(struct processInfo *process)
{

	// uid 用户ID
	process->uid = bpf_get_current_uid_gid() & 0xffffffff;
	// pid_tgid 线程组标识+线程标识
	u64 pid_tgid = bpf_get_current_pid_tgid();
	// tid 线程ID 线程唯一
	process->threadID = (u32)pid_tgid;
	// pid 进程ID
	process->threadID = pid_tgid >> 32;

    // 辅助函数 获取任务(进程)的task结构体
	struct task_struct *task = (struct task_struct *)bpf_get_current_task();
	// 从task里面拿父进程信息
	process->ppid = task->real_parent->tgid;
	// 获取进程所在的cgroup空间
    // netns = task->nsproxy->net_ns->ns.inum;
}



// 创建一个性能事件，在触发该事件的时时候发生前后端交互行为
BPF_PERF_OUTPUT(events);

// 监测软中断触发,修改标志位,使得koftirq可以读出发生了软中断,然后调用注册的处理函数
int raise_softirq_test(struct pt_regs *ctx,  int nr)
{    
    struct processInfo data = {};
    data.nr = nr;

    //IRQ_FILTER

    switch (nr)
    {
        case 2 :
            memcpy(data.irqName, "NET_TX_SOFTIRQ", sizeof("NET_TX_SOFTIRQ"));
        break;
        case 3 :
            memcpy(data.irqName, "NET_RX_SOFTIRQ", sizeof("NET_RX_SOFTIRQ"));
        break;
        default:
            memcpy(data.irqName, "ERROR", sizeof("ERROR"));
    }

    if (nr == 2 || nr == 3)
    {
        return 0;
    }
    events.perf_submit(ctx, &data, sizeof(data));
    return 0;
}

// 网络发送中断注册函数
void net_tx_action (struct pt_regs *ctx, struct softirq_action *h)
{
    struct processInfo data = {};
    memcpy(data.irqName, "NET_TX_ACTION", sizeof("NET_TX_ACTION"));
    events.perf_submit(ctx, &data, sizeof(data));
}

// 网络接收中断注册函数
void net_rx_action (struct pt_regs *ctx, struct softirq_action *h)
{
    struct processInfo data = {};
    memcpy(data.irqName, "NET_RX_ACTION", sizeof("NET_RX_ACTION"));
    events.perf_submit(ctx, &data, sizeof(data));
}


// 驱动取下网络包
void napi_gro_receive(struct pt_regs *ctx, struct napi_struct * napi, struct sk_buff * skb)
{
    struct processInfo data = {};
    memcpy(data.irqName, "napi_gro_receive", sizeof("napi_gro_receive"));
    events.perf_submit(ctx, &data, sizeof(data));
}

// 网络包处理框架
int netif_receive_skb(struct pt_regs *ctx, struct sk_buff *skb)
{
    struct processInfo data = {};
}


// iptables PREROUTING链路
int ip_rcv(struct pt_regs *ctx, struct sk_buff *skb, struct net_device *dev, struct packet_type *pt,struct net_device *orig_dev)
{

}

// iptables INPUT链路
int ip_local_deliver(struct pt_regs *ctx, struct sk_buff *skb)
{

}

// iptables FORWARD链路
int ip_forward(struct pt_regs *ctx, struct sk_buff *skb)
{

}

// iptables OUTPUT链路
int ip_local_out(struct pt_regs *ctx, struct net *net, struct sock *sk, struct sk_buff *skb)
{

}

// iptables POSTROUTING 链路
int ip_output(struct pt_regs *ctx, struct net *net, struct sock *sk, struct sk_buff *skb)
{

}
"""

# 软中断信息获取
if IRQ_Switch:
    bpf_text = bpf_text.replace(
        "IRQ_FILTER", 'if (data.nr != %s) {return ;}' % nrFilter)
else:
    bpf_text = bpf_text.replace("IRQ_FILTER", ' ')

# 
if PORT_Switch:
    bpf_text = bpf_text.replace(
        "PORT_SWITCH", 'if (data.port != %s) {return ;}' % srcPort)
else:
    bpf_text = bpf_text.replace("RORT_SWITCH", ' ')


b = BPF(text=bpf_text)
#b.attach_kprobe(event="__raise_softirq_irqoff", fn_name="raise_softirq_test")
#b.attach_kprobe(event="net_tx_action", fn_name="net_tx_action")
#b.attach_kprobe(event="net_rx_action", fn_name="net_rx_action")
b.attach_kprobe(event="napi_gro_receive", fn_name="napi_gro_receive")



# header
#print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "MESSAGE"))

def print_event(cpu, data, size):
    # 获取性能事件打印的参数
    event = b["events"].event(data)
    len = event.nr
    irqName = event.irqName.decode('utf8')
    print("%-18s %-50s" % (len, irqName))

# 循环打印性能事件
b["events"].open_perf_buffer(print_event)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()

