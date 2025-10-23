#include "http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

// 从文件中读取 HTML 内容
static int read_file(const char *filename, char *buffer, size_t maxlen) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        snprintf(buffer, maxlen, "<h2>404 Not Found</h2><p>File %s missing.</p>", filename);
        return strlen(buffer);
    }
    size_t n = fread(buffer, 1, maxlen - 1, fp);
    buffer[n] = '\0';
    fclose(fp);
    return n;
}

// 提取请求路径
static void parse_path(struct conn_item *conn) {
    const char *req = conn->rbuffer;
    const char *p1 = strchr(req, ' ');
    if (!p1) return;
    const char *p2 = strchr(p1 + 1, ' ');
    if (!p2) return;

    size_t len = p2 - (p1 + 1);
    if (len >= sizeof(conn->path)) len = sizeof(conn->path) - 1;
    strncpy(conn->path, p1 + 1, len);
    conn->path[len] = '\0';
}

int http_request(struct conn_item *conn)
{
    parse_path(conn);
    printf("[HTTP] Request Path: %s\n", conn->path);
    return 0;
}

int http_response(struct conn_item *conn)
{
    char body[8192] = {0};

    // 如果请求根路径，加载 index.html
    if (strcmp(conn->path, "/") == 0 || strcmp(conn->path, "/index.html") == 0) {
        read_file("index.html", body, sizeof(body));
    }
    else if (strcmp(conn->path, "/click") == 0) {
        snprintf(body, sizeof(body),
            "<html><head><title>Clicked</title>"
            "<style>"
            "body{font-family:Segoe UI;text-align:center;background:linear-gradient(120deg,#6a11cb,#2575fc);color:white;}"
            ".card{margin-top:150px;padding:40px;background:rgba(255,255,255,0.1);border-radius:15px;backdrop-filter:blur(10px);}"
            "a{color:#fff;text-decoration:none;font-weight:bold;}"
            "a:hover{text-decoration:underline;}"
            "</style></head>"
            "<body><div class='card'><h1>🎉 You clicked the button!</h1>"
            "<p>This is a dynamic HTTP response from C++</p>"
            "<a href='/'>Back to Home</a></div></body></html>"
        );
    }
    else {
        snprintf(body, sizeof(body),
            "<html><body><h2>404 Not Found</h2><p>Path: %s</p></body></html>",
            conn->path
        );
    }

    int body_len = strlen(body);
    conn->wlen = snprintf(conn->wbuffer, BUFFER_LENGTH,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n%s",
        body_len, body
    );
    return conn->wlen;
}
