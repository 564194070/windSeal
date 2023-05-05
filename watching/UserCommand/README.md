recordUserCommand的初衷是为了记录用户在容器内外的操作，并且阻止危险和可疑行为
第一步设计数据库，存储用户的行为新消息
第二步设置BPF代码，插桩检测用户行为
第三步设计前端，和数据库交互，存储用户信息便于查询历史

首先捡起来八百年都不用的数据库Mysql，设计一个存储用户信息的表，表单的格式如下:
用户UID 系统时间 操作机器 操作的容器 操作的命令 危险级别
建表语句详见UserCommand.sql

史诗级离大谱：
mysql 容器化部署后死活连接不上失败，报错
ERROR 2013 (HY000): Lost connection to MySQL server at 'reading initial communication packet', system error: 2
重启docker后就好了，莫名其妙的