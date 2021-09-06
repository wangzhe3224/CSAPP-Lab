#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
//LRU_MAGIC_NUMBER 是LRU可能出现的最大时间戳
//由于缓存总大小为　1MB 缓存对象最大大小为1KB 所以设计一个包括10个缓存块(每个块的大小为1KB)的缓存
#define LRU_MAGIC_NUMBER 99999
#define CACHE_OBJS_COUNT 10
typedef struct
{
    char cache_obj[MAX_OBJECT_SIZE];
    char cache_url[MAXLINE];
    int isEmpty;
    int stamp;        //时间戳
    int readcnt;      //reader 计数器
    int writecnt;     // writer 计数器
    sem_t rdcntmutex; //保护对readcnt进行访问
    sem_t wmutex;     //保护对cache block进行读操作
    sem_t wtcntmutex; //保护对writecnt进行访问
    sem_t queue;
} cache_block;

typedef struct
{
    cache_block cache_objs[CACHE_OBJS_COUNT];
    int cache_num;
} Cache;

static Cache cache;

/* Cache functions */
void cache_init();
int cache_find(char *url);
int cache_eviction();
void cache_update_stamp(int index);
void cache_url(char *rul, char *buf);
void readerPre(int i);
void readerAfter(int i);
void writerPre(int i);
void writerAfter(int i);

void *thread(void *vargp);
void print_thread_id(pthread_t id);
int doit(int fd);
int parse_uri(char *uri, char *newuri, char *host, char *port);
char *port_plus_one(char *port);
char *find(char *s, char c, int num);

void connect_to_server(char *new_uri, char *host, char *port, rio_t *rio_client, int connfd, char *original_url);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    printf("%s", user_agent_hdr);
    
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 开始 */
    int listenfd, *connfdp;
    char hostname[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    cache_init();//初始化缓存

    char* port = argv[1];
    listenfd = Open_listenfd(port);  /* 代理的监听端口 */

    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfdp = malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accept connection from (%s, %s)\n", hostname, port);
        /* 多线程 */
        Pthread_create(&tid, NULL, thread, connfdp);
        fprintf(stderr, "Thread id: %li\n", tid);
    }

    return 0;
}


void *thread(void *vargp)
{
    /* 释放connfdp的堆内存，防止内存泄漏 */
    int connfd = *(int *)((int *) vargp);
    /* detach线程会被系统自动回收，不需要主线程回收 */
    Pthread_detach(pthread_self());
    Free(vargp);
    fprintf(stderr, "Handling request with fd: %i\n", connfd);

    doit(connfd);

    Close(connfd);
    return NULL;
}

void print_thread_id(pthread_t id)
{
    size_t i;
    for (i = sizeof(i); i; --i)
        printf("%02x", *(((unsigned char *)&id) + i - 1));
}


/**
 * @brief handle 1 HTTP request connection
 * 
 * @param fd 
 * @return int 
 */
int doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char urlcpy[MAXLINE];  // Why?
    char new_uri[MAXLINE],host[MAXLINE], *port=malloc(6);//端口号最大为65536 所以分配6个字节
    rio_t rio;  // 用作读写buffer
    
    Rio_readinitb(&rio, fd);
    // 把 fd 带来的客户端请求读入本地缓存
    // eb 是一个blocking函数：超过最大buffer或者EOF返回
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return -1;

    fprintf(stderr, "处理请求：%s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
    fprintf(stderr, "解析请求：%s, %s, %s\n", method, uri, version);

    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return -1;
    }

    strcpy(urlcpy, uri);

    // 查询缓存, i是对应的缓存块
    int i;
    if ((i = cache_find(urlcpy)) != -1)
    {
        fprintf(stderr, "缓存命中，写入缓冲区。\n");
        readerPre(i);
        Rio_writen(fd, cache.cache_objs[i].cache_obj, strlen(cache.cache_objs[i].cache_obj));
        readerAfter(i);
        cache_update_stamp(i);
        return 0;
    }

    fprintf(stderr, "无缓存，连接后端服务器。。。\n");

    // 解析uri
    //port=PortPlusOne(proxy_port) ;//服务器的默认监听端口为代理监听端口+1
    if(!parse_uri(uri, new_uri,host,port))
    {
        port="80"; //当url中不包含端口　默认web端口号为80
    }
    printf("new uri:%s, host:%s, port:%s\n",new_uri,host,port);

    //如果是合法的HTTP请求，那么开启和服务器的连接 并且将http请求发送给服务器
    connect_to_server(new_uri, host, port, &rio, fd, urlcpy);   

    return 0;
}


