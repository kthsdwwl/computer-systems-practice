/*
 * proxy.c
 * Xi Lin(xlin2)
 *
 * This is a proxy program, which receives the request from client, 
 * send it to the target server, and then send the server response
 * back to client. This proxy supports multi-thread and cache.
 * 
 */

#include <stdio.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* My macros */
#define HOST       0
#define USER_AGENT 1

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxy_connection = "Proxy-Connection: close\r\n";
static const char *accept_hdr = 
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *method = "GET";
static const char *version = "HTTP/1.0\r\n";
static const char *error_read = "Error when calling Rio_readlineb.\n";
static const char *error_method = "Only accept GET method.\r\n";
static const char *error_uri = "URI invalid.\r\n";
static const char *protocol = "http://";


/* Global variables */
sem_t mutex;

/* Helper function declaration */
int arg_is_valid(char *arg) ;
void sigint_handler(int signal);
void *thread_job(void *arg);
void handle_request(int connfd);
void send_from_cache(int fd, CacheLine *cache_data);
void parse_uri(char *uri, char *host, char *port, char *path);
void add_request_header(char *header, char *buffer, int *flags);
void complete_request_header(char *req_header, char *host, int *flags);
void generate_request(char *req, char *path, char *req_header);
void forward_request(int fd, char *uri, char *host, char *port, char *req);

int main(int argc, char **argv)
{
    int listenfd, port, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    // check if input argument number meets requirements
    if (argc != 2 || !arg_is_valid(argv[1])) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    // check if input port number is valid
    port = atoi(argv[1]);
    
    if ((port < 1024) || (port > 65535)) {
        fprintf(stderr, "Invalid port number.\n");
        exit(0);
    }
    
    // signal handlers
    Signal(SIGPIPE, SIG_IGN);
    Signal(SIGINT, sigint_handler);
    
    // do the main job
    Sem_init(&mutex, 0, 1);
    init_cache();
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // create a thread to handle client request
        Pthread_create(&tid, NULL, thread_job, connfd);
    }
    
    return 0;
}


/*******************
 * Helper functions
 *******************/
 
/* 
 * arg_is_valid - check whether input port number is valid
 */
int arg_is_valid(char *arg) 
{
    int len = strlen(arg);
    int i = 0;
    while (i < len) {
        if ((*arg < '0') || (*arg > '9'))
            return 0;
        ++arg;
        ++i;
    }
    return 1;
}


/* 
 * sigint_handler - handler to handle sigint. When user clicks ctrl + c, 
 * free the cache and exit the program.
 */
void sigint_handler(int signal)
{
    printf("Exit\n");
    free_cache();
    exit(0);
}

/* 
 * thread_job - the function each thread will execute
 */
void *thread_job(void *arg)
{
    pthread_detach(pthread_self());
    int connfd = *((int *)arg);
    Free(arg);
    handle_request(connfd);
    Close(connfd);
    pthread_exit(NULL);
    return 0;
}

/* 
 * handle_request - receives client's request, rearrange it and send to 
 * server. After that, send server's response back to client.
 */
void handle_request(int connfd)
{
    rio_t rio;
    size_t size;
    int flags[2] = {0, 0};
    char buffer[MAXLINE] = {0}, req_method[MAXLINE] = {0}, 
         uri[MAXLINE] = {0}, host[MAXLINE] = {0}, 
         path[MAXLINE] = {0}, req_header[MAX_OBJECT_SIZE] = {0},
         uri_protocol[8] = {0}, req[MAX_OBJECT_SIZE] = {0},
         port[8] = "80";
    CacheLine *cache_data = NULL;
    
    Rio_readinitb(&rio, connfd);
    size = Rio_readlineb(&rio, buffer, MAXLINE);
    
    if (size < 0) {
        fprintf(stderr, error_read);
        return;
    }
    
    // get request method and uri from user request and check them
    sscanf(buffer, "%s %s", req_method, uri);
    if (strcmp(req_method, "GET") != 0) {
        Rio_writen(connfd, (void *)error_method, strlen(error_method));
        fprintf(stderr, error_method);
        return;
    }
    if (strcmp(strncpy(uri_protocol, uri, 7), protocol) != 0) {
        Rio_writen(connfd, (void *)error_uri, strlen(error_uri));
        fprintf(stderr, error_uri);
        return;
    }
    
    // check whether the object is cached. if yes, return object from cache
    cache_data = get_object(uri);
    if (cache_data != NULL) {
        send_from_cache(connfd, cache_data);
        return;
    }
    
    // get hostname, port number and path from uri
    parse_uri(uri, host, port, path);
    
    // read user header line by line and add to request header.
    // if the header is "Connection" or "Proxy-Connection", ignore it.
    while (1) {
        strcpy(buffer, "");
        if ((size = Rio_readlineb(&rio, buffer, MAXLINE)) < 0) {
            fprintf(stderr, error_read);
            return;
        }
        
        if (strcmp(buffer, "\r\n") == 0) break;
        add_request_header(req_header, buffer, flags);
    }
    // complete the header by adding lines such as "Connection" and 
    // "Proxy-Connection"
    complete_request_header(req_header, host, flags);
    
    // combine request method, path, version and header together to form
    // a request
    generate_request(req, path, req_header);
    
    // send the request to server and get response
    forward_request(connfd, uri, host, port, req);
}

