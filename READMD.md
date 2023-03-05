第三方依赖

crc32 校验和
skiplist 无锁跳表
snappy 压缩算法
spdlog 高效日志库
gtest 单元测试框架
benchmark 基准性能测试

编译+运行

bazel build //src... 编译源码
bazel buiild //test... 编译测试

接口demo


基于level 策略lsm-t实现的db，支持kv接口

feater
通过yacc实现 mysql 协议扩展

