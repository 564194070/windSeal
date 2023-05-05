from bcc import BPF


xdp_program = """
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>


/* 
struct ethhdr {
    unsigned char h_dest[ETH_ALEN];         // 目标 MAC 地址
    unsigned char h_source[ETH_ALEN];       // 源 MAC 地址
    __be16 h_proto;                         // 上层协议类型 大端字节序
};
*/

int xdpFilter (struct xdp_md * ctx) 
{
    void *data_end = (void*)(long)ctx->data_end;
    void *data = (void*)(long)ctx->data;

    //以太网数据帧头部结构体，用于描述一个以太网数据帧的各个字段
    struct ethhdr *eth = data;
    // 避免处理不完整的数据包，
    // 以太网帧头结束位置的下一个位置的下一个地址，大于数据区域的末尾地址，数据包不完整无法满足要求
    if (eth + 1 > data_end) 
    {
        return XDP_PASS;
    } 

    //获取IP头部
    struct iphdr *ip = data + sizeof(struct ethhdr);
    if (ip + 1 > data_end) 
    {
        return XDP_DROP;
    }

    // 开始匹配
    __be32 dst_ip = 0x12345678;
    if (ip->daddr == dst_ip) 
    {
        return XDP_PASS;
    }

    // 获取TCP头部信息
    struct tcphdr *tcp = data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    if (tcp + 1 > data_end)
    {
        return XDP_PASS;
    } 

    // 判断目标端口是否匹配
    __be16 dst_port = 80;
    if (tcp->dest == dst_port)
    {
        return XDP_DROP;
    }

    return XDP_PASS;
}
"""

IPSwitch = True
PortSwitch = True

IPFilter = "0x12345678"
PortFilter = "80"
NetDevice = "ens33"


# 设置XDP的IP过滤信息
if IPSwitch:
    bpf_text = xdp_program.replace("IP_FILTER", '%s' % IPFilter)
else:
    bpf_text = xdp_program.replace("IP_FILTER", ' ')

# 
if PortSwitch:
    bpf_text = bpf_text.replace("PORT_FILTER", '%s' % PortFilter)
else:
    bpf_text = bpf_text.replace("PORT_FILTER", ' ')


b = BPF(text=xdp_program)
fn = b.load_func("xdpFilter", BPF.XDP)
b.attach_xdp(NetDevice, fn, 0)


print("XDP filter attached to %s" % NetDevice)

# 等待XDP程序运行
while True:
    try:
        b.trace_print()
    except KeyboardInterrupt:
        break

# 在程序退出前从网络设备移除XDP程序
b.remove_xdp(NetDevice, 0)