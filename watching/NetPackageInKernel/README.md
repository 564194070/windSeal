数据包内核态流程监控，主要知识脉络来自于《深入理解Linux网络一书》

1.从软中断开始监控 
    外设速度低于CPU,为了在必要的时候获得CPU，在外设和CPU之间提供了一种中断的机制，现代OS也来时钟驱动也需要中断支持。
    在中断到达CPU之前，首先会经过中断控制器，根据中断源的优先级对中断进行派发。多核系统中，中断控制器还需要根据CPU预先设定的规则，将某个中断送入指定CPU，实现中断负载均衡。
中断控制器控制相关数据结构 struct irq_chip 具体解析详见 kernelDemo/irq.h
中断控制器描述相关数据结构 struct irq_desc 具体解析详见 kernekDemo/irq.h




















参考文章:https://zhuanlan.zhihu.com/p/83709066

