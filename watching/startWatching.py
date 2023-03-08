from bcc import BPF
# 引入需要的前端框架

# 加载BPF文件
bpf=BPF(src_file="hookCreateFork.c")

# 文本替换 
# 增加易用性，方便修改和过滤

# 从参数列表抓取参数替换参数最大值
bpf=bpf.replace("MAXARG", args.max_args)
# 如果设计了UID过滤规则就添加到源码
if args.uid:
    bpf=bpf.replace("UID_FILTER", 
            'if (uid != %s) {return 0;}' % args.uid)
else:
    bpf=bpf.replace("UID_FILTER", '')
# 如果设计了PPID过滤规则也添加到源码
if args.ppid:
    bpf=bpf.replace("PPID_FILTER",
            'if (data.ppid != %s) {return 0;}' % args.uuid)
else:
    bpf=bpf.replace("PPID_FILTER",'')


