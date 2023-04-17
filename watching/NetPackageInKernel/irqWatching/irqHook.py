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
    u32 nr;
    char irqName[30];
};

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


if IRQ_Switch:
    bpf_text = bpf_text.replace(
        "IRQ_FILTER", 'if (data.nr != %s) {return ;}' % nrFilter)
else:
    bpf_text = bpf_text.replace("IRQ_FILTER", ' ')


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

