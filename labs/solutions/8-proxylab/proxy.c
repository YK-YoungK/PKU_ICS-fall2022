#include <stdio.h>
#include "csapp.h"
#include <string.h>

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
/* #define DEBUG */
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 10490000
#define MAX_OBJECT_SIZE 102400
#define MAX_OBJECT_NUM 12

int totalcachesize, totalcachenum, totaltime, totalread;
sem_t cache_mutex, totaltime_mutex, read_mutex;

typedef struct
{
    char cache_obj[MAX_OBJECT_SIZE + 10];
    char cache_url[MAXLINE + 10];
    int empty;
    int object_size;
    int lutime;         /* time stamp */
    sem_t time_mutex;   /* Protection for lutime */
} cache_block;
cache_block allcache[MAX_OBJECT_NUM];

/* Some string constants */
/* You won't lose style points for including this long line in your code */
static char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static char *connection_hdr = "Connection: close\r\n";
static char *proxy_hdr = "Proxy-Connection: close\r\n";
static char *https_res = 
    "HTTP/1.1 200 Connection Established\r\nConnection: close\r\n\r\n";

/* functions for running the thread-based proxy */
void *thread(void *vargp);
void doit(int fd);

/* functions for maintain http requests */
void connect_server(char *uri, char *hostname, char *query, 
                    char *port, int connfd, rio_t *rio_client);

/* functions for maintain https requests */
int phase_uri(char *uri, char *hostname, char *query, char *port);
void phase_uri_https(char *uri, char *hostname, char *port);
void *https_send(void *vargp);

/* functions for maintaining the cache of proxy */
void cache_init();
int cache_find(char *url);
int cache_evict(int size);
int cache_read(char *dest_buf, char *url);
void cache_write(char *buf, char *url, int size);

int main(int argc, char *argv[])
{
    Signal(SIGPIPE, SIG_IGN);
    cache_init();

    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        dbg_printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfd);
    }
    return 0;
}

/* thread routine */
void *thread(void *vargp)
{
    Pthread_detach(pthread_self());
    int connfd = *((int *)vargp);
    Free(vargp);
    doit(connfd);
    return NULL;
}

/* main routine to serve requests */
void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], query[MAXLINE], port[MAXLINE];
    rio_t rio;
    pthread_t tid;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    dbg_printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (!strcmp(method, "CONNECT"))         /* https request */
    {
        phase_uri_https(uri, hostname, port);
        Rio_readlineb(&rio, buf, MAXLINE);
        while (strcmp(buf, "\r\n"))         /* Just ignore other headers */
            Rio_readlineb(&rio, buf, MAXLINE);

        int clientfd = Open_clientfd(hostname, port);
        if (clientfd < 0)
        {
            Close(fd);
            return;
        }
        else
            Write(fd, https_res, strlen(https_res));

        /* Create another thread to get data from client and send to server */
        int sendserver[2] = {0};
        sendserver[0] = fd;
        sendserver[1] = clientfd;
        Pthread_create(&tid, NULL, https_send, sendserver);

        /* get data from server and send to client */
        char buf[MAXLINE + 10] = {};
        int len;
        while ((len = Read(clientfd, buf, MAXLINE)) > 0)
            Write(fd, buf, len);

        /* Close the connections */
        Close(fd);
        Close(clientfd);
        return;
    }

    if (strcmp(method, "GET"))          /* Not http request */
    {
        printf("Proxy does not implement this method");
        return;
    }

    /* Serve http request */
    phase_uri(uri, hostname, query, port);
    connect_server(uri, hostname, query, port, fd, &rio);
    return;
}

/* phase https uri to hostname:port */
void phase_uri_https(char *uri, char *hostname, char *port)
{
    strcpy(hostname, uri);
    char *portpos = strstr(hostname, ":");
    strcpy(port, portpos + 1);
    *portpos = '\0';
    return;
}

/* get data from vargp[0] and send to vargp[1] */
void *https_send(void *vargp)
{
    Pthread_detach(pthread_self());
    int readfd = ((int *)vargp)[0];
    int writefd = ((int *)vargp)[1];
    char buf[MAXLINE + 10] = {};
    int len;
    while ((len = Read(readfd, buf, MAXLINE)) > 0)
        Write(writefd, buf, len);
    return NULL;
}

/* phase http uri to hostname:port(optional)/query */
int phase_uri(char *uri, char *hostname, char *query, char *port)
{
    char *hostpos = strstr(uri, "//");
    if (!hostpos)
        return -1;
    hostpos += 2;
    strcpy(hostname, hostpos);

    char *querypos = strstr(hostname, "/");
    if (querypos)
    {
        strcpy(query, querypos);
        *querypos = '\0';
    }
    else
        strcpy(query, "/");

    char *portpos = strstr(hostname, ":");
    if (portpos)
    {
        strcpy(port, portpos + 1);
        *portpos = '\0';
    }
    else
        strcpy(port, "80");
    return 0;
};

