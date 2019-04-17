/**
 * Tanner Davis - BYU CS 324 Winter 2019
 * 
 */ 
#include "csapp.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include "cache.c"
#include "sbuf.c"

/* Recommended max cache and object contentLengths */
#define MAX_CACHE_SIZE       1049000
#define MAX_OBJECT_SIZE      102400
#define MAXBUF               8192
#define MAX_EVENTS          25
#define QUEUE_SIZE           24
#define MIN_PORT_NUMBER      1024
#define MAX_PORT_NUMBER     65536
#define TIMEOUT             1000000
#define CACHE_OFF              "off"
#define FORWARD_SLASH          "/"
#define HTTP_DELIM             "://"
#define END_STR                '\0'
#define DEFAULT_HTTP_PORT      "80"
#define GET                    "GET"
#define END_HTTP               "\r\n"
#define END_HEADERS           "\r\n\r\n"
#define HOST_KEY               "Host:"
#define USER_AGENT_KEY         "User-Agent:"
#define ACCEPT_KEY             "Accept:"
#define ACCEPT_ENCODE_KEY      "Accept-Encoding:"
#define CONNECTION_KEY         "Connection:"
#define PROXY_KEY              "Proxy Connection:"
#define COOKIE_KEY             "Cookie:"
#define CONTENT_LEN_STR_1      "Content-Length"
//#define CONTENT_LEN_KEY_1    "Content-Length:"
#define CONTENT_LEN_STR_2      "Content-length"
//#define CONTENT_LEN_KEY_2    "Content-length:"

// Headers
static const char* userAgentHeadr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* connectionHeadr = "Connection: close\r\n";
static const char* proxyConnectionHeadr = "Proxy-Connection: close\r\n";
static const char* httpVersionHeadr = "HTTP/1.0\r\n";
static const char* acceptStr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char* acceptEncoding = "Accept-Encoding: gzip, deflate\r\n";
static const char* dnsFailStr = "HTTP/1.0 400 \
    Bad Request\r\nServer: CS324-Proxy\r\nContent-Length: 0\r\nConnection: \
    close\r\nContent-Type: text/html\r\n\r\n";
    /* <html><head></head><body><p>\
     Couldn't connect with this server because of DNS issues.</p></body></html> */
static const char* connectionFailStr = "HTTP/1.0 400 \
    Bad Request\r\nServer: CS324-Proxy\r\nContent-Length: 0\r\nConnection: \
    close\r\nContent-Type: text/html\r\n\r\n";
    /* <html><head></head><body><p>\
    Couldn't connect with this server.</p></body></html> */

// Struct for keeping track of a request's state
typedef struct requestState {
    int clientFd;
    int serverFd;
    char* requestBuf; // Buffer for original request
    char* proxyRequestBuf; // Buffer for proxy-made request
    char* responseBuf; // Buffer for server response or cache
    size_t bytesReadFromClient;
    size_t bytesToSendToServer;
    size_t bytesSentToServer;
    size_t bytesReadFromServer;
    size_t bytesSentToClient;
} requestStateType;

// Struct for callback functions and their arguments
struct eventAction {
	int (*callback)(requestStateType*);
	void *arg;
};
int efd; // epoll file descriptor

cacheList* cache; // Here be the cache
volatile sig_atomic_t exitRequested = 0; // SIGINT flag

// Function declarations
void usage();
int openListenfd(char *port);

int readFromClient(requestStateType* state);
int handleNewClient(requestStateType* state);
int writeToServer(requestStateType* state);
int readFromServer(requestStateType* state);
int writeToClient(requestStateType* state);

void logInfo(char* buf);
void proxyThread(void* arg);
int readRequest(char* clientRequest, char* proxyReq, char* host, char* port, char* cacheKey, char* query);
int forwardToServer(char* host, char* port, int* serverFd, char* requestStr);
int forwardToClient(int clientFd, char* response, unsigned int len);
int readAndForwardToClient(int serverFd, int clientFd, char* cacheKey, char* response);

int append(char* response, char* str, unsigned int addSize, unsigned int* prevSize);
int parseRequest(char* buf, char* method, char* protocol, char* version, char* hostAndPort, char* query);
void getHostAndPort(char* hostAndPort, char* host, char* port);
void closeFds(int* clientFd, int* serverFd);
// END function declarations

