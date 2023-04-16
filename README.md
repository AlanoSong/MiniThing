# MiniThing

#### 1. 介绍
- Windows平台的Everything文件查找速度非常快，优势在于利用了NTFS的USN日志，以及Windows上的文件监测机制
- 这个项目仿照类似原理，通过查询USN日志、监测Windows平台文件修改、使用SQLite数据库存储文件节点，并提供文件信息查询功能

#### 2. 如何使用
##### 2.1 编译
- Visual Studio：打开根目录下的MiniThing.sln，编译选择Build Solution，启动选择Local Windows Debugger
- CMake：待添加
##### 2.2 使用
- 目前仅支持命令行查找，程序启动后，输入要查询的盘符（eg."F:"）
- 第1次进入会建数据库，等待数据库建好之后，就能输入文件名进行查找
![](./Docs/Pictures/Use0.png)
- 盘符下的文件改动也会被软件捕获到
![](./Docs/Pictures/Use1.png)
- QT界面制作中...
- 正则查找等其他功能制作中...

#### 3. 编写计划
- [x] 实现基本的USN日志查询，建立初始的文件节点数据库，存放在unordered map中
- [x] 开启monitor thread，后台监测文件改动
- [x] 研究unicode规范，解决代码中宽字符的处理问题
- [ ] 添加开源的日志打印，或者自写轻量级的打印宏
- [x] 研究sqlite接口，将数据库完全port到sqlite中
- [x] 开启query thread，实现命令行查询
- [x] 用thread pool加速数据库初始化
- [ ] 基本bug清掉，代码结构优化
- [ ] 调查MFC、QT，选一个实现ui界面
- [ ] 整理输出相关文档

#### 4. 性能优化
##### 4.1 测试环境：
- CPU：i5-7200U，内存：8G，F盘（包含文件*5895，文件夹*96）
##### 4.2 初始性能：
- 建立数据库`183.216 S`，单次查询`0.0044486 S`
##### 4.3 Step 1：先用unordered_map建立数据库，然后转存到sqlite进行查询
- 建立数据库`44.4971 S`，单次查询`0.0018933 S`
##### 4.4 Step 2：建立数据库过程，采用mutiple thread加速
- 建立数据库`57.8045 S`，单次查询`0.0066217 S`
- 这1步建立数据库的速度，理论上应该比Step 2更快
- 但是在sub thread返回结果时，采用vector，又在存储sqlite时访问vector，比之前访问unordered map速度慢
- 所以瓶颈卡在vector访问上
##### 4.5 Step 3：建立数据库后，采用多线程向sqlite写入
- 待实现

#### 5. 已知bugs
- query thread没有设置退出消息

#### 6. 参与贡献
- Fork 本仓库
- 新建 Feat_xxx 分支
- 提交代码
- 新建 Pull Request
