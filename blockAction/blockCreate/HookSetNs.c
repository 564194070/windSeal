#include <uapi/linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/bpf.h>
#include <unistd.h>
#include <linux/nsproxy.h>
#include <linux/fcntl.h>


#define TASK_COMM_LEN 64

#define ARGSIZE 64
#define TOTAL_MAX_ARGS 5
#define FULL_MAX_ARGS_ARR (TOTAL_MAX_ARGS * ARGSIZE)
#define LAST_ARG (FULL_MAX_ARGS_ARR - ARGSIZE)

void test(int pid);

struct SetNsProcess
{
    u32 pid;
    char comm[TASK_COMM_LEN];
    int fd;
    int type;
    char typeName[30];
};

BPF_PERF_OUTPUT(events);
BPF_HASH(tasks, u32, struct SetNsProcess);

TRACEPOINT_PROBE(syscalls, sys_enter_setns)
{
    //函数原型
    //SYSCALL_DEFINE2(setns, int, fd, int, flags)

    /*
        1.CLONE_NEWNS   0x00020000 131072
        2.CLONE_NEWUTS  0x04000000 67108864
        3.CLONE_NEWIPC  0x08000000 134217728
        4.CLONE_NEWNET  0x40000000 1073741824
        5.CLONE_NEWUSER 0x10000000 268435456
        6.CLONE_NEWPID  0x20000000 536870912
    */
	unsigned int ret = 0;

    struct SetNsProcess data = { };
    // 获取进程PID
    u32 pid = bpf_get_current_pid_tgid();
    data.pid = pid;
    
    bpf_get_current_comm(&data.comm, sizeof(data.comm));
    data.fd = args->fd;
    switch (args->nstype)
    {
    case 131072:
        memcpy(data.typeName,"CLONE_NEWNS",20);
        break;
    case 67108864:
        memcpy(data.typeName,"CLONE_NEWUTS",20);
        break;
    case 134217728:
        memcpy(data.typeName,"CLONE_NEWIPC",20);
        break;
    case 1073741824:
        memcpy(data.typeName,"CLONE_NEWNET",20);
        break;
    case 268435456:
        memcpy(data.typeName,"CLONE_NEWUSER",20);
        break;
    case 536870912:
        memcpy(data.typeName,"CLONE_NEWPID",20);
        break;
    default:
        memcpy(data.typeName,"ERROR",20); 
    }

	tasks.update(&pid, &data);
	return 0;
}

TRACEPOINT_PROBE(syscalls, sys_exit_setns)
{
	u32 pid = bpf_get_current_pid_tgid();
	struct SetNsProcess *data = tasks.lookup(&pid);
	if (data != NULL) {
		events.perf_submit(args, data, sizeof(struct SetNsProcess));
        
        struct task_struct *t = (struct task_struct *)bpf_get_current_task();
        bpf_send_signal(2);
		tasks.delete(&pid);
	}
	return 0;
}

