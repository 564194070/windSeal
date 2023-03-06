#!/usr/bin/env python3
# Tracing execve() system call.
from bcc import BPF
from bcc.utils import printb


# 1) load BPF program
b = BPF(src_file="HookSetNs.c")

# 2) print header
print("%-6s %-16s %-3s %-3s" % ("PID", "COMM", "FD", "TYPE"))


# 3) define the callback for perf event
def print_event(cpu, data, size):
    # event data struct is generated from "struct data_t" by bcc
    event = b["events"].event(data)
    
    printb(b"%-6d %-16s %-6d %-16s" % (event.pid, event.comm, event.fd, event.typeName))


# 4) loop with callback to print_event
b["events"].open_perf_buffer(print_event)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()