/* 
 * send_from_cache - send object to client from cache
 */
void send_from_cache(int fd, CacheLine *cache_data)
{
    Rio_writen(fd, cache_data->object, cache_data->length);
}

/* 
 * parse_uri - parse hostname, port number and path from uri
 */
void parse_uri(char *uri, char *host, char *port, char *path)
{
    char *path_pos = NULL;
    char *port_pos = NULL;
    sscanf(uri, "http://%s", host);
    // start position of path and port, if they exist
    path_pos = strchr(host, '/');
    port_pos = strchr(host, ':');
    
    // extract path string
    if (path_pos != NULL) {
        strcpy(path, path_pos);
        *path_pos = '\0';
    }
    // extract port string
    if (port_pos != NULL) {
        strcpy(port, "");
        strcpy(port, port_pos + 1);
        *port_pos = '\0';
    }
}

/* 
 * add_request_header - add user request header to proxy request header
 * if necessary
 */
void add_request_header(char *header, char *buffer, int *flags)
{
    char key[MAXLINE] = {0};
    char *p = strchr(buffer, ':');
    strncpy(key, buffer, p - buffer);
    
    // if client's request header contains host, use that host
    if (strcmp(key, "Host") == 0) {
        flags[HOST] = 1;
        strcat(header, buffer);
    }
    else if (strcmp(key, "User-Agent") == 0) {
        flags[USER_AGENT] = 1;
        strcat(header, buffer);
    }
    else if (strcmp(key, "Connection") == 0) {
        return;
    }
    else if (strcmp(key, "Proxy-Connection") == 0) {
        return;
    }
    else if (strcmp(key, "Accept") == 0) {
        return;
    }
    else if (strcmp(key, "Accept-Encoding") == 0) {
        return;
    }
    else {
        strcat(header, buffer);
    }
}

/* 
 * complete_request_header - add connection and proxy-connection to proxy
 * header. If client's header does not contain host and user agent, add
 * pre-defined ones.
 */
void complete_request_header(char *req_header, char *host, int *flags)
{
    if (!flags[HOST]) {
        strcat(req_header, "Host: ");
        strcat(req_header, host);
        strcat(req_header, "\r\n");
    }
    if (!flags[USER_AGENT]) {
        strcat(req_header, user_agent_hdr);
    }
    strcat(req_header, connection);
    strcat(req_header, proxy_connection);
    strcat(req_header, accept_hdr);
    strcat(req_header, accept_encoding);
}

/* 
 * generate_request - combine request method, path, version and header 
 * together to form a requests
 */
void generate_request(char *req, char *path, char *req_header)
{
    if (strcmp(path, "") == 0) {
        strcat(path, "/");
    }
    sprintf(req, "%s %s %s", method, path, version);
    strcat(req, req_header);
    strcat(req, "\r\n");
}

/* 
 * forward_request - send the request to server and get response, then 
 * send the response back to client
 */
void forward_request(int fd, char *uri, char *host, char *port, char *req)
{
    char response[MAX_OBJECT_SIZE];
    char cache_buffer[MAX_OBJECT_SIZE] = {0};
    int n;
    int buffer_size = 0;
    int forward_fd = Open_clientfd(host, port);
    rio_t rio;
    
    Rio_writen(forward_fd, req, strlen(req));
    Rio_readinitb(&rio, forward_fd);
    
    while (1) {
        memset(response, 0, sizeof(response));
        if ((n = Rio_readnb(&rio, response, MAX_OBJECT_SIZE)) <= 0)
            break;
        Rio_writen(fd, response, n);
        strncat(cache_buffer, response, n);
        buffer_size += n;
    }
    
    // if the size of the object received is less than MAX_OBJECT_SIZE,
    // store it to cache
    if (buffer_size < MAX_OBJECT_SIZE) {
        add_object(uri, cache_buffer, buffer_size);
    }
    
    Close(forward_fd);
}
 
/***********************
 * End helper functions
 ***********************/
