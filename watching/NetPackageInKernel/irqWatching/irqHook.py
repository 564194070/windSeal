from bcc import BPF

# Kprobe for irq_find_mapping
bpf_text = """
#include <include/linux/irqdomain.h>


/*struct domain
{
    u64 hwirq;
    u32 len;
    unsigned int size;
    const char *name;
};
*/

int __irq_resolve_mapping(struct pt_regs *ctx, struct irq_domain *domain, irq_hw_number_t hwirq, unsigned int *irq, struct irq_desc * retval)
{
    //struct domain data = {};
    //data.len = sizeof(*domain).name;
    //data.size = (*domain).revmap_size;
    //data.name = (*domain).name;
    return 0;
}
"""

b = BPF(text=bpf_text)
b.attach_kprobe(event="__irq_resolve_mapping", fn_name="__irq_resolve_mapping")

# header
print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "MESSAGE"))

while True:
    try:
        (task, pid, cpu, flags, ts, msg) = b.trace_fields()
        print("%-18.9f %-16s %-6d %s" % (ts, task.decode('utf-8', 'replace'), pid, msg.decode('utf-8', 'replace')))
    except KeyboardInterrupt:
        exit()