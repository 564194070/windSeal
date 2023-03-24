from bcc import BPF

# Kprobe for irq_find_mapping
bpf_text = """
#include <include/linux/irqdomain.h>

struct domain
{
    u32 nr;

};

// 创建一个性能事件，在触发该事件的时时候发生前后端交互行为
BPF_PERF_OUTPUT(events);

int test(struct pt_regs *ctx, int nr)
{
    struct domain data = {};
    data.nr = nr;
    events.perf_submit(ctx, &data, sizeof(data));
    return 0;
}
"""

b = BPF(text=bpf_text)
b.attach_kprobe(event="raise_softirq", fn_name="test")

# header
#print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "MESSAGE"))

def print_event(cpu, data, size):
    # 获取性能事件打印的参数
    event = b["events"].event(data)
    len = event.nr
    print("%-18s" % len)

# 循环打印性能事件
b["events"].open_perf_buffer(print_event)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()