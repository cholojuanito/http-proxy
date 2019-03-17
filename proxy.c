#include <stdio.h>

//#include "cache.h"
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE  1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_PORT_NUMBER 65536
#define CACHE_OFF       "off"
#define MULTILINE_FLAG  "/"

// Headers
static const char* userAgentHdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* connectionHead = "Connection: close\r\n";
static const char* proxyConnectionHead = "Proxy-Connection: close\r\n";
static const char* httpVersionHead = "HTTP/1.0\r\n";
static const char* dnsFailStr = "HTTP/1.0 400 \
    Bad Request\r\nServer: RUI_Proxy\r\nContent-Length: 137\r\nConnection: \
    close\r\nContent-Type: text/html\r\n\r\n<html><head></head><body><p>\
    This server coulnd't be connected, because DNS lookup failed.</p><p>\
    Powered by Rui Zhang.</p></body></html>";
static const char* connectionFailStr = "HTTP/1.0 400 \
    Bad Request\r\nServer: RUI_Proxy\r\nContent-Length: 108\r\nConnection: \
    close\r\nContent-Type: text/html\r\n\r\n<html><head></head><body><p>\
    This server coulnd't be connected.</p><p>\
    Powered by Rui Zhang.</p></body></html>";


//TODO: Make sure you need this
static const char* acceptStr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char* acceptEncoding = "Accept-Encoding: gzip, deflate\r\n";

// Function declarations
void usage();
void proxyProcess(int* arg);
int readRequest(char* str, int clientFd, char* host, char* port, char* cacheIdx, char* resource);
int forwardToServer(char* host, char* port, int* serverFd, char* requestStr);
int forwardToClient(int clientFd, char* content, unsigned int len);
int readAndForwardResponse(int serverFd, int clientFd, char* cacheIdx, char* content);

int append(char* content, char* str, unsigned int addSize, unsigned int* prevSize);
int parseRequest(char* str, char* method, char* protocol, char* version, char* hostPort, char* resource);
void getHostAndPort(char* hostPort, char* host, char* port);
void closeFd(int* clientFd, int* serverFd);
// END function declarations


int main(int argc, char** argv) {
    char* port;
    int useCache = 0; // TODO: Change this to 1
    struct sockaddr_in clientAddr;
    int clientLength = sizeof(clientAddr);
    int listenFd;

    // Incorrect number of arguments
    if (argc < 2) {
        usage(argv[0]);
    }

    // Decide if the cache should be used
    if (argc >= 3) {
        useCache = (strcmp(argv[2], CACHE_OFF));
    }

    port = argv[1];
    int portNum = atoi(port);
    if (portNum <= 0 || portNum >= MAX_PORT_NUMBER) {
        printf("Invalid port number: %d. Should be between 1 and 65535\n", portNum);
        exit(1);
    }

    // Initialize cache
    if (useCache) {
        printf("Using cache\n");
        // init cache
    }
    else {
        printf("Not using cache\n");
    }

    // Ignore SIGPIPE
    Signal(SIGPIPE, SIG_IGN);

    listenFd = Open_listenfd(port);
    while (1) {
        pthread_t tid;
        int* connfdp = Malloc(sizeof(int));
        *connfdp = -1;
        *connfdp = Accept(listenFd, (SA *) &clientAddr, (socklen_t *) &clientLength);
        Pthread_create(&tid, NULL, (void *)proxyProcess, (void *) connfdp);
	}


    printf("%s", userAgentHdr);
    return 0;
}

void usage(char *str) {
	printf("usage: %s <port> [use cache]\n", str);
    printf("port: the port this program will connect to\n");
    printf("port: should be a number between 1 and 65535\n");
	printf("cache: 'off' to turn off caching\n");
	printf("cache: 'on' to turn on caching\n");
	printf("cache is 'on' by default\n");
	exit(1);
}

void proxyProcess(int* arg) {
    printf("pid: %u\n",(unsigned int)Pthread_self());
	Pthread_detach(Pthread_self());

    int clientFd = (int)(*arg);
	int serverFd = -1;
    char buf[MAXBUF], requestStr[MAXBUF], host[MAXBUF], port[MAXBUF], resource[MAXBUF];
    char cacheIdx[MAXBUF], content[MAXBUF];
    unsigned int length;

    //int readVal = readRequest(requestStr, clientFd, host, port, cacheIdx, resource);

    printf("Read data from %s:%s\n",host,port);
	fflush(stdout);

    // if (!readVal) {

    // }

}

int readRequest(char* str, int clientFd, char* host, char* port, char* cacheIdx, char* resource) {

}

int parseRequest(char* str, char* method, char* protocol, char* version, char* hostPort, char* resource) {
	char url[MAXBUF];

    //  requests with multiple lines
	if((!strstr(str, MULTILINE_FLAG)) || !strlen(str)) {
		return -1;
    }

	strcpy(resource, MULTILINE_FLAG);
	sscanf(str,"%s %s %s", method, url, version);

	if (strstr(url, "://")) {
		sscanf(url, "%[^:]://%[^/]%s", protocol, hostPort, resource);
    }
	else {
		sscanf(url, "%[^/]%s", hostPort, resource);
    }

	return 0;
}

int forwardToServer(char* host, char* port, int* serverFd, char* requestStr) {

}


int forwardToClient(int clientFd, char* content, unsigned int len) {

}


int readAndForwardResponse(int serverFd, int clientFd, char* cacheIdx, char* content) {

}

void getHostAndPort(char* hostPort, char* host, char* port) {
    char* buf = strstr(hostPort,":");
	if (buf) {
		*buf = '\0';
		strcpy(port, buf + 1);
	}
	else {
		strcpy(port, "80");
    }

	strcpy(host, hostPort);
}

int append(char* content, char* str, unsigned int addSize, unsigned int* prevSize) {
    if(addSize + (*prevSize) > MAX_OBJECT_SIZE) {
        return 0;
    }

	memcpy(content + (*prevSize), str, addSize);
	*prevSize += addSize;
	return 1;
}

void closeFd(int* clientFd, int* serverFd) {
    if(clientFd && *clientFd >= 0) {
		Close(*clientFd);
    }

	if(serverFd && *serverFd >= 0) {
		Close(*serverFd);
    }
}