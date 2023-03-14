from bcc import BPF

import os
import pymysql 

# 获取链接MYSQL的环境变量信息
mysqlAddress = os.getenv('MYSQL_ADDRESS')
mysqlUser = os.getenv('MYSQL_USER')
mysqlPasswd = os.getenv('MYSQL_PASSWD')
mysqlDatabase = os.getenv("MYSQL_DATABASE")

# 链接MYSQL数据库
mysql = pymysql.connect (
    host = mysqlAddress,
    user = mysqlUser,
    passwd = mysqlPasswd,
    database = mysqlDatabase,
    charset = "utf-8"
)

# 设置MYSQL游标
cursor = mysql.cursor()
# 表名
mysqlTable = "UserCommand"
# 构建SQL语句
InsertCommand = "INSERT "



# 清理环境
# 关闭游标
cursor.close()
# 关闭数据库
mysql.close()