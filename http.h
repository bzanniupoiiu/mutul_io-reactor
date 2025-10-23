#pragma once
#define BUFFER_LENGTH 2048

typedef int (*RCALLBACK)(int fd);

struct conn_item {
    int fd;
    char rbuffer[BUFFER_LENGTH];
    int rlen;
    char wbuffer[BUFFER_LENGTH];
    int wlen;
    char path[128]; // 新增：保存请求路径

    RCALLBACK accept_callback;
    RCALLBACK recv_callback;
    RCALLBACK send_callback;
};

int http_request(struct conn_item *conn);
int http_response(struct conn_item *conn);