int main(int argc, char** argv) {
    char* port;
    int useCache = 1;
    struct epoll_event event;
	struct epoll_event *events;
    struct eventAction *ea;
    struct requestState *argptr; // Argument for the callback
    int listenFd;
    int i; // For iterating the events when one occurs
    size_t n; // Number of events while waiting

    // Incorrect number of arguments
    if (argc < 2) {
        usage(argv[0]);
    }

    // Decide if the cache should be used
    if (argc >= 3) {
        useCache = (strcmp(argv[2], CACHE_OFF));
    }

    // Make sure the port is valid    
    port = argv[1];
    int portNum = atoi(port);
    if (portNum <= MIN_PORT_NUMBER || portNum >= MAX_PORT_NUMBER) {
        printf("Invalid port number: %d. Should be between 1025 and 65535\n", portNum);
        exit(1);
    }

    // Initialize cache
    cache = NULL;
    if (useCache) {
        printf("Using cache\n");
        cache = initCache();
    }
    else {
        printf("Not using cache\n");
    }

    // Ignore SIGPIPE
    //Signal(SIGPIPE, SIG_IGN);


    listenFd = openListenfd(port);
    // Set listenFd to be non-blocking
    if (fcntl(listenFd, F_SETFL, fcntl(listenFd, F_GETFL) | O_NONBLOCK)) {
        fprintf(stderr, "error setting listening socket\n");
		exit(1);
    }

    // Create an epoll file descriptor
    if ((efd = epoll_create1(0)) < 0) {
		fprintf(stderr, "error creating epoll fd\n");
		exit(1);
	}

    ea = malloc(sizeof(struct eventAction));
    ea->callback = handleNewClient;
    argptr = malloc(sizeof(requestStateType));
    argptr->clientFd = listenFd;

    ea->arg = argptr;
    event.data.ptr = ea;
    event.events = EPOLLIN | EPOLLET; // edge-triggered monitoring
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listenFd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(1);
	}

    // Buffer where events are returned
	events = calloc(MAX_EVENTS, sizeof(event));

    // Wait for new client connection events to happen
    while (1) {        
		n = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (n == 0) {
            // Check SIGTERM flag
            // Else
            continue;
        }
        else if (n < 0) {
            // An error occurred
            fprintf(stderr, "An error occurred while waiting...\n");
            continue;
        }
        else {
            // Iterate through the events and handle them
            for (i = 0; i < n; i++) {
                // Get the callback function and argument for the event
                ea = (struct eventAction *)events[i].data.ptr;
                argptr = ea->arg;

                // Check for errors
                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                    /* An error has occured on this fd */
                    fprintf (stderr, "epoll error on fd %d\n", listenFd);
                    close(listenFd);
                    free(ea->arg);
                    free(ea);
                    continue;
                }

                // Call the callback function
                if (!ea->callback(argptr)) {
                    close(listenFd);
                    free(ea->arg);
                    free(ea);
                }

            }
        }
	}

    
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

void logInfo(char* buf) {

    FILE* logFile = fopen("requests.txt", "a+");

    if (logFile == NULL) {
        fprintf(stderr, "error opening log file");
    }

    if (fwrite(buf, strlen(buf), 1, logFile) <= 0) {
        fprintf(stderr, "error writing to log file!\n");
    }

    fclose(logFile);
}

