#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "http.h"

using namespace std;

#define MAX_EVENTS 1024
#define SERVER_PORT 8080

struct conn_item connlist[1024];
int epfd = -1;


//==================================================================
// 函数声明
int accept_callback(int fd);
int recv_callback(int fd);
int send_callback(int fd);
int set_event(int fd, int event, int op);

//==================================================================


// =======================================================
// 辅助函数
int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int set_event(int fd, int event, int op) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, op, fd, &ev) < 0) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

// =======================================================
// 连接建立
int accept_callback(int fd) {
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
    if (clientfd < 0) {
        perror("accept");
        return -1;
    }

    set_nonblock(clientfd);

    auto &conn = connlist[clientfd];
    conn.fd = clientfd;
    conn.rlen = conn.wlen = 0;
    memset(conn.rbuffer, 0, BUFFER_LENGTH);
    memset(conn.wbuffer, 0, BUFFER_LENGTH);
    conn.recv_callback = recv_callback;
    conn.send_callback = send_callback;

    set_event(clientfd, EPOLLIN | EPOLLET, EPOLL_CTL_ADD);

    printf("[+] Client connected: fd=%d\n", clientfd);
    return clientfd;
}

// =======================================================
// 读取 HTTP 请求
int recv_callback(int fd) {
    auto &conn = connlist[fd];
    ssize_t n = recv(fd, conn.rbuffer + conn.rlen, BUFFER_LENGTH - conn.rlen, 0);

    if (n == 0) {
        printf("[-] Client disconnected: fd=%d\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        return -1;
    } else if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        perror("recv");
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        return -1;
    }

    conn.rlen += n;
    conn.rbuffer[conn.rlen] = '\0';

    printf("[>] HTTP request received from fd=%d:\n%s\n", fd, conn.rbuffer);

    // 调用 HTTP 解析逻辑
    int ret = http_request(&conn);
    if (ret == 0) {
        // 切换为写事件，准备发送响应
        set_event(fd, EPOLLOUT | EPOLLET, EPOLL_CTL_MOD);
    }
    return n;
}

// =======================================================
// 写回 HTTP 响应
int send_callback(int fd) {
    auto &conn = connlist[fd];

    // 让 http.cpp 负责生成响应内容（渲染页面）
    http_response(&conn);

    ssize_t n = send(fd, conn.wbuffer, conn.wlen, 0);
    if (n > 0)
        printf("[<] Sent %zd bytes to fd=%d\n", n, fd);

    close(fd);
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    printf("[-] Connection closed: fd=%d\n", fd);
    return 0;
}



// =======================================================
int main() {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblock(lfd);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);
    bind(lfd, (sockaddr*)&addr, sizeof(addr));
    listen(lfd, 10);

    epfd = epoll_create1(0);
    connlist[lfd].fd = lfd;
    connlist[lfd].accept_callback = accept_callback;
    set_event(lfd, EPOLLIN, EPOLL_CTL_ADD);

    printf("🚀 Server started on port %d\n", SERVER_PORT);

    epoll_event events[MAX_EVENTS];
    while (1) {
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready < 0 && errno != EINTR) break;

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            if (fd == lfd)
                connlist[lfd].accept_callback(fd);
            else if (events[i].events & EPOLLIN)
                connlist[fd].recv_callback(fd);
            else if (events[i].events & EPOLLOUT)
                connlist[fd].send_callback(fd);
        }
    }

    close(lfd);
    close(epfd);
    return 0;
}