void connect_to_server(char *new_uri, char *host, char *port, rio_t *rio_client, int connfd, char *original_url)
{
    int clientfd = Open_clientfd(host, port); 
    if (clientfd<0)
    {
        fprintf(stderr, "连接远端服务器失败\n");
        return;
    }
    fprintf(stderr, "连接远端服务器成功\n");

    char buf_client[MAXLINE];
    rio_t rio_server; // 远端服务器的buffer
    printf("HTTP request start\n");
    //先将HTTP请求发送给服务器
    /*先发送请求行*/
    sprintf(buf_client, "GET %s HTTP/1.0\r\n", new_uri);
    printf("缓冲区内容： %s\n", buf_client);
    printf("头部写入缓冲： %s\n", new_uri);
    Rio_writen(clientfd, buf_client, strlen(buf_client));

    //再发送请求头部
    /*以下内容是必须发送的*/
    printf("主体写入缓冲： %s\n", new_uri);
    sprintf(buf_client, "Host: %s\r\n", host);
    printf("缓冲区内容： %s\n", buf_client);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "%s", user_agent_hdr);
    printf("缓冲区内容： %s\n", buf_client);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Connection: close\r\n");
    printf("缓冲区内容： %s\n", buf_client);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Proxy-Connection: close\r\n");
    printf("缓冲区内容： %s\n", buf_client);
    Rio_writen(clientfd, buf_client, strlen(buf_client));

    // rio_client 是代理和服务器之间的buffer
    Rio_readlineb(rio_client, buf_client, MAXLINE);
    while (strcmp(buf_client, "\r\n"))
    {
        //忽略已经发送的请求头
        /*strstr(const char *src,char *sub) 查找首次src中首次出现子串sub的位置　如果没有找到返回NULL*/
        if (!strstr(buf_client, "Host") && !strstr(buf_client, "User-Agent") && !strstr(buf_client, "Connection") && !strstr(buf_client, "Proxy-Connection"))
        {
            printf("发送请求： %s\n", buf_client);
            Rio_writen(clientfd, buf_client, strlen(buf_client));
        }

        Rio_readlineb(rio_client, buf_client, MAXLINE);
    }
    printf("HTTP request end\r\n");
    //发送空行　因为以上循环的终止条件是buf_clinet为空行 buf_clinet=="\r\n" strcmp返回0
    Rio_writen(clientfd, "\r\n", strlen("\r\n"));

    printf("HTTP response start\n");
    //再将从服务器获得的HTTP响应发送给客户端
    Rio_readinitb(&rio_server, clientfd);
    //coonfd是代理和客户端之间建立连接的描述符
    //HTTP响应结束是EOF
    /*receive message from end server and send to the client*/
    size_t n;
    char cache_buf[MAX_OBJECT_SIZE]; //缓存当前HTTP响应
    int HTTP_reponse_size = 0;       //HTTP响应的大小
    while ((n = Rio_readlineb(&rio_server, buf_client, MAXLINE)) != 0)
    {
        fprintf(stderr, "reponse size:%d\n", HTTP_reponse_size);
        HTTP_reponse_size += n;
        if (HTTP_reponse_size < MAX_OBJECT_SIZE)
            strcat(cache_buf, buf_client);
        Rio_writen(connfd, buf_client, n);
    }
    printf("HTTP response end\n");

    Close(clientfd);

    //如果HTTP响应的大小没有超过设计的最大缓存块大小　说明可以在代理本地缓存该HTTP响应的内容
    if (HTTP_reponse_size < MAX_OBJECT_SIZE)
    {
        cache_url(original_url, cache_buf);
    }
}

/* 
当用户在浏览器输入URL地址，浏览器会向代理发送类似这样的请求
GET http://www.cmu.edu/hub/index.html HTTP/1.0
也可包含自定义的服务器的web端口号
GET http://www.cmu.edu:8080/hub/index.html HTTP/1.0
代理应该将请求解析为:1)主机名:www.cmu.edu; 2) 端口号 8080(如果存在)　３)路径/查询: /hub/index.html
主机名说明服务器在哪里，端口号说明服务器上的监听端口号
这样代理就可以确定他应该打开一个到www.cmu.edu的连接，并且发送一个HTTP请求:
GET /hub/index.html HTTP/1.0
HTTP请求以回车换行为结束('\r\n')
现代的浏览器会产生以HTTP/1.1为结尾的HTTP请求，但是代理的请求是HTTP/1.0。在本次实验中，代理一个处理HTTP/1.0请求，然后向服务器发送HTTP/1.0请求。*/
int parse_uri(char *uri, char *newuri, char *host, char *port)
{
    char *ptr;
    ptr = find(uri, '/', 3); //查找第3个出现'/'的位置 http://www.cmu.edu:8080/
    if (ptr)
    {
        //将解析后的新uri保存　并且将原uri分割成　http://www.cmu.edu　和　/hub/index.html
        strcpy(newuri, ptr);
        *ptr = '\0';
    }
    else
        strcpy(newuri, "");

    ptr = find(uri, '/', 2); //查找第2个出现'/'的位置 http://www.cmu.edu
    if (ptr)
    {
        //将host解析为  www.cmu.edu:8080
        strcpy(host, ptr + 1);
        *ptr = '\0';
    }
    ptr = find(host, ':', 1); //查找第2个出现':'的位置 www.cmu.edu:8080
    //if (ptr!=NULL)printf("aa\n");
    if (ptr != NULL)
    {   /* 在uri中存在端口号*/
        //将解析后的port保存　并且将原uri分割成host为　www.cmu.edu　和　端口号8080
        //port=malloc(6);//端口号最大为65536 所以分配6个字节

        strcpy(port, ptr + 1);
        *ptr = '\0';
        return 1;
    }

    return 0;
}

