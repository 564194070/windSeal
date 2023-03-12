from bcc import BPF
import argparse

# 引入需要的前端框架


# BCC前端会有以下几部分代码
# 1.声明后文中可能用到的函数；2.参数解析相关函数；3.桩点相关函数



# 1.声明后文中需要使用的函数
# 声明一下后面需要使用的代码
# 选定监控的用户信息
def parseUid(user):
    # 鉴别UID
    try: 
        result = int (user)
    # 如果捕获到值错误
    except ValueError:
        try :
            # 返回指定用户在/etc/passwd中的信息
            user_info = pwd.getpwnam(user)
        except KeyError:
            raise argparse.ArgumentTypeError (
                    "{0!r} is not valid UID or user entry".format(user))
        else:
            return user_info.pw_uid
    else:
        # 如果数值UID小于0的可能
        return result

examples = """
    #参数还没定，暂时先空着
"""


# 关于参数解析的事件
parser = argparse.ArgumentParser(
        # 调用参数帮助信息前，打印的信息
        description="跟踪进程创建的工具",
        # 自定义帮助文档输出的类
        formatterClass=argparse.RawDescriptionHelpFormatter,
        # 帮助信息结尾处打印的信息
        epilog=examples
        )
parser.add_argument ("-T", "--time", action="store_true",help="计时" )





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

# cgroupMap的默认过滤函数
bpf = filter_by_containers(args)+bpf

# 如果需要观察源码，使用如下参数
if args.ebpf:
    print(bpf)
    exit()

execve_fnname = b.get_syscall_fnname("execve")
# 插入execve的起点
bpf.attach_kprobe(event=execve_fnname, fn_name="hookSyscallExecve")
# 插入execve的终点
bpf.attach_kretprobe(event=execve_fnname, fn_name="hookSyscallExecveRet")

# 打印显示的头部信息
if args.time:
    print("%-9s" % ("TIME"), end="")
if args.timestamp:
    print("%-8s" % ("TIME(s)"), end="")
if args.print_uid:
    print("%-6s" % ("UID"), end="")

print("%-16s %-7s %-7s %3s %s" % ("PCOMM", "PID", "PPID", "RET", "ARGS"))


class EventType(object):
    EVENT_ARG = 0
    EVENT_RET = 1

# 计算程序开始时间
start_ts = time.time()
#构建一个默认value为list的字典
argv = defaultdict(list)


# PPID匹配，短时进程可能会退出，在一些内核版本可能会出问题
# 这是从task0>real_parent->tgid获取PPID
def getPPID(pid):
    try:
        with open("/proc/%d/status" % pid) as status:
            for line in status:
                if line.startwith("PPid:"):
                    return int(line.split()[1])
    except IOError:
        pass
    return 0

# 进程事件和前端的交互
def printEvent(cpu, data, size):
    event = bpf["events"].event(data)
    skip = False

    if event.type == EventType.EVENT_ARG:
        # 默认value的字典
        argv[event.pid].append(event.argv)
    elif event.type == EventType.EVENT_RET:
        if event.retval != 0 and not args.fails:
            skip = True
        if args.name and not re.search(bytes(args.name), event.comm)
            skip = True
        if args.line and not re.search(bytes(args.line), b' '.join(argv[event.pid]))
            skip = True
        # 给预设的字典中，关于事件的进程号赋值
        if args.quote:
            argv[event.pid] = [
                    b"\"" + arg.replace(b"\"", b"\\\"") + b"\""
                    for arg in argv[event.pid]
                    ]

        #正常情况的主分支
        if not skip:
            if args.time:
                printb(b"%-9s" % strftime("%H:%M:%S").encode('ascii'))
            if args.timestamp:
                printb(b"%-8.3f" % (time.time() - start_ts))
            if args.print_uid
