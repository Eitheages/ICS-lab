/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include <stdarg.h>
#include "csapp.h"

typedef struct sockaddr SA;
typedef struct {
    int fd;
    struct sockaddr_storage addr;
} client_info_t;
/**
 * Function prototypes
 */
int parse_uri(char* uri, char* target_addr, char* path, char* port);
void format_log_entry(char* logstring, struct sockaddr_in* sockaddr, char* uri,
                      size_t size);
void sigint_handler(int);
void sigpipe_handler(int);
int doit(client_info_t*);
void* thread(void*);

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char** argv) {
    int listen_fd;
    socklen_t client_len;
    pthread_t tid;

    Signal(SIGINT, &sigint_handler);
    Signal(SIGPIPE, &sigpipe_handler);
    Signal(SIGALRM, &sigpipe_handler);
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(1);
    }

    listen_fd = Open_listenfd(argv[1]);
    while (1) {
        client_len = sizeof(struct sockaddr_storage);
        client_info_t* client_info =
            (client_info_t*)malloc(sizeof(client_info_t));

        client_info->fd =
            accept(listen_fd, (SA*)&client_info->addr, &client_len);
        if (client_info->fd < 0) {
            fprintf(stderr, "Cannot open connect fd!");
            continue;
        }

        pthread_create(&tid, NULL, thread, client_info);
    }
    return 0;
}

void* thread(void* vargp) {
    pthread_detach(pthread_self());
    client_info_t client_info = *(client_info_t*)vargp;
    free(vargp);

    doit(&client_info);
    close(client_info.fd);

    return NULL;
}

/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char* uri, char* hostname, char* pathname, char* port) {
    char* hostbegin;
    char* hostend;
    char* pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char* p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    } else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char* logstring, struct sockaddr_in* sockaddr, char* uri,
                      size_t size) {
    time_t now;
    char time_str[MAXLINE];
    char host[INET_ADDRSTRLEN];

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    if (inet_ntop(AF_INET, &sockaddr->sin_addr, host, sizeof(host)) == NULL)
        unix_error("Convert sockaddr_in to string representation failed\n");

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %s %s %zu", time_str, host, uri, size);
}

void sigint_handler(int sig) {
    Sio_puts("Proxy closed by SIGINT.\n");
    _exit(0);
}

void sigpipe_handler(int sig) {}

void unix_error_w(char* msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
ssize_t Rio_readlineb_w(rio_t* rp, void* usrbuf, size_t maxlen) {
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        unix_error_w("Rio_readlineb_w error");
        return 0;
    }
    return rc;
}

void Rio_writen_w(int fd, void* usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        unix_error_w("Rio_writen_w error");
    }
}

ssize_t Rio_readnb_w(rio_t* rp, void* usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        unix_error_w("Rio_readnb_w error");
        return 0;
    }
    return rc;
}

typedef struct {
    char hostname[MAXLINE];
    char port[MAXLINE];
    char uri[MAXLINE];
} url;

ssize_t search_content_len(const char* header) {
    const char* s = strstr(header, "Content-Length: ");
    if (s == NULL) {
        return -1;
    }
    s = header + 16;
    return strtol(s, NULL, 0);
}

// Returns -1 as failure
int build_header(char* dst, rio_t* rp, ssize_t* content_len) {
    dst[0] = 0;
    ssize_t read_n;
    char buf[MAXLINE];
    *content_len = -1;
    // read the request header
    while ((read_n = Rio_readlineb_w(rp, buf, MAXLINE)) &&
           strcmp(buf, "\r\n")) {
        char* line_end = strstr(buf, "\r\n");
        if (line_end == NULL) {
            return -1;
        }
        if (*content_len == -1) {
            *content_len = search_content_len(buf);
        }
        sprintf(dst, "%s%s", dst, buf);
    }
    if (read_n < 0) {
        return -1;
    }
    sprintf(dst, "%s\r\n", dst);
    return 0;
}