int handleNewClient(requestStateType* state) {
	socklen_t clientlen;
	int clientFd;
	struct sockaddr_storage clientaddr;
	struct epoll_event event;
	struct eventAction *ea;
    struct requestState *newReqState;

	clientlen = sizeof(struct sockaddr_storage); 

	// loop and get all the connections that are available
    // ***Even though this says state->clientFd it is the proxy's listenFd.***
	while ((clientFd = accept(state->clientFd, (struct sockaddr*)&clientaddr, &clientlen)) > 0) {

		// set fd to non-blocking (set flags while keeping existing flags)
		if (fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL, 0) | O_NONBLOCK) < 0) {
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}

        // Initialize a new request state
        newReqState = malloc(sizeof(struct requestState));
        newReqState->clientFd = clientFd;
        newReqState->requestBuf = malloc(MAX_OBJECT_SIZE);
        newReqState->proxyRequestBuf = malloc(MAX_OBJECT_SIZE);
        newReqState->responseBuf = malloc(MAX_OBJECT_SIZE);
        newReqState->bytesReadFromClient = 0;
        newReqState->bytesToSendToServer = 0;
        newReqState->bytesSentToServer = 0;
        newReqState->bytesReadFromServer = 0;
        newReqState->bytesSentToClient = 0;

        // Initialize the new event action
		ea = malloc(sizeof(struct eventAction));
        // Next action: Read from client
		ea->callback = readFromClient;

		// add a read event to epoll file descriptor
		ea->arg = newReqState;
		event.data.ptr = ea;
		event.events = EPOLLIN | EPOLLET; // use edge-triggered monitoring
		if (epoll_ctl(efd, EPOLL_CTL_ADD, clientFd, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}
	}

	if (errno == EWOULDBLOCK || errno == EAGAIN) {
		// no more clients to accept()
		return 1;
	} else {
		perror("error accepting");
		return 0;
	}
}

int readFromClient(requestStateType* state) {
	int numBytesRead;
    // Read from the client and keep track of the offset in case the request doesn't come all at once
	while ((numBytesRead = read(state->clientFd, (state->requestBuf + state->bytesReadFromClient), MAXLINE)) > 0) {
		printf("Received %d bytes\n", numBytesRead);
        state->bytesReadFromClient += numBytesRead;
	}

    if (strstr(state->requestBuf, END_HEADERS) != NULL) {
        // Ready to parse the request
        state->serverFd = -1;
        char errorBuf[MAXBUF], host[MAXBUF], port[MAXBUF], query[MAXBUF], cacheKey[MAXBUF];

        int readVal = readRequest(state->requestBuf, state->proxyRequestBuf, host, port, cacheKey, query);
        if (!readVal) {
            struct epoll_event event;
	        struct eventAction *ea;
            state->bytesToSendToServer = strlen(state->proxyRequestBuf);

            if (readNodeContent(cache, cacheKey, state->responseBuf, state->bytesReadFromServer) == 0) {
                printf("Found in cache!\n");

                // Initialize the new event action
                ea = malloc(sizeof(struct eventAction));
                // Next action: Read from client
                ea->callback = writeToClient;

                // Prep for writing to client
                ea->arg = state;
                event.data.ptr = ea;
                event.events = EPOLLOUT | EPOLLET; // use edge-triggered monitoring
                if (epoll_ctl(efd, EPOLL_CTL_MOD, state->serverFd, &event) < 0) {
                    fprintf(stderr, "error adding event\n");
                    exit(1);
                }

                // int forwardVal = forwardToClient(state->clientFd, state->responseBuf, state->bytesReadFromServer);
                // if (forwardVal == -1) {
                //     fprintf(stderr, "Forward CACHE response to client error\n");
                // }
            }
            else {                
                // Create server file descriptor
                state->serverFd = Open_clientfd(host, port);

                if (state->serverFd < 0) {
                    // Something went wrong with connecting to server
                    if (state->serverFd == -1) {
                        return -1;
                    }
                    // DNS lookup failed
                    else {
                        return -2;
                    }
                }

                // Set serverFd to be non-blocking
                if (fcntl(state->serverFd, F_SETFL, fcntl(state->serverFd, F_GETFL) | O_NONBLOCK)) {
                    fprintf(stderr, "error setting server socket to non-blocking\n");
                    exit(1);
                }

                // Initialize the new event action
                ea = malloc(sizeof(struct eventAction));
                // Next action: Read from client
                ea->callback = writeToServer;

                // Prep for writing to server
                ea->arg = state;
                event.data.ptr = ea;
                event.events = EPOLLOUT | EPOLLET; // use edge-triggered monitoring
                if (epoll_ctl(efd, EPOLL_CTL_ADD, state->serverFd, &event) < 0) {
                    fprintf(stderr, "error adding event\n");
                    exit(1);
                }
            }
        }
    }

    if (numBytesRead < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        // no more data to read()
        return 1;
	}
    else {
		perror("error reading");
		return 0;
	}
    return 0;
}

