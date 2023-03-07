#include <uapi/linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>

#define ARGSIZE 128

// 抽象出来一个进程的描述信息
struct forkInfo {
	u32 pid;			// 用户空间的进程ID
	u32 ppid;			// 父进程ID
	u32 uid;			// 创建进程的用户ID
	char comm[50];		// 进程名称
	char argv[ARGSIZE];	// 创建一个数组用来接收插桩点参数的内容
	int retval;			// 记录插桩点的返回值
}

// 创建一个性能事件，在触发该事件的时时候发生前后端交互行为
BPF_PERF_OUTPUT(events);

// pt_regs BPF现场和上下文寄存器
static int getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data)
{
	const char *argp = NULL;
	// 辅助函数，从用户空间读取数据到变量
	bpf_probe_read_user(&argp, sizeof(argp), ptr);
	// 如果ptr没有数据复制给argp
	if (argp) 
	{
		return __getArgs(ctx, (void*)(argp), data)
	}
	return 0;
}

static int __getArgs(struct pt_regs *ctx, void *ptr, struct forkInfo *data)
{
	bpf_probe_read_user(data->argv, sizeof(data->argv), ptr);
	event.perf_submit(ctx,data,sizeof(struct forkInfo);
	return 1;
}

int hookSyscallExecve(struct pt_regs *ctx, const char * filename, const char *const * argv, const char *const * envp)
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

	// 过滤模板，在正式编译前进行替换	
	UID_FILTER

	if (container_should_be_filtered()) 
	{
		return 0;	
	}

	// 创建预设的数据结构，存储exec的参数并
	struct forkInfo data = {};
	// 创建结构体，获取当前进程的信息
	struct task_struct *task;
	
	data.ppid = pid;
	task = 

}
