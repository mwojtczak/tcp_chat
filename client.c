#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "message.h"
#include "err.h"

#define TRUE             1
#define FALSE            0
#define PORT_NUMBER      20160

#define PORT_SIGN        "20160"
#define BUFFER_SIZE      1001


static const char bye_string[] = "BYE";

int prepare_line(char * line){
    return strnlen(line, MAX_MESSAGE_SIZE);
}

void clear_line(char * line){
    memset(line, '\0', BUFFER_SIZE);
}

int main (int argc, char *argv[]) {
    int rc;
    int sock;
    struct addrinfo addr_hints, *addr_result;
    char line[BUFFER_SIZE];
    char read_line[BUFFER_SIZE];
    char* port;
    size_t line_size;
    int maxi = 1;

    /* Kontrola dokumentów ... */
    if (argc == 2){
        port = PORT_SIGN;
    }
    else if (argc == 3) {
        port = argv[2];
    } else {
        fatal("Usage: %s host port", argv[0]);
    }
    printf("%s\n", port);

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        syserr("socket");
    }

//



    /* Trzeba się dowiedzieć o adres internetowy serwera. */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_flags = 0;
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    /* I tak wyzerowane, ale warto poznać pola. */
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    rc =  getaddrinfo(argv[1], port, &addr_hints, &addr_result);
    if (rc != 0) {
        fprintf(stderr, "rc=%d\n", rc);
        syserr("getaddrinfo: %s", gai_strerror(rc));
    }

    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) != 0) {
        syserr("connect");
    }
    freeaddrinfo(addr_result);

    struct pollfd pfd[2]; //dla stdin i klienta
    pfd[0].fd = fileno(stdin);
    pfd[0].events = POLLIN;
    pfd[1].fd = sock; //server bind, listen socket
    pfd[1].events = POLLIN | POLLOUT;


//    while (TRUE){
//        fgets(line, sizeof line, stdin);
//        line_size = prepare_line(line); //strlen(line);
//        if (write(sock, line, line_size) < 0)
//            perror("writing on stream socket");
//    }

//    //chyba działa jak chcę: wypisuje na stdout co otrzymał, zamyka gdy CTR+D
//    while (fgets(line, sizeof line, stdin) != NULL){
//        if (feof(stdin)){
//            printf("ending\n");
//            break;
//        }
//        printf("got: ");
//        printf(line);
//        //write to server
//
//    }

//    while (TRUE){
//        if (fgets(line, sizeof line, stdin) != NULL){
//
//            if (feof(stdin)){
//                printf("ending\n");
//                break;
//            }
//            printf("got: ");
//            printf(line);
//            if (write(sock, line, strlen (line)) < 0)
//                perror("writing on stream socket");
//            //write to server
//
//        } else {
//            printf("NULL W FGETS\n");
//        }
//        ssize_t rval = read(sock, read_line, BUFFER_SIZE);
//        if (rval == 0) {
//            fprintf(stderr, "Ending connection\n");
//        } else{
//            printf("rval != 0\n");
//            printf("-->%.*s\n", (int)rval, read_line);
//        }
//    }
//    do {
//        printf("line:");
//        char * test = fgets(line, sizeof line, stdin);
//        printf(line);
////        if (write(sock, line, strlen (line)) < 0)
////            perror("writing on stream socket");
//    }
//    while (test != NULL && feof(stdin));
//    while (strncmp(line, bye_string, sizeof bye_string - 1));
//    if (close(sock) < 0)
//        perror("closing stream socket");

    int action;
    int sockfd;
    int read_size;
    while(TRUE){
        clear_line(read_line);
        action = poll(pfd, maxi + 1, 0);
        if(pfd[0].revents & POLLIN) { //stdin
            //process input and perform command
            read_size = read(pfd[0].fd, read_line, MAX_MESSAGE_SIZE);
            printf("sending message to my fellas: -->%.*s\n", read_size, read_line);

//            if (write(pfd[1].fd, read_line, read_size) < 0)
//                perror("writing on stream socket");
            printf("wysyłam do kolegów: %d\n", read_size);
            //creating message:
            struct message message_to_send;
            message_to_send.lenght = htons(read_size);
//            message_to_be_send.text = read_line;

            //tworzę bufor do wysłania wiadomosci
            struct message * mess = NULL;
            mess = malloc(sizeof(struct message) + read_size);
            mess->lenght = htons(read_size);
            memcpy(&mess->text, read_line, read_size);

//            if (write(pfd[1].fd, &message_to_send, sizeof(message_to_send)) < 0)
            if (write(pfd[1].fd, mess, sizeof(struct message) + read_size) < 0)
                perror("writing on stream socket");

            free(mess);
        }
        clear_line(read_line);
        if(pfd[1].revents & POLLIN) {
            //from server: read and printf
            //better version:
            unsigned short message_size;
            //czytam liczbę
            if (read(pfd[1].fd, (char*)&message_size, sizeof(message_size)) < 0)
                perror("reading from stream socket");
            message_size = ntohs(message_size);
            //czytam resztę wiadomości
            if (read(pfd[1].fd, read_line, message_size) < 0)
                perror("reading from stream socket");
            printf("my fella wrote: -->%.*s\n", strnlen(read_line, message_size), read_line);
            //printf("my fella wrote: -->%.*s\n", strnlen(read_line, message_size), read_line);
            printf("otrzymano: %d\n", message_size);

            //what works fine rof stream now
//            if (read(pfd[1].fd, read_line, MAX_MESSAGE_SIZE) < 0)
//                perror("reading from stream socket");
////            printf("my fella wrote: %s .\n",read_line);
//            printf("my fella wrote: -->%.*s\n", strnlen(read_line, MAX_MESSAGE_SIZE), read_line);


        }

    }
    printf("ENDING\n");
    return 0;
}

/*EOF*/
/*
 *
 */