-- 创建数据库 选择数据库
CREATE DATABASE RECORD;
USE RECORD;

-- 创建一张存储用户行为的表
-- 序号 系统时间 用户 命令 危险的等级 操作机器 操作容器 UID 进程 PID 线程 PPID 
CREATE TABLE `USER_COMMAND` (
    `INDEX`     INT(7)      NOT NULL AUTO_INCREMENT     COMMENT '自增ID,计数排序用',
    `TIME`      DATE        NOT NULL                    COMMENT '命令执行的时间',
    `USER`      CHAR(8)     NOT NULL                    COMMENT '执行命令的用户',
    `COMMAND`   VARCHAR(10) NOT NULL                    COMMENT '用户执行的命令',
    `LEVEL`     TINYINT     NOT NULL DEFAULT 1          COMMENT '执行命令的危险程度1最低正常指令,5最高严重危险',
    `MACHINE`   VARCHAR(10) NOT NULL                    COMMENT '执行命令的机器',
    `CONTAINER` VARCHAR(10) NOT NULL DEFAULT '容器外'    COMMENT '执行命令的容器',
    `UID`       VARCHAR(10) NOT NULL DEFAULT 0          COMMENT '用户ID',
    `PID`       VARCHAR(10) NOT NULL DEFAULT 0          COMMENT '进程ID',
    `PPID`      VARCHAR(10) NOT NULL DEFAULT 0          COMMENT '线程ID',
    PRIMARY KEY (`INDEX`)
) ENGINE=INNODB DEFAULT CHARSET = utf8;