int writeToServer(requestStateType* state) {
    int numBytesWritten = -1;
    while ((numBytesWritten = write(state->serverFd, state->proxyRequestBuf, strlen(state->proxyRequestBuf))) < state->bytesToSendToServer) {
    }
    state->bytesSentToServer = numBytesWritten;

    if (state->bytesToSendToServer == state->bytesSentToServer) {
        struct epoll_event event;
	    struct eventAction *ea;
        // Initialize the new event action
        ea = malloc(sizeof(struct eventAction));
        // Next action: Read from client
        ea->callback = readFromServer;

        // Prep for reading from server
        ea->arg = state;
        event.data.ptr = ea;
        event.events = EPOLLIN | EPOLLET; // use edge-triggered monitoring
        if (epoll_ctl(efd, EPOLL_CTL_MOD, state->serverFd, &event) < 0) {
            fprintf(stderr, "error adding event\n");
            exit(1);
        }
    }
    
    if (numBytesWritten == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        // no more data to read()
        return 1;
	}
    else if (numBytesWritten == -1 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
		perror("error reading");
		return 0;
	}

    return 1;
}

int readFromServer(requestStateType* state) {
    int numBytesRead;
    // Read from the client and keep track of the offset in case the request doesn't come all at once
	while ((numBytesRead = read(state->serverFd, (state->responseBuf + state->bytesReadFromServer), MAXLINE)) > 0) {
		printf("Received %d bytes\n", numBytesRead);
        state->bytesReadFromServer += numBytesRead;
	}


    if (numBytesRead < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        // no more data to read()
        return 1;
	}
    else {
		perror("error reading");
		return 0;
	}
    return 0;
}

int writeToClient(requestStateType* state) {
    printf("WRITING TO CLIENT!");
    return 0;
}


void proxyThread(void* arg) {    
    //printf("pid: %u\n", (unsigned int) pthread_self());
    while (exitRequested == 0) {
        int clientFd = 1;
        int serverFd = -1;
        char buf[MAXBUF], requestStr[MAXBUF], host[MAXBUF], port[MAXBUF], query[MAXBUF];
        char cacheKey[MAXBUF], response[MAX_OBJECT_SIZE];
        unsigned int length;

        // Read the client request
        int readVal = 0;  //readRequest(requestStr, clientFd, bufferedReq, host, port, cacheKey, query);

        //printf("Read data from %s:%s\n",host,port);
        fflush(stdout);

        if (!readVal) {
            if (readNodeContent(cache, cacheKey, response, &length) == 0) {
                printf("Found in cache!\n");
                int forwardVal = forwardToClient(clientFd, response, length);
                if (forwardVal == -1) {
                    fprintf(stderr, "Forward CACHE response to client error\n");
                }
                //Close(clientFd);
            }
            else {
                // Forward request to remote sever
                int bytesWritten = forwardToServer(host, port, &serverFd, requestStr);
                if (bytesWritten == -1) {
                    fprintf(stderr, "forward response to server error.\n");
                    strcpy(buf, connectionFailStr);
                    Rio_writen(clientFd, buf, strlen(connectionFailStr));
                }
                else if (bytesWritten == -2) {
                    fprintf(stderr, "forward response to server error (dns look up fail).\n");
                    strcpy(buf, dnsFailStr);
                    Rio_writen(clientFd, buf, strlen(dnsFailStr));
                }
                else {
                    int forwardVal = readAndForwardToClient(serverFd, clientFd, cacheKey, response);
                    if (forwardVal == -1)
                        fprintf(stderr, "forward response to client error\n");
                    else if (forwardVal == -2)
                        fprintf(stderr, "save response to cache error\n");
                }
            }
        }

        closeFds(&clientFd, &serverFd);

        // Break out only on SIGINT
    }
    return;
}


