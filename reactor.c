

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include "server.h"



#define CONNECTION_SIZE 1048576 //1024*1024

#define MAX_PORTS       20
#define TIME_SUB_MS(tv1, tv2)   ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000);

int epfd = 0;
struct timeval begin = {0};



struct conn conn_list[CONNECTION_SIZE] = {0};


int set_event(int fd, int event, int flag)
{
    if (flag)
    {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else
    {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }

}

int set_event(int fd, int event, int flag);
int event_register(int fd, int event);
int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);
int init_server(unsigned short port);


int main()
{

    unsigned short port = 2000;
    

    epfd = epoll_create(1); //epfd: 4

    int i = 0;
    for (i = 0; i < MAX_PORTS; i++)
    {
        int sockfd = init_server(port + i);
        conn_list[sockfd].fd = sockfd;
        conn_list[sockfd].r_action.recv_callback = accept_cb;

        set_event(sockfd, EPOLLIN, 1);
    }
    
    gettimeofday(&begin, NULL);
    while (1) // mainloop
    {
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);

        int i = 0;
        for (i = 0; i < nready; i++)
        {
            int connfd = events[i].data.fd;

            if (events[i].events & EPOLLIN)
            {
                conn_list[connfd].r_action.recv_callback(connfd);
            }

            if (events[i].events & EPOLLOUT)
            {
                conn_list[connfd].send_callback(connfd);
            }
        }
    }
    //close(sockfd);

    return 0;
}





int event_register(int fd, int event)
{
    if (fd < 0)
    {
        return -1;
    }
    conn_list[fd].fd = fd;
    conn_list[fd].r_action.recv_callback = recv_cb;
    conn_list[fd].send_callback = send_cb;


    memset(conn_list[fd].rbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].rlength = 0;
    memset(conn_list[fd].wbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].wlength = 0;


    set_event(fd, event, 1);
    return 0;
}


// listen(sockfd, ?) --> EPOLLIN --> accept_callback
int accept_cb(int fd)
{
    struct sockaddr_in clientaddr; //客户端配置
    socklen_t len = sizeof(clientaddr);

    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
    //printf("accept finished: %d\n", clientfd);
    // if (clientfd < 0) 
    // {
    //     return -1;
    // }
    event_register(clientfd, EPOLLIN);
    if (clientfd % 1000 == 0)
    {
        struct timeval current;
        gettimeofday(&current, NULL);

        int time_used = TIME_SUB_MS(current, begin);
        memcpy(&begin, &current, sizeof(struct timeval));

        printf("accept finished: %d, time used: %d\n", clientfd, time_used);
    }
    return 0;
}

int recv_cb(int fd)
{   
    
    int count = recv(fd, conn_list[fd].rbuffer, BUFFER_LENGTH, 0);
    if (count == 0) //返回0 断开
    {
        printf("client: disconnect: %d\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // unfinished

        return 0;
    }
    else if (count < 0)
    {
        printf("count: %d, errno: %d, %s\n", count, errno, strerror(errno));
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); 

        return 0;
    }

    conn_list[fd].rlength = count;
    //printf("RECV: %s\n", conn_list[fd].rbuffer);

#if 0

    conn_list[fd].wlength = conn_list[fd].rlength;
    memcpy(conn_list[fd].wbuffer, conn_list[fd].rbuffer, conn_list[fd].wlength);
    

#elif 0

    http_request(&conn_list[fd]);

#else 

    ws_request(&conn_list[fd]);

#endif


    set_event(fd, EPOLLOUT, 0);

    return count;
}

int send_cb(int fd)
{

#if 0

    http_response(&conn_list[fd]);

#else

    ws_response(&conn_list[fd]);

#endif


    int count = 0;

#if 0
    if (conn_list[fd].status == 1)
    {
        count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
        set_event(fd, EPOLLOUT, 0);
    }
    else if (conn_list[fd].status == 2)
    {
        set_event(fd, EPOLLOUT, 0);
    }
    else
    {
        if (conn_list[fd].wlength != 0) {
			count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
		}

        set_event(fd, EPOLLIN, 0);
    }

#else

    if (conn_list[fd].wlength != 0) {
		count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
	}
	
	set_event(fd, EPOLLIN, 0);

#endif
    return count;
}

int init_server(unsigned short port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr; //服务器配置
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port); //0-1023为系统默认
    if (-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)))
    {
        printf("bind faild: %s\n", strerror(errno));
    }
    listen(sockfd, 10);

    //printf("listen finished: %d\n", sockfd);
    
    return sockfd;
}
