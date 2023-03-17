from bcc import BPF


bpf = """
    EBPF_COMMENT
    #include<linux/sched.h>
    BPF_STATIC_ASSERT_DEF

    struct forkInfo {
        u64 startTime;
        u64 exitTime;
        u32 pid;
        u32 tid;
        u32 ppid;
        u32 signalInfo;
        int exitCode;
        char task[TASK_COMM_LEN];
    }

"""