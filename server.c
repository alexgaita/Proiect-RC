#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#define MYPORT 6000
#define BACKLOG 10
#define MAXDATASIZE 100

struct arg_struct
{
    int fileDescriptor;
    struct sockaddr_in address;
};

int receiveDataFromClient(int sockFd, char *compareString, char *host)
{

    char buffer[MAXDATASIZE];
    int numbytes;

    if ((numbytes = recv(sockFd, buffer, 10, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    buffer[numbytes] = '\0';

    // printf("Am citit: ^%s^---^%s^\n",buffer,compareString);
    printf("client-%s: %s\n", host, buffer);

    if (strcmp(buffer, compareString) != 0)
    {
        return 1;
    }
    return 0;
}

void sendToClient(int sockFd, char *buf)
{

    if (send(sockFd, buf, strlen(buf), 0) == -1)
        perror("send");
}

void *ClientServer(void *arguments)
{
    long tid;

    struct arg_struct *args = arguments;
    char *clientHost = inet_ntoa(args->address.sin_addr);
    int sockFd = args->fileDescriptor;
    char buf[MAXDATASIZE];

    int numbytes;

    printf("server: conexiune de la: %s\n", clientHost);

    if (receiveDataFromClient(sockFd, "SYN", clientHost))
    {
        printf("server: closing connection with: %s\n", clientHost);
        close(sockFd);
        pthread_exit(NULL);
    }

    sendToClient(sockFd, "SYN-ACK");

    if (receiveDataFromClient(sockFd, "ACK", clientHost))
    {
        printf("server: closing connection with: %s\n", clientHost);
        close(sockFd);
        pthread_exit(NULL);
    }

    srand(time(0));
    int r = rand() % 20;
    int i = 0;

    printf("After the %d message, client should send ACK\n", r);

    while (1)
    {
        if (!receiveDataFromClient(sockFd, "FIN", clientHost))
        {
            break;
        }
        i++;
        if (i == r)
        {
            sendToClient(sockFd, "Send ACK");
            while (1)
            {
                if (!receiveDataFromClient(sockFd, "ACK", clientHost))
                {
                    r = -1;
                    break;
                }
                sendToClient(sockFd, "Send ACK");
            }
        }

        sendToClient(sockFd, "ACK");
    }

    sendToClient(sockFd, "ACK FIN");

    receiveDataFromClient(sockFd,"ACK",clientHost);

    printf("server: closing connection with: %s\n", clientHost);
    close(sockFd);
    pthread_exit(NULL);
}

int main(void)
{
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;
    int yes = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    memset(&(my_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("Server listening on %s:%d ....\n", "localhost", MYPORT);

    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                             &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }

        int rc;
        pthread_t thread;

        struct arg_struct args;
        args.address = their_addr;
        args.fileDescriptor = new_fd;

        rc = pthread_create(&thread, NULL, ClientServer, (void *)&args);
        if (rc)
        {
            printf("Codul erorii este: %d\n", rc);
            exit(-1);
        }
    }

    return 0;
}
