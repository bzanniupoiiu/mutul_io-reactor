#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace std;


int main()
{
    
    int lfd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serveraddr.sin_port = htons(6666);
    if(bind(lfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr)))
    {
        perror("bind");
        return -1;
    }

    listen(lfd,10);

    int epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;

    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);

    struct epoll_event events[1024] = {0};

    printf("Server started, listening on port 6666...\n");

    while(1)
    {
        int nready = epoll_wait(epfd,events,1024,-1);

        int i = 0;
        for ( i = 0; i < nready; i++)
        {
            int fd = events[i].data.fd;
            if(fd == lfd)
            {
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);

                int clientfd = accept(lfd,(struct sockaddr*)&clientaddr,&len);

                ev.events=EPOLLIN|EPOLLET;
                ev.data.fd = clientfd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ev);
                printf("New client connected, fd=%d\n", clientfd);
            }
            else
            {
                char buf[5] = {0};
                size_t n = read(fd, buf, sizeof(buf) - 1);
                if (n <= 0) {
                    printf("Client disconnected: fd=%d\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    printf("Received from fd=%d: %s\n", fd, buf);
                    // 回显给客户端
                    write(fd, buf, n);
                }

            }
            

        }
        
    }



    
}