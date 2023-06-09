#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 6000

#define MAXDATASIZE 100

char *sendDataToServer(int sockFd)
{
    char *buffer = malloc(10 * sizeof(char));

    memset(buffer, '\0', strlen(buffer));

    printf("Send message to server:   ");
    fflush(stdin);

    int ch;
    int i = 0;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
        buffer[i] = ch;
        i++;
        if (i == 9)
            break;
    }
    buffer[i] = '\0';

    if (send(sockFd, buffer, strlen(buffer), 0) == -1)
    {
        perror("send");
    }

    return buffer;
}

void receiveDataFromServer(int sockFd)
{
    char buffer[MAXDATASIZE];
    int numbytes;
    if ((numbytes = recv(sockFd, buffer, MAXDATASIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    if(numbytes == 0){
        printf("Wrong input from you :(\n");
        exit(1);
    }

    buffer[numbytes] = '\0';

    printf("server: %s\n", buffer);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ((he = gethostbyname("localhost")) == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(PORT);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&their_addr,
                sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    //SEND FIRST ACK
    sendDataToServer(sockfd);

    //RECEIVE SYN-ACK
    receiveDataFromServer(sockfd);

    //SEND ACK
    sendDataToServer(sockfd);

    while (1)
    {
        //START SENDING DATA
        char *newBuff = sendDataToServer(sockfd);
        printf("client: %s\n", newBuff);

        if (strcmp(newBuff, "FIN") == 0)
        {
            break;
        }

        //RECEIVE ACKS FROM SERVER
        receiveDataFromServer(sockfd);
    }
    
    receiveDataFromServer(sockfd);

    if (send(sockfd, "ACK", 3, 0) == -1)
    {
        perror("send");
    }

    printf("Inchid conexiunea cu localhost\n");

    close(sockfd);

    return 0;
}
