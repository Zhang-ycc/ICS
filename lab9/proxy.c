#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

//wrappers
ssize_t Rio_readn_w(int fd, void* ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
        printf("Rio_readn error");
    return n;
}

ssize_t Rio_readnb_w(rio_t* rp, void* usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        printf("Rio_readnb error");
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t* rp, void* usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        printf("Rio_readlineb error");
        return 0;
    }
    return rc;
}

void Rio_writen_w(int fd, void* usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
        printf("Rio_writen error");
}


/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
void doit(int connfd, struct sockaddr_in* clientaddr);
void* thread(void* v);
size_t receive(int connfd, rio_t* client_rio);

typedef struct vargp{
    int connfd;
    struct sockaddr_in clientaddr;
} vargp_t;

sem_t mutex;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int listenfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    vargp_t *vargp;
    pthread_t tid;
    

    if (argc != 2) {
        fprintf(stderr, "usage :%s <port> \n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    Signal(SIGPIPE, SIG_IGN);
    //Sem_init(&mutex, 0, 1);

    while (1) {
        vargp = Malloc(sizeof(vargp_t));
        clientlen = sizeof(vargp->clientaddr);
        vargp->connfd = Accept(listenfd, (SA*)&vargp->clientaddr, &clientlen);
        Getnameinfo((SA*)&vargp->clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n", hostname, port);

        Pthread_create(&tid, NULL, thread, vargp);
    }
    Close(listenfd);
    exit(0);
}

void* thread(void* v) {
    Pthread_detach(Pthread_self());
    vargp_t* var = (vargp_t*)v;
    doit(var->connfd, &var->clientaddr);
    Close(var->connfd);
    Free(var);
    return NULL;
}

void doit(int connfd, struct sockaddr_in* clientaddr)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], requestheader[MAXLINE], port[MAXLINE];
    rio_t conn_rio;

    Rio_readinitb(&conn_rio, connfd);
    Rio_readlineb_w(&conn_rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    parse_uri(uri, hostname, pathname, port);

    //request header
    sprintf(requestheader, "%s /%s %s\r\n", method, pathname, version);

    size_t n, content_length = 0, byte_size = 0;

    while ((n = Rio_readlineb_w(&conn_rio, buf, MAXLINE)) != 0)
    {
        if (!strncasecmp(buf, "Content-Length", 14))
            sscanf(buf + 15, "%zu", &content_length);
        sprintf(requestheader, "%s%s", requestheader, buf);
        if (!strncmp(buf, "\r\n", 2)) break;
    }
    if (!n)
        return; //if empty, skip

    int client_fd = open_clientfd(hostname, port);
    
    Rio_writen_w(client_fd, requestheader, strlen(requestheader));

    char new_buf[MAXLINE];
    int flag = 1;
    //request body
    if (strcasecmp(method, "GET")) {
        for (int i = 0; i < content_length; i++) {
            if (Rio_readnb_w(&conn_rio, new_buf, 1) == 0) {
                flag = 0;
                break;  //if none, skip
            }
            Rio_writen_w(client_fd, new_buf, 1);
        }
    }
    
    //if need, get response
    if (flag) {
        rio_t client_rio;
        Rio_readinitb(&client_rio, client_fd);
        byte_size = receive(connfd, &client_rio);
    }

    format_log_entry(buf, clientaddr, uri, byte_size);
    
    //only one peer thread can modify the log
    Sem_init(&mutex, 0, 1);
    P(&mutex);
    printf("%s\n", buf);
    V(&mutex);

    Close(client_fd);
}

size_t receive(int connfd, rio_t* client_rio)
{
    char buf[MAXLINE];
    size_t n, byte_size = 0, content_length = 0;

    //response header
    while ((n = Rio_readlineb_w(client_rio, buf, MAXLINE)) != 0) {
        byte_size += n;
        if (!strncasecmp(buf, "Content-Length", 14))
            sscanf(buf + 15, "%zu", &content_length);
        Rio_writen_w(connfd, buf, strlen(buf));
        if (!strncmp(buf, "\r\n", 2)) break;
    }
    if (!n)
        return 0;

    //response body
    for (int i = 0; i < content_length; i++) {
        if (Rio_readnb_w(client_rio, buf, 1) == 0) return 0;
        Rio_writen_w(connfd, buf, 1);
        byte_size++;
    }
    return byte_size;
}


/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
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
        char *p = hostend + 1;
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
    }
    else {
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
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
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