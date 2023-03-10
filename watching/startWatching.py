from bcc import BPF
import argparse

# 引入需要的前端框架

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


# 关于参数解析的事件
parser = argparse.ArgumentParser(
        # 调用参数帮助信息前，打印的信息
        description="Trace exec() syscalls",
        # 自定义帮助文档输出的类
        formatterClass=argparse.RawDescriptionHelpFormatter,
        # 帮助信息结尾处打印的信息
        epilog=examples
        )





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