int readRequest(char* clientRequest, char* proxyReq, char* host, char* port, char* cacheKey, char* query) {
    char buf[MAXBUF], method[MAXBUF], protocol[MAXBUF], hostAndPort[MAXBUF], version[MAXBUF];
    //  rio_t rioClient;

    // Rio_readinitb(&rioClient, clientFd);
    

    if (parseRequest(clientRequest, method, protocol, version, hostAndPort, query) == -1) {
        return -1;
    }

    getHostAndPort(hostAndPort, host, port);

    // Begin copying the request
    // GET requests
    if (strstr(method, GET)) {

        strcpy(proxyReq, method);
        strcat(proxyReq, " ");
        strcat(proxyReq, query);
        strcat(proxyReq, " ");
        strcat(proxyReq, httpVersionHeadr);

        // Add Host information if present
        if (strlen(host)) {
            strcpy(buf, HOST_KEY);
            strcat(buf, host);
            strcat(buf, ":");
            strcat(buf, port);
            strcat(buf, END_HTTP);
            strcat(proxyReq, buf);
        }

        // Add necessary headers
        strcat(proxyReq, userAgentHeadr);
        strcat(proxyReq, acceptStr);
        strcat(proxyReq, acceptEncoding);
        strcat(proxyReq, connectionHeadr);
        strcat(proxyReq, proxyConnectionHeadr);


        // Copy any other relavant headers
        // while(Rio_readlineb(&rioClient, buf, MAXBUF) > 0) {
        //     // End when you see '\r\n'
        //     if (!strcmp(buf, END_HTTP)) {
        //         strcat(request, END_HTTP);
        //         break;
        //     }
        //     // Ignore these headers, we overwrote them
        //     else if (strstr(buf, USER_AGENT_KEY) || strstr(buf, ACCEPT_KEY) || strstr(buf, ACCEPT_ENCODE_KEY)
        //             || strstr(buf, PROXY_KEY) || strstr(buf, CONNECTION_KEY) || strstr(buf, COOKIE_KEY)
        //             || strstr(buf, HOST_KEY)) {
        //             continue;
        //     }
        //     else {
        //         strcat(request, buf);
        //     }
        // }

        // Add caching information here
        strcpy(cacheKey, host);
        strcat(cacheKey, ":");
        strcat(cacheKey, port);
        strcat(cacheKey, query);

        // TODO Log it as well
        char* httpPrefix = "http://";
        char* logBuf = Malloc(sizeof(char) * (strlen(cacheKey) + strlen(httpPrefix) + strlen("\n")));
        strcpy(logBuf, httpPrefix);
        strcat(logBuf, host);
        strcat(logBuf, ":");
        strcat(logBuf, port);
        strcat(logBuf, query);
        strcat(logBuf, "\n");
        return 0;
    }

    return -1;
}

