# webserver-websocket
这里包含一个简单的webserver和websocket程序
networkio.c里采用了四种方式：一请求一线程， select/poll/epoll 展示io多路复用
配合mul_port_client_epoll.c可以测试并发量

reactor.c 是网络io层，采用reactor模式
webserver是http协议层/应用层
websocket是允许长时间的双向通信的应用层