//寻找在字符串s中　第num次出现字符'c'的位置，返回指向该位置的指针
char *find(char *s, char c, int num)
{
    int cnt = 0;
    while (*s != '\0')
    {
        //printf("%c",*s);
        if (*s == c)
            cnt++;
        if (cnt == num)
        { //printf("\n");
            return s;
        }
        ++s;
    }
    //printf("\n");
    return NULL;
}

/**
 * @brief return error message to client
 * 
 * @param fd
 * @param cause 
 * @param errnum 
 * @param shortmsg 
 * @param longmsg 
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];  // 创建一个buffer（在栈中)

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));
    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

void cache_init()
{
    cache.cache_num = 0;
    for (int i=0; i<CACHE_OBJS_COUNT; i++)
    {
        cache.cache_objs[i].isEmpty  = 1;
        cache.cache_objs[i].stamp    = -1;
        cache.cache_objs[i].readcnt  = 0;
        cache.cache_objs[i].writecnt = -1;
        Sem_init(&cache.cache_objs[i].rdcntmutex, 0, 1); // 读取计数器锁
        Sem_init(&cache.cache_objs[i].wmutex, 0, 1);
        Sem_init(&cache.cache_objs[i].wtcntmutex, 0, 1);
        Sem_init(&cache.cache_objs[i].queue, 0, 1);
    }
}

int cache_find(char *url)
{
    // A hash map is better
    int i;
    for (i = 0; i < CACHE_OBJS_COUNT; i++)
    {
        readerPre(i);
        if (cache.cache_objs[i].isEmpty == 0 && strcmp(url, cache.cache_objs[i].cache_url) == 0)
            break;
        readerAfter(i);
    }
    if (i==CACHE_OBJS_COUNT)
        return -1;

    return i;
}

int cache_eviction()
{
    int i;
    int max_stamp = -2, max_stamp_idx = -1;
    for (i=0; i<CACHE_OBJS_COUNT; i++)
    {
        readerPre(i);
        // 找到一个可用的缓存块
        if (cache.cache_objs[i].isEmpty)
        {
            cache.cache_objs[i].isEmpty = 0;
            max_stamp_idx = i;
            readerAfter(i);
            break;
        }
        // 如果没找到可用的缓存块，选择一个stamp最大的块
        if (cache.cache_objs[i].stamp > max_stamp) //搜索具有最大时间戳的缓存块
        {
            max_stamp_idx = i;
            max_stamp = cache.cache_objs[i].stamp;
        }
        readerAfter(i);
    }

    return max_stamp_idx;
}

//更新LRU时间戳
void cache_update_stamp(int index)
{

    writerPre(index);
    cache.cache_objs[index].stamp = 0; //将这个块设置位最近使用
    writerAfter(index);
    for (int i = 0; i < CACHE_OBJS_COUNT; i++)
    {
        if (i != index && cache.cache_objs[i].isEmpty == 0)
        {
            writerPre(i);
            cache.cache_objs[i].stamp++;
            writerAfter(i);
        }
    }
}

void cache_url(char *url, char *buf)
{
    int i = cache_eviction();
    writerPre(i);
    strcpy(cache.cache_objs[i].cache_url, url);
    strcpy(cache.cache_objs[i].cache_obj, buf);
    cache.cache_objs[i].isEmpty = 0;
    writerAfter(i);

    cache_update_stamp(i);
}

void readerPre(int i)
{
    P(&cache.cache_objs[i].rdcntmutex);
    cache.cache_objs[i].readcnt++;
    if (cache.cache_objs[i].readcnt == 1)
        // 读者优先, 获得写入锁
        P(&cache.cache_objs[i].wmutex); //first in
    V(&cache.cache_objs[i].rdcntmutex);
}

//读完数据之后的操作
void readerAfter(int i)
{
    P(&cache.cache_objs[i].rdcntmutex);
    cache.cache_objs[i].readcnt--;
    if (cache.cache_objs[i].readcnt == 0)
        // 没有读者了，才释放写入锁
        V(&cache.cache_objs[i].wmutex); //Last out
    V(&cache.cache_objs[i].rdcntmutex);
}

//进入写者模式之前的操作
void writerPre(int i)
{
   P(&cache.cache_objs[i].wmutex);
}

//写完数据之后的操作
void writerAfter(int i)
{
  V(&cache.cache_objs[i].wmutex);
}