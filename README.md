# MiniThing

#### 介绍
- Windows平台的Everything软件查找速度非常快，本质是利用了NTFS文件系统的USN日志，以及Windows平台的文件检测功能
- 本软件按照相同原理，通过查询USN日志，检测Windows平台文件修改，使用sqlite存储文件信息，实现了类似的功能

#### 编写计划
- [x] 实现基本的USN日志查询，建立初始的sqlite数据库
- [x] 开启monitor thread，检测系统文件改动
- [x] （2周）（2023/04/09）研究Unicode原理，彻底解决宽字符的打印、与常规字符转换、sqlite的存取问题（找找有没有相关的开源轮子）
- [ ] （1周）找个开源的日志打印轮子
- [ ] （2周）研究sqlite接口，将当前存储在map中的文件信息，完全port到sqlite中
- [ ] （1周）开启query thread，实现一个基本的查询功能
- [ ] （4周）基本bug清掉，常规操作流程排除bug
- [ ] （4周）调查MFC、QT，选一个实现查询界面
- [ ] （2周）整理输出相关文档

#### 参与贡献
1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
