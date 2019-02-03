#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include "base64.h"
#include "cJSON.h"
#include "sha1.h"
#include "util.h"
#include "config.h"

#define DEFAULT_BUFF_SIZE 9999
#define DEFAULT_PORT 7777
#define DEFAULT_LINE_MAX 256
#define DEFUALT_READ_TIMEOUT 300
#define WEB_SOCKET_KEY_LEN_MAX 256

struct config {
    char* accounts;
    char* address;
    char* port;
    char* daemon;
    char* test;
} config;

struct conn {
    int localfd;
    int remotefd;
    volatile int fin;
    pthread_mutex_t finMutex;
    pthread_cond_t finCond;
    struct sockaddr_in localAddr;
    struct sockaddr_in remoteAddr;
} conn;

typedef struct cuteServer {
    int pid;
    int fd;
    int poolSize;
    int needAuth;
    int daemon;
    volatile int index;
    volatile int maxConnCount;
    volatile int currConnCount;
    pthread_t *tids;
    pthread_mutex_t connMutex;
    pthread_cond_t connEmptyCond;
    pthread_cond_t connFullCond;
    struct sockaddr_in addr;
    struct conn *pool;
    struct config config;
    void (*serverHandler)(struct cuteServer *);
    void (*sinalHandler)(int);
    void* (*connHandler)(void *);
} cuteServer;

int initConfig(char *key, char *value);
int getRequest(int fd, char *payloadData);
int sendResponse(int fd, char* data, int dataLen);
char *getAcceptKey(char* buff);
char *getSecKey(char* buff);
int packData(char* frame, char* data, int dataLen);
int shakeHand(int fd);
int connectToRemote(const char *hostname, const char *serv);
void pipeForRemote(int connfd, int remotefd);
void pipeForLocal(int connfd, int remotefd);
void waitSignal(int signal);
void startup(struct cuteServer *server);
void __startup(struct cuteServer *server);
void* handleConnByWS(void *args);
void* __handleConnByWS(void *args);
void* handleConnBySS(void *args);
void closeConn(int fd);
void error(char *message);

cuteServer server;
void error(char *message) {
    fprintf(stderr, "error: %s", message);
    exit(1);
}

int getRequest(int fd, char *payloadData) {
    char buff[DEFAULT_BUFF_SIZE];
    char masks[4];
    int len = -1;
    ssize_t n = 0;
    if(fd <= 0) {
        return len;
    }
    n = read(fd, buff, 2);
    if (n <= 0) {
        return (int)n;
    }
    char fin = (buff[0] & 0x80) == 0x80;
    if (!fin) {
        return len;
    }
    char maskFlag = (buff[1] & 0x80) == 0x80;
    if (!maskFlag) {
        return len;
    }
    int payloadLen = buff[1] & 0x7F;
    if (payloadLen == 126) {
        memset(buff, 0, sizeof(buff));
        n = recv(fd, buff, 6, MSG_WAITALL);
        payloadLen = (buff[0] & 0xFF) << 8 | (buff[1] & 0xFF);
        memcpy(masks, buff + 2, 4);
        memset(buff, 0, sizeof(buff));
        n = recv(fd, buff, payloadLen, MSG_WAITALL);
    } else if (payloadLen == 127) {
        // TODO
        return len;
    } else {
        memset(buff, 0, sizeof(buff));
        n = recv(fd, buff, 4, MSG_WAITALL);
        memcpy(masks, buff, 4);
        memset(buff, 0, sizeof(buff));
        n = recv(fd, buff, payloadLen, MSG_WAITALL);
    }
    
    len = payloadLen;
    memset(payloadData, 0, payloadLen);
    memcpy(payloadData, buff, payloadLen);
    int i;
    for (i = 0; i < payloadLen; i++) {
        payloadData[i] = (char)(payloadData[i] ^ masks[i % 4]);
    }
    
    return len;
}

int sendResponse(int fd, char* data, int dataLen) {
    int len = 0;
    char buff[DEFAULT_BUFF_SIZE + 4];
    if (fd < 0) {
        return len;
    }
    
    int frameLen = packData(buff, data, dataLen);
    if (frameLen <= 0) {
        return len;
    }
    
    return (int)send(fd, buff, frameLen, MSG_WAITALL);
}