int parseRequest(char* buf, char* method, char* protocol, char* version, char* hostAndPort, char* query) {
	char url[MAXBUF];

    //  Make sure it is a valid request
	if((!strstr(buf, FORWARD_SLASH)) || !strlen(buf)) {
		return -1;
    }

    // Extract the method, url and http version
	sscanf(buf,"%s %s %s", method, url, version);

    // Prepend the '/' to the query
    strcpy(query, FORWARD_SLASH);
	if (strstr(url, HTTP_DELIM)) {
        // Extract the http protocol, host:port and query
		sscanf(url, "%[^:]://%[^/]%s", protocol, hostAndPort, query);
    }
	else {
        // No protocol mentioned
		sscanf(url, "%[^/]%s", hostAndPort, query);
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

    int numBytes = Rio_writen(*serverFd, requestStr, strlen(requestStr));
    if (numBytes == -1) {
        return -1;
    }

    return numBytes;
}


int forwardToClient(int clientFd, char* response, unsigned int len) {
    if (Rio_writen(clientFd, response, len) == -1) {
        return -1;
    }

    return 0;
}


int readAndForwardToClient(int serverFd, int clientFd, char* cacheKey, char* response) {
    rio_t rioServer;
    char buf[MAXBUF];
    unsigned int contentLength = 0, cacheSize = 0;
    int validReponseLength = 1;

    response[0] = END_STR;

    Rio_readinitb(&rioServer, serverFd);

    do {
        int numBytesRead = Rio_readlineb(&rioServer, buf, MAXBUF);
        // Read the response from the server line by line
        if (numBytesRead == -1) {
            return -1;
        }

        // Append the read bytes to the proxy's response
        if (validReponseLength) {
            validReponseLength = append(response, buf, strlen(buf), &cacheSize);
        }

        // Standardize the content length header
        if (strstr(buf, CONTENT_LEN_STR_1) || strstr(buf, CONTENT_LEN_STR_2)) {
			sscanf(buf, "Content-length: %d", &contentLength);
        }

        // Write the read bytes to the client
        if (Rio_writen(clientFd, buf, strlen(buf)) == -1) {
            return -1;
        }

    } while (strcmp(buf, END_HTTP) != 0 && strlen(buf));


    // Write the response body (if any) to the client
    unsigned int lengthToWrite = 0;
    if (contentLength) {
        // If the content length is more than MAXBUF keep writing until it is not
        while (contentLength > MAXBUF) {
            if ((lengthToWrite = Rio_readnb(&rioServer, buf, MAXBUF)) == -1) {
                return -1;
            }

            // Append to response
            if (validReponseLength) {
                validReponseLength = append(response, buf, MAXBUF, &cacheSize);
            }

            // Write to client
            if (Rio_writen(clientFd, buf, lengthToWrite) == -1) {
                return -1;
            }

            // Decrease the length of the content left to write
            contentLength -= MAXBUF;
        }

        // Finish writing anything that is leftover from the loop above
        // Bascially anything less than MAXBUF
        if (contentLength) {
            if ((lengthToWrite = Rio_readnb(&rioServer, buf, contentLength)) == -1) {
                return -1;
            }

            if (validReponseLength) {
                validReponseLength = append(response, buf, contentLength, &cacheSize);
            }

            if (Rio_writen(clientFd, buf, lengthToWrite) == -1) {
                return -1;
            }
        }

    }
    else {

        while ((lengthToWrite = Rio_readnb(&rioServer, buf, MAXBUF)) > 0) {
            if (validReponseLength) {
                validReponseLength = append(response, buf, lengthToWrite, &cacheSize);
            }

            if (Rio_writen(clientFd, buf, lengthToWrite) == -1) {
                return -1;
            }
        }
    }

    if (validReponseLength) {
        // Add to cache
        int addToCacheVal = insertNodeContent(cache, cacheKey, response, cacheSize);
        if (addToCacheVal == -1) {
            printf("Cache error!\n");
			return -2;
        }
        else if (addToCacheVal == -2) {
            printf("Not enough space in cache!\n");
            return -2;
        }
        else {
            printf("Added response to cache!\n");
        }
    }

    return 0;

}

void getHostAndPort(char* hostAndPort, char* host, char* port) {
    // Split the host and port into "seperate" strings
    char* buf = strstr(hostAndPort,":");
	if (buf) {
		*buf = END_STR;
        // Copy the port
		strcpy(port, buf + 1);
	}
	else {
        // Give the default port of '80'
		strcpy(port, DEFAULT_HTTP_PORT);
    }

    // Copy the host
	strcpy(host, hostAndPort);
}

int append(char* response, char* str, unsigned int addSize, unsigned int* prevSize) {
    if(addSize + (*prevSize) > MAX_OBJECT_SIZE) {
        return 0;
    }

	memcpy(response + (*prevSize), str, addSize);
	*prevSize += addSize;
	return 1;
}

void closeFds(int* clientFd, int* serverFd) {
    if(clientFd && *clientFd >= 0) {
		Close(*clientFd);
    }

	if(serverFd && *serverFd >= 0) {
		Close(*serverFd);
    }
}

int openListenfd(char *port)
{
    struct sockaddr_in ip4Addr;
    int listenfd;

    /* Get a list of potential server addresses */
    // memset(&ip4Addr, 0, sizeof(struct sockaddr_in));
    ip4Addr.sin_family = AF_INET;
    ip4Addr.sin_port = htons(atoi(port));
    ip4Addr.sin_addr.s_addr = INADDR_ANY;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("listenfd socket error");
      return -1;
    }

    if (bind(listenfd, (struct sockaddr*)&ip4Addr, sizeof(struct sockaddr_in)) < 0) {
      close(listenfd);
      perror("listenfd bind error");
      return -1;
    }

    fprintf(stderr, "listenfd bound to port %s\n", port);

    if (listen(listenfd, 100) < 0) {
      close(listenfd);
      perror("listenfd listen error");
      return -1;
    }

    return listenfd;
}
