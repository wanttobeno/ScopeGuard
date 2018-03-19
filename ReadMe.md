
#ScopeGuard

代码提取自watchman

[watchman Github](https://github.com/facebook/watchman/)

####  需要VS2015及以后版本

--
### 用途
ScopeGuard 用于资源的安全删除。

--
### ScopeGuard的原理介绍
2000年的一编文章
[Change the Way You Write Exception-Safe Code]
(http://www.drdobbs.com/cpp/generic-change-the-way-you-write-excepti/184403758)

--
###ScopeGuard用法：

直接使用宏  SCOPE_EXIT，申请资源后，在后面跟上SCOPE_EXIT删除代码，就不会忘记释放或者重复释放了。

![ScreenShot](ScreenShot.png)

