char *getSecKey(char* buff) {
    char *key;
    char *keyBegin;
    char *flag = "Sec-WebSocket-Key: ";
    int i = 0, bufLen = 0;
    
    key = (char *)malloc(WEB_SOCKET_KEY_LEN_MAX);
    memset(key, 0, WEB_SOCKET_KEY_LEN_MAX);
    if (!buff) {
        return NULL;
    }
    
    keyBegin = strstr(buff, flag);
    if (!keyBegin) {
        return NULL;
    }
    keyBegin += strlen(flag);
    
    bufLen = (int)strlen(buff);
    for (i = 0; i < bufLen; i++) {
        if (keyBegin[i] == 0x0A || keyBegin[i] == 0x0D) {
            break;
        }
        key[i] = keyBegin[i];
    }
    
    return key;
}

char *getAcceptKey(char* buff) {
    char *clientKey;
    char *serverKey;
    char *sha1DataTemp;
    char *sha1Data;
    int i, n;
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    if (!buff) {
        return NULL;
    }
    clientKey = (char *)malloc(DEFAULT_LINE_MAX);
    memset(clientKey, 0, DEFAULT_LINE_MAX);
    clientKey = getSecKey(buff);
    if (!clientKey) {
        return NULL;
    }
    
    strcat(clientKey, GUID);
    sha1DataTemp = sha1_hash(clientKey);
    n = (int)strlen(sha1DataTemp);
    sha1Data = (char *)malloc(n / 2 + 1);
    memset(sha1Data, 0, n / 2 + 1);
    for (i = 0; i < n; i += 2) {
        sha1Data[i / 2] = htoi(sha1DataTemp, i, 2);
    }
    serverKey = base64_encode(sha1Data, (int)strlen(sha1Data));
    
    return serverKey;
}

int shakeHand(int fd) {
    char buff[DEFAULT_BUFF_SIZE];
    char responseHeader[DEFAULT_BUFF_SIZE];
    char *acceptKey;
    if (!fd) {
        return -1;
    }
    if(read(fd, buff, DEFAULT_BUFF_SIZE) < 0) {
        return -1;
    }
    acceptKey = getAcceptKey(buff);
    if (!acceptKey) {
        return -1;
    }
    
    memset(responseHeader, '\0', DEFAULT_BUFF_SIZE);
    sprintf(responseHeader, "HTTP/1.1 101 Switching Protocols\r\n");
    sprintf(responseHeader, "%sUpgrade: websocket\r\n", responseHeader);
    sprintf(responseHeader, "%sConnection: Upgrade\r\n", responseHeader);
    sprintf(responseHeader, "%sSec-WebSocket-Accept: %s\r\n\r\n", responseHeader, acceptKey);
    
    return (int)write(fd, responseHeader, strlen(responseHeader));
}

int packData(char* frame, char* data, int dataLen) {
    int len = 0;
    if (dataLen < 126) {
        memset(frame, 0, dataLen + 2);
        frame[0] = 0x81;
        frame[1] = dataLen;
        memcpy(frame + 2, data, dataLen);
        len = dataLen + 2;
    } else if (dataLen < 0xFFFF) {
        memset(frame, 0, dataLen + 4);
        frame[0] = 0x81;
        frame[1] = 0x7e;
        frame[2] = (dataLen >> 8 & 0xFF);
        frame[3] = (dataLen & 0xFF);
        memcpy(frame + 4, data, dataLen);
        len = dataLen + 4;
    } else {
        //TODO
        len = 0;
    }
    
    return len;
}

void closeConn(int fd) {
    //shutdown(fd, 2);
    close(fd);
}

int connectToRemote(const char *hostname, const char *serv) {
    int sockfd, n;
    
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((n = getaddrinfo(hostname, serv, &hints, &res)) != 0) {
        printf("tcp connect error for %s, %s:%s\n", hostname, serv, gai_strerror(n));
        return -1;
    }
    
    ressave = res;
    
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) continue;
        
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;
        close(sockfd);
    } while ((res = res->ai_next) != NULL);
    
    if (res == NULL) {
        printf("tcp connect error for %s, %s\n", hostname, serv);
        return -1;
    }
    
    freeaddrinfo(ressave);
    return sockfd;
}

