#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct {
    char method[10];
    char hostname[MAXLINE];
    char port[10];
    char query[MAXLINE];
    char version[10];
    char header[MAXLINE];
} Request;

typedef struct CachedItem CachedItem;

struct CachedItem {
    char request[MAXLINE];
    size_t response_size;
    char* response;
    struct CachedItem* next;
};

typedef struct {
    size_t size;
    pthread_rwlock_t* lock;
    CachedItem* list;
} CacheList;

CacheList* cache_list = NULL;

void handle_client(void* vargp);
void initialize_struct(Request* req);
void parse_request(char request[MAXLINE], Request* req);
void assemble_request(Request* req, char request[MAXLINE]);
int get_from_cache(char request[MAXLINE], int clientfd);
void get_from_server(char request[MAXLINE], int serverfd, int clientfd);
void print_struct(Request* req);

/****** Cache functions ******/
CacheList* cache_init();
void cache_insert(CachedItem* item, CacheList* cache);
void evict(CacheList* cache);
CachedItem *find(char request[MAXLINE], CacheList* cache);
void move_to_front(CachedItem* item, CacheList* cache);
void cache_destruct(CacheList* cache);

int main(int argc, char** argv)
{
    if(argc!=2){
        printf("Usage: proxy <port>\n\nwhere <port> is listening port number between 4500 and 65000\n");
        return 0;
    }
    printf("%s", user_agent_hdr);
    Signal(SIGPIPE, SIG_IGN);

    cache_list = cache_init();

    int listenfd, *clientfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        clientfd = malloc(sizeof(int));
        *clientfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to client: (%s, %s)\n", client_hostname, client_port);
        pthread_t tid;
        Pthread_create(&tid, NULL, (void*)handle_client, clientfd);
    }
    cache_destruct(cache_list);
    return 0;
}

void handle_client(void* vargp){
    Pthread_detach(pthread_self());
    int clientfd = *((int*)vargp);
    free(vargp);

    int lines = 0;
    int serverfd;
    char buf[MAXLINE];
    char request_full[MAXLINE];
    Request* req = malloc(sizeof(Request));
    rio_t rio_to_client;

    initialize_struct(req);
    Rio_readinitb(&rio_to_client, clientfd);
    buf[0] = 0;
    while(strncmp(buf, "\r\n", 2)){
        size_t n = Rio_readlineb(&rio_to_client, buf, MAXLINE);
        if(n == -1 && errno == ECONNRESET){
            return;
        }
        lines++;
        if(lines==1){
            parse_request(buf, req);
            sprintf(req->header, "%sHost: %s:%s\r\n", req->header, req->hostname, req->port);
            sprintf(req->header, "%s%s", req->header, user_agent_hdr);
            sprintf(req->header, "%sConnection: close\r\n", req->header);
            sprintf(req->header, "%sProxy-Connection: close\r\n", req->header);
        } else if(strncmp(buf, "Proxy-Connection:", 17) && strncmp(buf, "Host:", 5) && strncmp(buf, "User-Agent:", 11)){
            sprintf(req->header, "%s%s", req->header, buf);
        }
    }
    if(lines<=0){
        return;
    }
    assemble_request(req, request_full); 

    if(get_from_cache(request_full, clientfd)){
        printf("%s", request_full);
        printf("Got from Cache\n");
        Close(clientfd);
        return;
    }
    printf("%s", request_full);
    serverfd = Open_clientfd(req->hostname, req->port);
    if(serverfd<0){
        printf("No connection to server\n");
        return;
    }
    printf("Connected to server: (%s, %s)\n", req->hostname, req->port);
    get_from_server(request_full, serverfd, clientfd);
    Close(serverfd);
    Close(clientfd);
    free(req);
}

void initialize_struct(Request* req){
    for(int i=0;i<MAXLINE;i++){
        req->hostname[i] = 0;
        req->query[i] = 0;
        req->header[i] = 0;
    }
    for(int i=0;i<10;i++){
        req->method[i] = 0;
        req->version[i] = 0;
        req->port[i] = 0;
    }
}

void parse_request(char request[MAXLINE], Request* req){
    char uri[MAXLINE];
    char version[MAXLINE];
    sscanf(request, "%s %s %s", req->method, uri, version);
    int uri_len = strlen(uri);
    strtok(uri, "/");
    char* url;
    if(strncmp(uri, "http", 4)){
        url = malloc(sizeof(char)*(strlen(uri)+1));
        strncpy(url, uri, strlen(uri)+1);
    } else {
        url = strtok(uri+strlen(uri)+1, "/ ");
    }
    if(strlen(url)+strlen(uri)+2<uri_len){
        char* query = url + strlen(url) + 1;
        sprintf(req->query, "/%s", query);
    } else {
        strncpy(req->query, "/", 2);
    }
    if(strstr(url, ":")){
        char* hostname = strtok(url, ":");
        char* port = hostname + strlen(hostname) + 1;
        sprintf(req->hostname, "%s",hostname);
        sprintf(req->port, "%s", port);
    } else {
        sprintf(req->hostname, "%s", url);
        strncpy(req->port, "80", 3);
    }
    if(!strncmp(version, "HTTP/1.1", 8)){
        strncpy(req->version, "HTTP/1.0", 9);
    }
}