/* serve http request */
void connect_server(char *uri, char *hostname, char *query, 
                    char *port, int connfd, rio_t *rio_client)
{
    char cache_buf[MAX_OBJECT_SIZE + 10] = {};

    /* Find the request in cache first */
    int cachelen = cache_read(cache_buf, uri);
    if (cachelen >= 0)
    {
        dbg_printf("send back, len: %d\n", cachelen);
        Rio_writen(connfd, cache_buf, cachelen);
        Close(connfd);
        return;
    }

    int clientfd = Open_clientfd(hostname, port);
    if (clientfd < 0)
    {
        printf("connection failed\n");
        Close(connfd);
        return;
    }

    char buf[MAXLINE + 10] = {};

    dbg_printf("send HTTP request start\n");
    sprintf(buf, "GET %s HTTP/1.0\r\n", query);
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Host: %s\r\n", hostname);
    Rio_writen(clientfd, buf, strlen(buf));
    Rio_writen(clientfd, user_agent_hdr, strlen(user_agent_hdr));
    Rio_writen(clientfd, connection_hdr, strlen(connection_hdr));
    Rio_writen(clientfd, proxy_hdr, strlen(proxy_hdr));

    /* Send other request headers */
    Rio_readlineb(rio_client, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        if (!strstr(buf, "Host") && !strstr(buf, "User-Agent") && 
            !strstr(buf, "Connection") && !strstr(buf, "Proxy-Connection"))
            Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(rio_client, buf, MAXLINE);
    }
    Rio_writen(clientfd, "\r\n", strlen("\r\n"));
    dbg_printf("send HTTP request end\r\n");

    dbg_printf("get HTTP response start\n");
    rio_t rio_server;
    Rio_readinitb(&rio_server, clientfd);

    /* get response from end server and send to the client */
    int len = 0;
    int totallen = 0;   /* size of response */
    while ((len = Rio_readnb(&rio_server, buf, MAXLINE)) > 0)
    {
        Rio_writen(connfd, buf, len);
        dbg_printf("%s", buf);
        dbg_printf("reponse size:%d\n", len);
        totallen += len;
        if (totallen <= MAX_OBJECT_SIZE)
            memcpy(cache_buf + (totallen - len), buf, len);
    }
    dbg_printf("get HTTP response end\n");

    Close(clientfd);
    Close(connfd);

    if (totallen <= MAX_OBJECT_SIZE)            /* put into cache */
        cache_write(cache_buf, uri, totallen);
}

/* init cache */
void cache_init()
{
    totalcachesize = 0;
    totalcachenum = 0;
    totaltime = 0;
    totalread = 0;
    Sem_init(&cache_mutex, 0, 1);
    Sem_init(&totaltime_mutex, 0, 1);
    Sem_init(&read_mutex, 0, 1);
    for (int i = 0; i < MAX_OBJECT_NUM; ++i)
    {
        allcache[i].empty = 1;
        Sem_init(&allcache[i].time_mutex, 0, 1);
    }
}

/* Find cache block with given url, return -1 if not found */
int cache_find(char *url)
{
    dbg_printf("totalcachenum: %d\n", totalcachenum);
    dbg_printf("find: %s\n", url);
    int result = -1;
    for (int i = 0; i < MAX_OBJECT_NUM; ++i)
    {
        if ((allcache[i].empty == 0) && (!strcmp(url, allcache[i].cache_url)))
            result = i;
    }
    dbg_printf("%d\n", result);
    return result;
}

/* Choose the evicted block, using strict LRU */
int cache_evict(int size)
{
    /* Empty block available and cache size available */
    if (totalcachesize + size <= MAX_CACHE_SIZE && 
                        totalcachenum < MAX_OBJECT_NUM)
    {
        for (int i = 0; i < MAX_OBJECT_NUM; ++i)
        {
            if (allcache[i].empty == 1)
                return i;
        }
    }

    int mintime = 1 << 30, minplace = -1;
    for (int i = 0; i < MAX_OBJECT_NUM; ++i)
    {
        if (allcache[i].empty == 0)
        {
            P(&allcache[i].time_mutex);
            if (allcache[i].lutime < mintime)
            {
                mintime = allcache[i].lutime;
                minplace = i;
            }
            V(&allcache[i].time_mutex);
        }
    }

    totalcachesize -= allcache[minplace].object_size;
    totalcachenum--;
    allcache[minplace].empty = 1;
    return minplace;
}

/* 
 * Read and copy cache (given url), return the length of copied buf;
 * return -1 if cache miss
 */
int cache_read(char *dest_buf, char *url)
{
    P(&read_mutex);
    totalread++;
    if (totalread == 1)
        P(&cache_mutex);
    V(&read_mutex);

    int id = cache_find(url);
    if (id == -1)               /* cache miss */
    {
        P(&read_mutex);
        totalread--;
        if (totalread == 0)
            V(&cache_mutex);
        V(&read_mutex);
        return -1;
    }

    int len = allcache[id].object_size;
    memcpy(dest_buf, allcache[id].cache_obj, allcache[id].object_size);
    
    /* Update time stamp */
    P(&totaltime_mutex);
    totaltime++;
    P(&allcache[id].time_mutex);
    allcache[id].lutime = totaltime;
    V(&allcache[id].time_mutex);
    V(&totaltime_mutex);

    P(&read_mutex);
    totalread--;
    if (totalread == 0)
        V(&cache_mutex);
    V(&read_mutex);

    return len;
}

/* Write new cache block */
void cache_write(char *buf, char *url, int size)
{
    P(&cache_mutex);

    /* find eviction(s) */
    int evict = cache_evict(size);
    while (totalcachesize + size > MAX_CACHE_SIZE)
        evict = cache_evict(size);

    /* update */
    totalcachesize += size;
    totalcachenum++;

    allcache[evict].empty = 0;
    memcpy(allcache[evict].cache_obj, buf, size);
    strcpy(allcache[evict].cache_url, url);
    allcache[evict].object_size = size;

    /* Update time stamp */
    P(&totaltime_mutex);
    totaltime++;
    /* Since we have one writer and no reader here, it is no need to lock */
    allcache[evict].lutime = totaltime;
    V(&totaltime_mutex);

    V(&cache_mutex);
}