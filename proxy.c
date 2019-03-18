#include <stdio.h>

//#include "cache.h"
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE           1049000
#define MAX_OBJECT_SIZE         102400
#define MIN_PORT_NUMBER      1024
#define MAX_PORT_NUMBER     65536
#define CACHE_OFF                        "off"
#define MULTILINE_FLAG               "/"
#define DEFAULT_HTTP_PORT    "80"
#define GET                                        "GET"
#define END                                       "\r\n"
#define USER_AGENT_KEY           "User-Agent:"
#define ACCEPT_KEY                     "Accept:"
#define ACCEPT_ENCODE_KEY "Accept-Encoding:"
#define CONNECTION_KEY         "Connection:"
#define PROXY_KEY                       "Proxy Connection:"
#define COOKIE_KEY                      "Cookie:"
#define CONTENT_LEN_STR_1    "Content-Length"
//#define CONTENT_LEN_KEY_1   "Content-Length:"
#define CONTENT_LEN_STR_2   "Content-length"
//#define CONTENT_LEN_KEY_2   "Content-length:"

// Headers
static const char* userAgentHeadr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* connectionHeadr = "Connection: close\r\n";
static const char* proxyConnectionHeadr = "Proxy-Connection: close\r\n";
static const char* httpVersionHeadr = "HTTP/1.0\r\n";
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
    if (portNum <= MIN_PORT_NUMBER || portNum >= MAX_PORT_NUMBER) {
        printf("Invalid port number: %d. Should be between 1025 and 65535\n", portNum);
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

    // ip4addr.sin_family = AF_INET;
    // ip4addr.sin_port = htons(atoi(argv[1]));
    // ip4addr.sin_addr.s_addr = INADDR_ANY;

    // if ((listenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    //     perror("socket error");
    //     exit(EXIT_FAILURE);
    // }

    // if (bind(listenFd, (struct sockaddr*)&ip4addr, sizeof(struct sockaddr_in)) < 0) {
    //     close(listenFd);
    //     perror("bind error");
    //     exit(EXIT_FAILURE);
    // }

    // if (listen(listenFd, 100) < 0) {
    //     close(listenFd);
    //     perror("listen error");
    //     exit(EXIT_FAILURE);
    // }

    listenFd = Open_listenfd(port);
    while (1) {
        pthread_t tid;
        int* connfd = Malloc(sizeof(int));
        *connfd = -1;
        *connfd = Accept(listenFd, (SA *) &clientAddr, (socklen_t *) &clientLength);
        Pthread_create(&tid, NULL, (void *)proxyProcess, (void *) connfd);
	}


    printf("%s", userAgentHeadr);
    return 0;
}

void usage(char *str) {
	printf("\tusage: %s <port> [use cache]\n", str);
  printf("\t\tport: the port this program will connect to\n");
  printf("\t\tport: must be a number between 1025 and 65535\n");
	printf("\t\tcache: 'off' to turn off caching\n");
	printf("\t\tcache: 'on' to turn on caching\n");
	printf("\t\tcache is 'on' by default\n");
	exit(1);
}

void proxyProcess(int* arg) {
    printf("pid: %u\n", (unsigned int)Pthread_self());
    // Detach thread to avoid memory leaks
	  Pthread_detach(Pthread_self());

    int clientFd = (int)(*arg);
	  int serverFd = -1;
    char buf[MAXBUF], requestStr[MAXBUF], host[MAXBUF], port[MAXBUF], resource[MAXBUF];
    char cacheIdx[MAXBUF], content[MAXBUF];
    unsigned int length;

    // Read the client request
    int readVal = readRequest(requestStr, clientFd, host, port, cacheIdx, resource);

    printf("Read data from %s:%s\n",host,port);
	  fflush(stdout);

    if (!readVal) {
        // if (!read_node_content()) {

        // }
        // else {

        // }

        // Forward request to remote sever
        int serverResponseVal = forwardToServer(host, port, &serverFd, requestStr);
        if (serverResponseVal == -1) {
            fprintf(stderr, "forward content to server error.\n");
            strcpy(buf, connectionFailStr);
            Rio_writen(clientFd, buf, strlen(connectionFailStr));
        }
        else if (serverResponseVal == -2) {
            fprintf(stderr, "forward content to server error (dns look up fail).\n");
            strcpy(buf, dnsFailStr);
            Rio_writen(clientFd, buf, strlen(dnsFailStr));
        }
        else {
            //if (Rio_writen(server_fd, request_str, strlen(request_str)) == -1)
            //	fprintf(stderr, "forward content to server error.\n");

            int fVal = readAndForwardResponse(serverFd, clientFd, cacheIdx, content);
            if (fVal == -1)
                fprintf(stderr, "forward content to client error.\n");
            else if (fVal == -2)
                fprintf(stderr, "save content to cache error.\n");
        }
    }

}