void assemble_request(Request* req, char request[MAXLINE]){
    sprintf(request, "%s %s %s\r\n", req->method, req->query, req->version);
    sprintf(request, "%s%s", request, req->header);
}

int get_from_cache(char request[MAXLINE], int clientfd){
    CachedItem* item = NULL;
    if((item=find(request, cache_list)) != 0){
        Rio_writen(clientfd, item->response, item->response_size);
        return 1;
    }
    return 0;
}

void get_from_server(char request[MAXLINE], int serverfd, int clientfd){
    char response[MAXLINE];
    char cachebuf[MAX_OBJECT_SIZE];
    size_t n;
    size_t total_size = 0;
    char save = 1;
    rio_t rio_to_server;
    Rio_readinitb(&rio_to_server, serverfd);

    Rio_writen(serverfd, request, strlen(request));
    while((n=Rio_readnb(&rio_to_server, response, MAXLINE))>0){
        Rio_writen(clientfd, response, n);
        if(total_size+n<MAX_OBJECT_SIZE){
            strncpy(cachebuf+total_size, response, n);
        } else {
            save = 0;
        }
        total_size += n;
    }
    if(n == -1 && errno == ECONNRESET){
        return;
    }
    if(save){
        CachedItem* item = malloc(sizeof(CachedItem));
        strncpy(item->request, request, strlen(request)+1);
        item->response_size = total_size+1;
        item->response = malloc(sizeof(char)*(total_size+1));
        strncpy(item->response, cachebuf, total_size+1);
        cache_insert(item, cache_list);
    }
}

void print_struct(Request* req){
    printf("method: %s\n", req->method);
    printf("hostname: %s\n", req->hostname);
    printf("port: %s\n", req->port);
    printf("query: %s\n", req->query);
    printf("version: %s\n", req->version);
}

/****** Cache functions ******/

CacheList* cache_init(){
    CacheList* cache = malloc(sizeof(CacheList));
    cache->size = 0;
    cache->lock = malloc(sizeof(pthread_rwlock_t));
    pthread_rwlock_init(cache->lock, NULL);
    cache->list = NULL;
    return cache;
}

void cache_insert(CachedItem* item, CacheList* cache){
    pthread_rwlock_wrlock(cache->lock);
    while((item->response_size)+(cache->size)>=MAX_CACHE_SIZE){
        evict(cache);
    }
    item->next = cache->list;
    cache->list = item;
    cache->size += item->response_size;
    pthread_rwlock_unlock(cache->lock);
}

void evict(CacheList* cache){
    CachedItem* node = cache->list;
    while(node && node->next && node->next->next){
        node = node->next;
    }
    if(node == NULL){
        return;
    }

    if(node->next == NULL){
        cache->list = NULL;
        cache->size = 0;
    } else {
        cache->size -= node->next->response_size;
        free(node->next);
        node->next = NULL;
    }
}

CachedItem *find(char request[MAXLINE], CacheList* cache){
    if(cache->list == NULL){
        return NULL;
    }
    pthread_rwlock_rdlock(cache->lock);
    CachedItem* ret = NULL;
    CachedItem* node = cache->list;
    if(!strncmp(node->request, request, strlen(request))){
        ret = node;
        node = NULL;
    }
    while(node && node->next){
        CachedItem* target = node->next;
        if(!strncmp(target->request, request, strlen(request))){
            ret = target;
            break;
        }
        node = node->next;
    }
    pthread_rwlock_unlock(cache->lock);
    if(ret != NULL && node != NULL){
        move_to_front(node, cache);
    }
    return ret;
}

void move_to_front(CachedItem* item, CacheList* cache){
    pthread_rwlock_wrlock(cache->lock);
    CachedItem* target = item->next;
    item->next = target->next;
    target->next = cache->list;
    cache->list = target;
    pthread_rwlock_unlock(cache->lock);
}

void cache_destruct(CacheList* cache){
    CachedItem* node = cache->list;
    while(node){
        CachedItem* next = node->next;
        free(node);
        node = next;
    }
    pthread_rwlock_destroy(cache->lock);
    free(cache->lock);
    free(cache);
}
