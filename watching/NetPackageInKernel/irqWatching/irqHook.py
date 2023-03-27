from bcc import BPF



IRQ_Switch = True
nrFilter = 2

# Kprobe for irq_find_mapping
bpf_text = """
#include <include/linux/interrupt.h>

struct domain
{
    u32 nr;
    char irqName[30];
};

// 创建一个性能事件，在触发该事件的时时候发生前后端交互行为
BPF_PERF_OUTPUT(events);

void raise_softirq_test(struct pt_regs *ctx,  int nr)
{
    struct domain data = {};
    data.nr = nr;

    //IRQ_FILTER

    switch (nr)
    {
        case 0 :
            memcpy(data.irqName, "HI_SOFTIRQ", sizeof("HI_SOFTIRQ"));
        break;
        case 1 :
            memcpy(data.irqName, "TIMER_SOFTIRQ", sizeof("TIMER_SOFTIRQ"));
            events.perf_submit(ctx, &data, sizeof(data));
        break;
        case 2 :
            memcpy(data.irqName, "NET_TX_SOFTIRQ", sizeof("NET_TX_SOFTIRQ"));
            events.perf_submit(ctx, &data, sizeof(data));
        break;
        case 3 :
            memcpy(data.irqName, "NET_RX_SOFTIRQ", sizeof("NET_RX_SOFTIRQ"));
        break;
        case 4 :
            memcpy(data.irqName, "BLOCK_SOFTIRQ", sizeof("BLOCK_SOFTIRQ"));
        break;
        case 5 :
            memcpy(data.irqName, "IRQ_POLL_SOFTIRQ", sizeof("IRQ_POLL_SOFTIRQ"));
        break;
        case 6 :
            memcpy(data.irqName, "TASKLET_SOFTIRQ", sizeof("TASKLET_SOFTIRQ"));
        break;
        case 7 :
            memcpy(data.irqName, "SCHED_SOFTIRQ", sizeof("SCHED_SOFTIRQ"));
        break;
        case 8 :
            memcpy(data.irqName, "HRTIMER_SOFTIRQ", sizeof("HRTIMER_SOFTIRQ"));
        break;
        case 9 :
            memcpy(data.irqName, "RCU_SOFTIRQ", sizeof("RCU_SOFTIRQ"));
        break;
        default:
            memcpy(data.irqName, "ERROR", sizeof("ERROR"));
    }

}

void net_tx_action (struct pt_regs *ctx, struct softirq_action *h)
{
    struct domain data = {};
    memcpy(data.irqName, "NET_TX_ACTION", sizeof("NET_TX_ACTION"));
    events.perf_submit(ctx, &data, sizeof(data));
}

void net_rx_action (struct pt_regs *ctx, struct softirq_action *h)
{
    struct domain data = {};
    memcpy(data.irqName, "NET_RX_ACTION", sizeof("NET_RX_ACTION"));
    events.perf_submit(ctx, &data, sizeof(data));
}
"""





if IRQ_Switch:
    bpf_text = bpf_text.replace(
        "IRQ_FILTER", 'if (data.nr != %s) {return ;}' % nrFilter)
else:
    bpf_text = bpf_text.replace("IRQ_FILTER", ' ')


b = BPF(text=bpf_text)
b.attach_kprobe(event="__raise_softirq_irqoff", fn_name="raise_softirq_test")
b.attach_kprobe(event="net_tx_action", fn_name="net_tx_action")
b.attach_kprobe(event="net_rx_action", fn_name="net_rx_action")



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