int readRequest(char* request, int clientFd, char* host, char* port, char* cacheIdx, char* resource) {
    char buf[MAXBUF], method[MAXBUF], protocol[MAXBUF], hostPort[MAXBUF], version[MAXBUF];
     rio_t rioClient;

    Rio_readinitb(&rioClient, clientFd);
    if (Rio_readlineb(&rioClient, buf, MAXBUF) == -1) {
        return -1;
    }

    if (parseRequest(buf, method, protocol, hostPort, resource, version) == -1) {
        return -1;
    }

    getHostAndPort(hostPort, host, port);

    if (strstr(method, GET)) {
        strcpy(request, method);
        strcat(request, " ");
        strcat(request, resource);
        strcat(request, " ");
        strcat(request, httpVersionHeadr);

        if (strlen(host)) {
            strcpy(buf, "Host: ");
            strcat(buf, host);
            strcat(buf, ":");
            strcat(buf, port);
            strcat(buf, "\r\n");
            strcat(request, buf);
        }

        strcat(request, userAgentHeadr);
        strcat(request, acceptStr);
        strcat(request, acceptEncoding);
        strcat(request, connectionHeadr);
        strcat(request, proxyConnectionHeadr);

        while(Rio_readlineb(&rioClient, buf, MAXBUF) > 0) {
            //
            if (!strcmp(buf, END)) {
                strcat(request, END);
                break;
            }
            //
            else if (strstr(buf, USER_AGENT_KEY) || strstr(buf, ACCEPT_KEY) || strstr(buf, ACCEPT_ENCODE_KEY)
                           || strstr(buf, PROXY_KEY) || strstr(buf, CONNECTION_KEY) || strstr(buf, COOKIE_KEY) ) {
                    continue;
            }
            //
            else {
                strcat(request, buf);
            }
        }

        // Add caching information here
        // strcpy(cacheIdx, host);
        // strcat(cacheIdx, ":");
        // strcat(cacheIdx, port);
        // strcat(cacheIdx, resource);

        return 0;
    }

    return -1;
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
    *serverFd = Open_clientfd(host, port);

    if (*serverFd < 0) {
        // Something went wrong with connecting to server
        if (*serverFd == -1) {
            return -1;
        }
        // DNS lookup failed
        else {
            return -2;
        }
    }

    if (Rio_writen(*serverFd, requestStr, strlen(requestStr)) == -1) {
        return -1;
    }

    return 0;
}


int forwardToClient(int clientFd, char* content, unsigned int len) {
    if (Rio_writen(clientFd, content, len) == -1) {
        return -1;
    }

    return 0;
}


int readAndForwardResponse(int serverFd, int clientFd, char* cacheIdx, char* content) {
    rio_t rioServer;
    char buf[MAXBUF];
    unsigned int size =0, length = 0, cacheSize =0;
    int validSize = 1;

    content[0] = '\0';

    Rio_readinitb(&rioServer, serverFd);

    // Read
    do {
        if (Rio_readlineb(&rioServer, buf, MAXBUF) == -1) {
            return -1;
        }

        if (validSize) {
            validSize = append(content, buf, MAXBUF, &cacheSize);
        }

        if (strstr(buf, CONTENT_LEN_STR_2)) {
			sscanf(buf, "Content-length: %d", &size);
        }

		if (strstr(buf, CONTENT_LEN_STR_1)) {
			sscanf(buf, "Content-Length: %d", &size);
        }

        if (Rio_writen(clientFd, buf, length) == -1) {
            return -1;
        }

    } while (strcmp(buf, END) != 0 && strlen(buf));

    if (size) {
        while (size > MAXBUF) {
            if ((length = Rio_readnb(&rioServer, buf, MAXBUF)) == -1) {
                return -1;
            }

            if (validSize) {
                validSize = append(content, buf, MAXBUF, &cacheSize);
            }

            if (Rio_writen(clientFd, buf, length) == -1) {
                return -1;
            }

            size -= MAXBUF;
        }

        if (size) {
            if ((length = Rio_readnb(&rioServer, buf, size)) == -1) {
                return -1;
            }

            if (validSize) {
                validSize = append(content, buf, size, &cacheSize);
            }

            if (Rio_writen(clientFd, buf, length) == -1) {
                return -1;
            }
        }

    }
    else {

        while ((length = Rio_readnb(&rioServer, buf, MAXLINE)) > 0) {
            if (validSize) {
                validSize = append(content, buf, length, &cacheSize);
            }

            if (Rio_writen(clientFd, buf, length) == -1) {
                return -1;
            }
        }
    }

    if (validSize) {
        // Add to cache
        // if (insert_content_node(cache_list, cache_index, content, cache_size) == -1) {
		// 	return -2;
        // }
		// printf("insert correct!\n");
    }

    return 0;

}

void getHostAndPort(char* hostPort, char* host, char* port) {
    char* buf = strstr(hostPort,":");
	if (buf) {
		*buf = '\0';
		strcpy(port, buf + 1);
	}
	else {
		strcpy(port, DEFAULT_HTTP_PORT);
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
