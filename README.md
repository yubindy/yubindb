# yubindb

## 第三方依赖

* crc32 校验和
* skiplist 无锁跳表
* snappy 压缩算法
* spdlog 高效日志库
* gtest 单元测试框架
* benchmark 基准性能测试

## 主要设计
* 基于 Level 策略 Lsm-Tree 实现的单机 KV 存储。
* 实现writebatch通过批量写入 WAL 预写日志实现数据恢复,减少同步竞争和i/o次数；
* 实现基于时间戳的快照，在键值存储的整体状态上提供了一致的只读视图；
* MemTable 中采用无锁跳表实现高效查找和遍历；
* 使用 LRU 实现缓存淘汰策略，维护系统缓存；
* 采用布隆过滤器，避免缓存穿透；通过写时复制避免 string 多次拷贝；
* sstable 中基于 snappy 进行数据压缩;
* 对于排序的 Key 进行前缀压缩，缓解写放大；
* sstable 由后台线程异步合并，减少数据冗余；
* 使用 GTest 编写单元测试并通过 benchmark 进行性能测试。

## 编译+运行

* bazel build //src... 编译源码
* bazel buiild //test... 编译测试

## 接口
* 支持kv接口
* 后续考虑 通过yacc实现 mysql 协议扩展

