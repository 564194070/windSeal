from bcc import BPF
from ctypes import c_char_p
import os
from collections import defaultdict


bpf_text = """ 
#include <uapi/linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/nsproxy.h>
#include <net/net_namespace.h>
#define ARGSIZE 128
enum eventType
{
	EventArg,
	EventRet,
};
// 抽象出来一个进程的描述信息
struct forkInfo {
	u32 pid;			// 用户空间的进程ID
	u32 ppid;			// 父进程ID
	u32 uid;			// 创建进程的用户ID
    u32 netns;         // cgroup名称空间信息,用来分辨指令所在的容器
	char comm[50];		// 进程名称
	char argv[100];	// 创建一个数组用来接收插桩点参数的内容
	int retval;			// 记录插桩点的返回值
	enum eventType type;// 记录事件的类型
};
// 创建一个性能事件，在触发该事件的时时候发生前后端交互行为
BPF_PERF_OUTPUT(events);
static int __getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data);
static int getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data);
// pt_regs BPF现场和上下文寄存器
static int getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data)
{
	const char *argp = NULL;
	// 辅助函数，从用户空间读取数据到变量
	bpf_probe_read_user(&argp, sizeof(argp), ptr);
	// 如果ptr没有数据复制给argp
	if (argp) 
	{
        
		return __getArgs(ctx, (void*)(argp), data);
	}
	return 0;
}
static int __getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data)
{
	bpf_probe_read_user(data->argv, sizeof(data->argv), ptr);
    bpf_trace_printk("execve: filename=%s\\n", data->argv);
	events.perf_submit(ctx,data,sizeof(struct forkInfo));
	return 1;
}
int syscall__(struct pt_regs *ctx, const char *filename,
                const char *const __user *argv,
                const char *const __user *const __user *envp)
{
	/*
	 * 内核插桩点 tracepoint:syscalls:sys_enter_execve 在父进程中创建一个子进程,自己成调用exec启动新的程序(执行程序函数)
	 * 1.filename 二进制可执行文件或脚本
	 * 2.argv     调用程序执行的参数序列
	 * 3.envp     键值对作为新程序的环境变量
	*/
	// uid 用户ID
	u32 uid = bpf_get_current_uid_gid() & 0xffffffff;
	// pid_tgid 线程ID
	u64 pid_tgid = bpf_get_current_pid_tgid();
	// tid 线程ID 线程唯一
	u32 tid = (u32)pid_tgid;
	// pid 进程ID
	u32 pid = pid_tgid >> 32;
	// 创建预设的数据结构,存储exec的参数并
	struct forkInfo data = {};
	// 创建结构体，获取当前进程的信息
	struct task_struct *task;
	
	data.pid = pid;
	// 辅助函数 获取任务(进程)的task结构体
	task = (struct task_struct *)bpf_get_current_task();
	// 从task里面拿父进程信息
	data.ppid = task->real_parent->tgid;
	// 获取进程所在的cgroup空间
    data.netns = task->nsproxy->net_ns->ns.inum;
    
	// 父进程过滤信息
	// 辅助函数，用来获取当前进程名称
	bpf_get_current_comm(&data.comm, sizeof(data.comm));
	data.type = EventArg;
	//读取写入参数
    bpf_trace_printk("1");
    bpf_trace_printk("filename: filename=%s\\n",filename);
	__getArgs(ctx, (void*)filename, &data);
    bpf_trace_printk("filename: filenameargv=%s\\n",data.argv);
	//跳过了第一个参数，现在读取剩下的参数  如果循环次数是常数，编译器会直接展开
	#pragma unroll 
	for (int i = 1; i < 30; ++i) 
	{
		if (getArgs(ctx, (void*)&argv[i], &data) == 0) 
		{
			goto out;
		}
	}
    
	// 如果参数太多，超过了最大限额，采取截断方式处理
	char ellipsis[] = "...";
	__getArgs(ctx, (void*)ellipsis, &data);
out:
	return 0;
}
"""

# 设置BPF注入点
b = BPF(text=bpf_text)
execve_fnname = b.get_syscall_fnname("execve")
# 不是这个就不能读入参了，现阶段原理暂不明细
print(execve_fnname)
b.attach_kprobe(event="__x64_sys_execve", fn_name="syscall__")


argv = defaultdict(list)

def print_event(cpu, data, size):
    event = b["events"].event(data)
    print(event.argv)
    argv[event.pid].append(event.argv)
    argv_text = b' '.join(argv[event.pid]).replace(b'\n', b'\\n')
    argv_text = argv_text.decode('utf8')
    #print(argv_text)
    b.trace_print()

b["events"].open_perf_buffer(print_event)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()