void pipeForRemote(int localfd, int remotefd) {
    int readlen = 0;
    int writelen = 0;
    char buff[DEFAULT_BUFF_SIZE];
    
    struct timeval tv;
    tv.tv_sec = DEFUALT_READ_TIMEOUT;
    tv.tv_usec = 0;
    
#ifdef __APPLE__
    int opt = 1;
    setsockopt(localfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
#endif
    
    for(;;) {
#ifdef __APPLE__
        setsockopt (remotefd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif
        
        readlen = (int)read(remotefd, buff, sizeof(buff));
        if(readlen > 0) {
            writelen = sendResponse(localfd, buff, readlen);
            if(writelen < 0) {
                printf("pipeForRemote write errno:%d\n", errno);
                break;
            }
        }
        if(readlen == 0) {
            break;
        }
        if(readlen < 0) {
            printf("pipeForRemote read errno:%d\n", errno);
            break;
        }
    }
    
    close(remotefd);
    printf("localfd(%d) closed:\n", localfd);
}

void pipeForLocal(int localfd, int remotefd) {
    int readlen = 0;
    int writelen = 0;
    char buff[DEFAULT_BUFF_SIZE];
    
    struct timeval tv;
    tv.tv_sec = DEFUALT_READ_TIMEOUT;
    tv.tv_usec = 0;
    
#ifdef __APPLE__
    int opt = 1;
    setsockopt(localfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
#endif
    for(;;) {
#ifdef __APPLE__
        setsockopt (remotefd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif
        
        readlen = getRequest(localfd, buff);
        if(readlen > 0) {
            writelen = (int)send(remotefd, buff, readlen, MSG_WAITALL);
            if(writelen < 0) {
                printf("pipeForLocal write errno:%d\n", errno);
                break;
            }
        }
        if(readlen == 0) {
            break;
        }
        if(readlen < 0) {
            printf("pipeForLocal read errno:%d\n", errno);
            break;
        }
    }
    
    close(localfd);
    printf("remotefd(%d) closed:\n", remotefd);
}

void* __pipeForLocal(void* args) {
#ifdef __APPLE__
    pthread_setname_np("__pipeForLocal");
#endif
    struct conn* conn = (struct conn*)args;
    printf("current fd:%d in __pipeForLocal\n", conn->localfd);
    pipeForLocal(conn->localfd, conn->remotefd);
    pthread_mutex_lock(&conn->finMutex);
    conn->fin = 1;
    pthread_cond_signal(&conn->finCond);
    pthread_mutex_unlock(&conn->finMutex);
    return (void*)0;
}

//TODO
void* __handleConnByWS(void *args) {
    char buff[DEFAULT_BUFF_SIZE];
    for (;;) {
        pthread_mutex_lock(&server.connMutex);
        while (server.currConnCount == 0) {
            pthread_cond_wait(&server.connEmptyCond, &server.connMutex);
        }
        
        server.index--;
        server.currConnCount--;
        struct conn conn = server.pool[server.index];
        pthread_cond_signal(&server.connFullCond);
        pthread_mutex_unlock(&server.connMutex);
        
        if(shakeHand(conn.localfd) <= 0) {
            closeConn(conn.localfd);
        }
        memset(buff, 0, sizeof(buff));
        int len = getRequest(conn.localfd, buff);
        if(len > 0) {
            char* header = buff+1;
            cJSON* json = cJSON_Parse(header);
            if(json) {
                cJSON *service = cJSON_GetObjectItem(json, "Service");
                // cJSON *type = cJSON_GetObjectItem(json, "Type");
                
                char* host;
                char* port;
                char *savePtr;
                host = strtok_r(service->valuestring, ":", &savePtr);
                port = strtok_r(NULL, ":", &savePtr);
                if (host != NULL && port != NULL) {
                    conn.remotefd = connectToRemote(host, port);
                    if (conn.remotefd < 0) {
                        printf("failed to connect remote server(%s:%s).\n", host, port);
                    } else {
                        pthread_t *tidptr = (pthread_t*)malloc(sizeof(pthread_t));
                        pthread_create(tidptr, NULL, __pipeForLocal, (void*)&conn);
                        pipeForRemote(conn.localfd, conn.remotefd);
                        
                        closeConn(conn.localfd);
                        closeConn(conn.remotefd);
                    }
                }
            }
        }
    }
    
close:
    
    return (void*)0;
}

void waitSignal(int signal) {return;}

void* handleConnByWS(void *args) {
    char buff[DEFAULT_BUFF_SIZE];
#ifdef __APPLE__
    pthread_setname_np("handleConnByWS");
#endif
    struct conn* conn = (struct conn*)args;
    conn->fin = 0;
    pthread_mutex_init(&conn->finMutex, NULL);
    pthread_cond_init(&conn->finCond, NULL);
    
    printf("current fd:%d in handleConnByWS\n", conn->localfd);
    
    if(shakeHand(conn->localfd) <= 0) {
        goto quit;
    }
    
    memset(buff, 0, sizeof(buff));
    int len = getRequest(conn->localfd, buff);
    if(len <= 0) {
        goto quit;
    }
    
    char* header = buff+1;
    cJSON* json = cJSON_Parse(header);
    if(!json) {
        goto quit;
    }
    
    int auth = 1;
    if(server.needAuth == 1 && server.config.accounts && strcmp(server.config.accounts, "")) {
        auth = 0;
        cJSON *userName = cJSON_GetObjectItem(json, "UserName");
        cJSON *password = cJSON_GetObjectItem(json, "password");
        char *accounts;
        char *account;
        char *savePtr;
        accounts = strdup(server.config.accounts);
        account = strtok_r(accounts, ";", &savePtr);
        while(account != NULL){
            char* temp = join(join(userName->valuestring, ":"), password->valuestring);
            if(strcmp(temp, account) == 0) {
                auth = 1;
                break;
            }
            account = strtok_r(NULL, ",", &savePtr);
        }
        free(accounts);
    }
    
    if(auth == 0) {
        goto quit;
    }
    
    cJSON *service = cJSON_GetObjectItem(json, "Service");
    char* host;
    char* port;
    char *savePtr;
    host = strtok_r(service->valuestring, ":", &savePtr);
    port = strtok_r(NULL, ":", &savePtr);
    if (host != NULL && port != NULL) {
        conn->remotefd = connectToRemote(host, port);
        if (conn->remotefd < 0) {
            printf("failed to connect remote server(%s:%s).\n", host, port);
            goto quit;
        } else {
            pthread_t tid;
            pthread_create(&tid, NULL, __pipeForLocal, (void*)conn);
            pipeForRemote(conn->localfd, conn->remotefd);
            if(pthread_cond_wait(&conn->finCond, &conn->finMutex)) {
                printf("pipe will be closed.\n");
            }
        }
    } else {
quit:
        closeConn(conn->localfd);
    }
    return 0;
}

void startup(struct cuteServer *server) {
    int i = 0;
    for (;;) {
        struct conn* conn = (struct conn*)malloc(sizeof(struct conn));
        socklen_t len = sizeof(len);
        memset(&conn->localAddr, 0, sizeof(conn->localAddr));
        conn->localfd = accept(server->fd, (struct sockaddr *)&conn->localAddr, &len);
        printf("accepting...\n");
        if(pthread_create(&server->tids[i], NULL, server->connHandler, (void*)conn) != 0) {
            error("failed to create thread.\n");
        }
        
        i++;
    }
}

// TODO
void __startup(struct cuteServer *server) {
    int i;
    for (i = 0; i < server->poolSize; i++) {
        if (pthread_create(&server->tids[i], NULL, server->connHandler, NULL) != 0)
            error("failed to create threads.\n");
    }
    
    for (;;) {
        pthread_mutex_lock(&server->connMutex);
        while (server->currConnCount > server->poolSize) {
            pthread_cond_wait(&server->connFullCond, &server->connMutex);
        }
        
        struct conn conn;
        socklen_t len = sizeof(len);
        memset(&conn.localAddr, 0, sizeof(conn.localAddr));
        printf("accepting...\n");
        conn.localfd = accept(server->fd, (struct sockaddr *)&conn.localAddr, &len);
        server->pool[server->index] = conn;
        server->currConnCount++;
        server->index++;
        pthread_cond_broadcast(&server->connEmptyCond);
        pthread_mutex_unlock(&server->connMutex);
    }
}


void background() {
    int pid;
    int i;
    if((pid=fork())) {
        exit(0);
    } else if(pid< 0) {
        exit(1);
    }
    
    setsid();
    if((pid=fork())) {
        exit(0);
    } else if(pid < 0) {
        exit(1);
    }
    
    for(i=0;i< NOFILE;++i) {
        close(i);
    }
    
    chdir("/tmp");
    umask(0);
}

int initConfig(char *key, char *value) {
    if(strcmp(key, "accounts") == 0) {
        server.config.accounts = (char*)malloc(strlen(value));
        strcpy(server.config.accounts, value);
    } else if (strcmp(key, "test") == 0) {
        server.config.test = (char*)malloc(strlen(value));
        strcpy(server.config.test, value);
    } else if(strcmp(key, "address") == 0) {
        server.config.address = (char*)malloc(strlen(value));
        strcpy(server.config.address, value);
    } else if(strcmp(key, "port") == 0) {
        server.config.port = (char*)malloc(strlen(value));
        strcpy(server.config.port, value);
    } else if(strcmp(key, "daemon") == 0) {
        server.config.daemon = (char*)malloc(strlen(value));
        strcpy(server.config.daemon, value);
    }
    
    return 0;
}

void test() {
    // for test
    parseConfig("/Users/Euphie/Documents/c/cute/config/cute.ini", initConfig);
    printf("%s\n", server.config.accounts);
    
    char *ptr;
    char *p;
    ptr = strtok_r(server.config.accounts, " ", &p);
    printf("%s\n", ptr);
    ptr = strtok_r(NULL, " ", &p);
    printf("%s\n", ptr);
    exit(0);
}

//#define M_WORKER

void initOptions(int argc, char *argv[]) {
    int c;
    memset(&server.addr, 0, sizeof(server.addr));
    server.addr.sin_family = AF_INET;
    server.addr.sin_port = htons(DEFAULT_PORT);
    server.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server.needAuth = 0;
    server.daemon = 0;
    while ((c = getopt(argc, argv, "a:p:l:m:c:hd")) != -1) {
        switch(c) {
            case 'c':
                if(!parseConfig(optarg, initConfig)) {
                    if(server.config.accounts) {
                        server.needAuth = 1;
                    }
                    if(server.config.address) {
                        server.addr.sin_addr.s_addr = inet_addr(server.config.address);
                    }
                    if(server.config.port) {
                        server.addr.sin_port = htons(atoi(server.config.port));
                    }
                    if(strcmp(server.config.daemon, "yes") == 0) {
                        server.daemon = 1;
                    }
                } else {
                    exit(1);
                }
                break;
            case 'a':
                server.needAuth = 1;
                server.config.accounts = optarg;
                break;
            case 'p':
                server.addr.sin_port = htons(atoi(optarg));
                break;
            case 'l':
                server.addr.sin_addr.s_addr = inet_addr(optarg);
                break;
            case 'd':
                server.daemon = 1;
                break;
            case 'h':
                printf("usage: cute [-l address] [-p port] [-h help] [-d daemon] [-c config]\n");
                exit(0);
        }
    }
    
    return;
}

int main(int argc, char *argv[]) {
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
    
    // test();
    initOptions(argc, argv);
    
    
    if(server.daemon == 1) {
        background();
    }
    
    server.poolSize = 20;
    server.currConnCount = 0;
    server.maxConnCount = 1000;
    server.index = 0;
    server.pid = getpid();
    server.sinalHandler = waitSignal;
#ifdef M_WORKER
    // TODO
    server.connHandler = __handleConnByWS;
    server.serverHandler = __starup;
#else
    server.connHandler = handleConnByWS;
    server.serverHandler = startup;
#endif
    server.tids = (pthread_t *)malloc(sizeof(pthread_t) * server.poolSize);
    server.pool = (struct conn *)malloc(sizeof(struct conn) * server.poolSize);
    pthread_mutex_init(&server.connMutex, NULL);
    pthread_cond_init(&server.connEmptyCond, NULL);
    pthread_cond_init(&server.connFullCond, NULL);
    
    server.fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt));
    if (server.fd < 0) {
        error("failed to create socket.\n");
    }
    
    if (bind(server.fd, (struct sockaddr *)&server.addr, sizeof(server.addr)) < 0) {
        error("failed to bind the address.\n");
    }
    
    if (listen(server.fd, server.maxConnCount * 2) < 0) {
        error("failed to listen.\n");
    }
    
    printf("server %s:%d has started...\n", inet_ntoa(server.addr.sin_addr), ntohs(server.addr.sin_port));
    server.serverHandler(&server);
}
