
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>


void *client_thread(void *arg)
{
    int clientfd = *(int*)arg;

    while (1)
    {
        char buffer[1024] = {0};
        int count = recv(clientfd, buffer, 1024, 0);
        if (count == 0) //返回0 断开
        {
            printf("client: disconnect: %d\n", clientfd);
            close(clientfd);
            break;
        }
        printf("RECV: %s\n", buffer);
        
        count = send(clientfd, buffer, count, 0);
        printf("SEND SUCCEEDED: %d bytes\n", count);   
    }
    
}



int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr; //服务器配置
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(2000); //0-1023为系统默认
    if (-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)))
    {
        printf("bind faild: %s\n", strerror(errno));
    }
    listen(sockfd, 10);

    printf("listen finished: %d\n", sockfd);
    
    struct sockaddr_in clientaddr; //客户端配置
    socklen_t len = sizeof(clientaddr);


#if 0
    int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

    char buffer[1024] = {0};
    int count = recv(clientfd, buffer, 1024, 0);

    printf("RECV: %s\n", buffer);
    
    count = send(clientfd, buffer, count, 0);
    printf("SEND SUCCEEDED: %d bytes\n", count);

#elif 0

    while(1)
    {
        int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

        char buffer[1024] = {0};
        int count = recv(clientfd, buffer, 1024, 0);

        printf("RECV: %s\n", buffer);
        
        count = send(clientfd, buffer, count, 0);
        printf("SEND SUCCEEDED: %d bytes\n", count);
    }

#elif 0
    while(1)
    {
        int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
        printf("accept finished: %d\n", clientfd);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &clientfd);
        
    }


#elif 0
    
    fd_set rfds, rset;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    int maxfd = sockfd;

    while (1)
    {
        printf("\n");
        rset = rfds;

        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL); // 返回可读总数
        printf("nready: %d\n", nready);

        if (FD_ISSET(sockfd, &rset))
        {
            int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
            printf("accept finished: %d\n", clientfd);

            FD_SET(clientfd, &rfds);

            if (clientfd > maxfd)   maxfd = clientfd;
        }
        printf("maxfd: %d\n", maxfd);
        //recv
        int i = 0;
        for (i = sockfd + 1; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &rset))
            {   
                printf("now clientfd: %d\n", i);
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0) //返回0 断开
                {
                    printf("client: disconnect: %d\n", i);
                    close(i);
                    FD_CLR(i, &rfds);
                    continue;
                }
                printf("RECV: %s\n", buffer);
                
                count = send(i, buffer, count, 0);
                printf("SEND SUCCEEDED: %d bytes\n", count);
            }
        }
    }

#elif 0

    struct pollfd fds[1024] = {0};

    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLLIN;
    
    int maxfd = sockfd;

    while (1)
    {
        int nready = poll(fds, maxfd + 1, -1);
        
        if (fds[sockfd].revents & POLLIN) // 若sockfd可读
        {

            int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
            printf("accept finished: %d\n", clientfd);

            fds[clientfd].fd = clientfd;
            fds[clientfd].events = POLLIN;

            if (clientfd > maxfd)   maxfd = clientfd;

        }

        int i = 0;
        for (i = sockfd + 1; i <= maxfd; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                char buffer[1024] = {0};

                int count = recv(i, buffer, 1024, 0);
                if (count == 0) //返回0 断开
                {
                    printf("client: disconnect: %d\n", i);
                    close(i);
                    
                    fds[i].fd = -1;
                    fds[i].events = 0;

                    continue;
                }
                printf("RECV: %s\n", buffer);
                
                count = send(i, buffer, count, 0);
                printf("SEND SUCCEEDED: %d bytes\n", count);

            }
        }
    }


#else 

    int epfd = epoll_create(1);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1)
    {
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);

        int i = 0;
        for (i = 0; i < nready; i++)
        {
            int connfd = events[i].data.fd;
            
            if (connfd = sockfd)
            {
                int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
                printf("accept finished: %d\n", clientfd);

                ev.events = EPOLLIN;
                ev.data.fd = clientfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

            }
            else if (events[i].events & EPOLLIN)
            {
                char buffer[1024] = {0};

                int count = recv(connfd, buffer, 1024, 0);
                if (count == 0) //返回0 断开
                {
                    printf("client: disconnect: %d\n", connfd);
                    close(connfd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, &ev);

                    continue;
                }
                printf("RECV: %s\n", buffer);
                
                count = send(connfd, buffer, count, 0);
                printf("SEND SUCCEEDED: %d bytes\n", count);

            }
        }
    }
    
#endif

    getchar();

    printf("exit\n");

    return 0;
}


