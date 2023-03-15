from bcc import BPF

import os
import pymysql
from time import strftime
from datetime import datetime
import socket


# 过滤开关
UIDSwitch = False
uidFilter = -1

PPIDSwitch = False
ppidFilter = -1

# 获取链接MYSQL的环境变量信息
mysqlAddress = os.getenv('MYSQL_ADDRESS')
mysqlUser = os.getenv('MYSQL_USER')
mysqlPasswd = os.getenv('MYSQL_PASSWD')
mysqlDatabase = os.getenv("MYSQL_DATABASE")

# 链接MYSQL数据库
mysql = pymysql.connect(
    host=mysqlAddress,
    user=mysqlUser,
    passwd=mysqlPasswd,
    database=mysqlDatabase
)

# 设置MYSQL游标
cursor = mysql.cursor()
# 表名
mysqlTable = "UserCommand"
# 构建SQL语句
InsertCommand = "INSERT "

# bpf注入点信息
bpf_text = """ #include <uapi/linux/ptrace.h>
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
	char argv[ARGSIZE];	// 创建一个数组用来接收插桩点参数的内容
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
	events.perf_submit(ctx,data,sizeof(struct forkInfo));
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
	PPID_FILTER
	// 辅助函数，用来获取当前进程名称
	bpf_get_current_comm(&data.comm, sizeof(data.comm));
	data.type = EventArg;

	//读取写入参数
	__getArgs(ctx, (void*)filename, &data);
	//跳过了第一个参数，现在读取剩下的参数  如果循环次数是常数，编译器会直接展开
	#pragma unroll 
	for (int i = 1; i < MAXARG; ++i) 
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

// 在离开函数的时候，上报下性能事件和外界交互
int hookSyscallExecveRet(struct pt_regs* ctx)
{
    // 创建预设的数据结构,存储exec的参数并
    struct forkInfo data = {};
    // 创建结构体，获取当前进程的信息
    struct task_struct *task;

	u32 uid = bpf_get_current_uid_gid() & 0xffffffff;
    UID_FILTER
    data.pid = bpf_get_current_pid_tgid() >> 32;
    data.uid = uid;

    task = (struct task_struct *)bpf_get_current_task();
	data.ppid = task->real_parent->tgid;

	// 父进程ID过滤
    PPID_FILTER
	// 进程名获取
    bpf_get_current_comm(&data.comm, sizeof(data.comm));
    data.type = EventRet;

	// 获取返回值
    data.retval = PT_REGS_RC(ctx);
	//提交性能事件，开始和前端交互
    events.perf_submit(ctx, &data, sizeof(data));
    
	return 0;
}
"""
bpf_text = bpf_text.replace("MAXARG", "20")

if UIDSwitch:
    bpf_text = bpf_text.replace(
        "UID_FILTER", 'if (uid != %s) {return 0;}' % uidFilter)
else:
    bpf_text = bpf_text.replace("UID_FILTER", ' ')
# 如果设计了PPID过滤规则也添加到源码
if PPIDSwitch:
    bpf_text = bpf_text.replace(
        "PPID_FILTER", 'if (data.ppid != %s) {return 0;}' % ppidFilter)
else:
    bpf_text = bpf_text.replace("PPID_FILTER", ' ')

# 设置BPF注入点
b = BPF(text=bpf_text)
execve_fnname = b.get_syscall_fnname("execve")
b.attach_kprobe(event=execve_fnname, fn_name="hookSyscallExecve")
b.attach_kretprobe(event=execve_fnname, fn_name="hookSyscallExecveRet")

# 分辨返回值或者参数列表


class EventType(object):
    EVENT_ARG = 0
    EVENT_RET = 1


start_ts = datetime.now()


# 和性能事件交互的前端
def print_event(cpu, data, size):
	# 获取性能事件打印的参数
	event = b["events"].event(data)
	# 填充用户行为数据
	time=datetime.now().strftime("%Y-%m-%d %H:%M:%S")
	user = event.uid
	pid = event.pid
	ppid = event.ppid
	netns = event.netns
	comm = event.comm.decode('utf8')
	machine = socket.gethostname()
	if event.type == EventType.EVENT_ARG:
		argv = event.argv
		SQL = "INSERT INTO USER_COMMAND (TIME,USER,COMMAND,LEVEL,MACHINE,CONTAINER,UID,PID,PPID,ARGS) VALUES (str_to_date('\%s\','%%Y-%%m-%%d %%H:%%i:%%s.%%f'),'%s','%s','%s','%s','%s','%s','%s','%s','%s')" % (time, user, comm, "1", machine, netns, user, pid, ppid, argv)
		cursor.execute(SQL)
	elif event.type == EventType.EVENT_RET:
		retval = event.retval
		SQL = "INSERT INTO USER_COMMAND (TIME,USER,COMMAND,LEVEL,MACHINE,CONTAINER,UID,PID,PPID,RET) VALUES (str_to_date('\%s\','%%Y-%%m-%%d %%H:%%i:%%s.%%f'),'%s','%s','%s','%s','%s','%s','%s','%s','%s')" % (time, user, comm, "1", machine, netns, user, pid, ppid, retval)
		cursor.execute(SQL)
# 循环打印性能事件
b["events"].open_perf_buffer(print_event)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()

# 清理环境
# 关闭游标
cursor.close()
# 关闭数据库
mysql.close()