int doit(client_info_t* client_info) {
    ssize_t write_n;
    char read_buf[MAXBUF + 1], write_buf[2 * MAXBUF];
    char logstring[MAXLINE];

    url* url_p = NULL;
    struct sockaddr_in* sockaddr = NULL;
    int success = 0;
    int server_fd = -1;
    int client_fd = client_info->fd;

    rio_t rio_client;
    Rio_readinitb(&rio_client, client_fd);

    // read the request line
    Rio_readlineb_w(&rio_client, read_buf, MAXLINE);

    char method[20], uri[MAXLINE], version[20];
    sscanf(read_buf, "%s %s %s", method, uri, version);
    if (strlen(method) == 0 || strlen(uri) == 0 || strlen(version) == 0) {
        goto END;
    }
    url_p = (url*)Malloc(sizeof(url));
    url_p->uri[0] = '/';
    if (parse_uri(uri, url_p->hostname, url_p->uri + 1, url_p->port) == -1) {
        goto END;
    }
    // Connect and write
    server_fd = open_clientfd(url_p->hostname, url_p->port);
    if (server_fd < 0) {
        goto END;
    }

    // Request line
    ssize_t request_body_len;
    char* write_buf_ptr = write_buf;
    write_buf[0] = 0;
    write_buf_ptr +=
        sprintf(write_buf, "%s %s %s\r\n", method, url_p->uri, version);
    // Request header
    if (build_header(write_buf_ptr, &rio_client, &request_body_len) == -1) {
        goto END;
    }
    write_n = strlen(write_buf);
    Rio_writen_w(server_fd, write_buf, write_n);

    // Request body (if needed)
    if (!strcasecmp(method, "POST")) {
        if (request_body_len == -1) {
            goto END;
        }
        while (request_body_len > 0) {
            ssize_t n;
            n = MAXLINE > request_body_len ? request_body_len : MAXLINE;
            n = Rio_readnb_w(&rio_client, write_buf, n);
            if (n == 0) {
                // Meet EOF
                goto END;
            }
            Rio_writen_w(server_fd, write_buf, n);
            request_body_len -= n;
        }
    }

    rio_t rio_server;
    Rio_readinitb(&rio_server, server_fd);
    size_t response_len = 0;    // sum length of the response
    ssize_t response_body_len;  // Content-Length, i.e.

    // Response line
    Rio_readlineb_w(&rio_server, read_buf, MAXLINE);
    if (strstr(read_buf, "\r\n") == NULL) {
        goto END;
    }
    response_len += strlen(read_buf);
    Rio_writen_w(client_fd, read_buf, strlen(read_buf));

    if (build_header(write_buf, &rio_server, &response_body_len) == -1) {
        goto END;
    }
    if (response_body_len < 0) {
        goto END;
    }
    response_len += strlen(write_buf);
    Rio_writen_w(client_fd, write_buf, strlen(write_buf));
    response_len += response_body_len;

    // Response body
    if (response_body_len > MAXLINE) {
        while (response_body_len > 0) {
            ssize_t n;
            if ((n = rio_server.rio_cnt) > 0) {
                memcpy(read_buf, rio_server.rio_bufptr, n);
                Rio_writen_w(client_fd, read_buf, n);
                response_body_len -= n;
                rio_server.rio_cnt = 0;
                continue;
            }
            // n = response_body_len > MAXLINE ? MAXLINE : response_body_len;
            n = 1;
            // n = Rio_readnb_w(&rio_server, read_buf, n);
            n = read(server_fd, read_buf, n);
            if (n == 0) {
                // Meet EOF
                goto END;
            }
            Rio_writen_w(client_fd, read_buf, n);
            response_body_len -= n;
        }
    } else {
        if (Rio_readnb_w(&rio_server, read_buf, response_body_len) < response_body_len) {
            goto END;
        }
        Rio_writen_w(client_fd, read_buf, response_body_len);
    }

    // Logging
    socklen_t client_len = sizeof client_info->addr;
    char client_host[MAXLINE];
    Getnameinfo((SA*)&client_info->addr, client_len, client_host, MAXLINE, NULL,
                0, NI_NUMERICHOST);
    sockaddr = (struct sockaddr_in*)Malloc(sizeof(struct sockaddr_in));
    sockaddr->sin_family = AF_INET;
    Inet_pton(AF_INET, client_host, &sockaddr->sin_addr);
    sockaddr->sin_port = htons((uint16_t)atoi(url_p->port));
    format_log_entry(logstring, sockaddr, uri, response_len);
    printf("%s\n", logstring);

END:
    free(sockaddr);
    free(url_p);
    if (server_fd >= 0) {
        close(server_fd);
    }

    return success;
}