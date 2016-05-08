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

int prepare_line(char * line){
    return strnlen(line, MAX_MESSAGE_SIZE);
}

void clear_line(char * line){
    memset(line, '\0', strnlen(line, MAX_MESSAGE_SIZE));
}

int main (int argc, char *argv[]) {
    int rc;
    int sock;
    struct addrinfo addr_hints, *addr_result;
    char read_line[MAX_BUFF_SIZE];
    char* port;
    size_t line_size;

    /* Kontrola dokumentów ... */
    if (argc == 2){
        port = PORT_SIGN;
    }
    else if (argc == 3) {
        if (!is_port_number(argv[2])){
            fprintf(stderr, "ERROR: Usage: %s host [port]", argv[0]);
            exit(EXIT_FAILURE_PARAMS);
        }
        port = argv[2];
    } else {
        fprintf(stderr, "ERROR: Usage: %s host [port]", argv[0]);
        exit(EXIT_FAILURE_PARAMS);
    }

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        syserr("socket");
    }

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

    int action;
    int sockfd;
    int read_size;
    while(TRUE){
        memset(read_line, 0, MAX_BUFF_SIZE);
        action = poll(pfd, 2, 0);
        if(pfd[0].revents & POLLIN) { //stdin
            read_size = read(pfd[0].fd, read_line, MAX_MESSAGE_SIZE);
            unsigned short text_size = (unsigned short)find_string_end(read_line);
            if (text_size > 0){
//                printf("sending message to my fellas: -->");
                //printf("%.*s\n", text_size, read_line);

                //tworzę bufor do wysłania wiadomosci
                struct message * mess = malloc(sizeof(struct message) + text_size);
//                mess->lenght = htons(text_size);
//                memcpy(&mess->text, read_line, text_size);
                copy_message_into_struct(mess, text_size, read_line);
                int sent = write(pfd[1].fd, mess, sizeof(struct message) + text_size);
                free(mess);
                if (sent < 0){
                    perror("writing on stream socket");
                    exit(EXIT_FAILURE);
                } else if (sent < sizeof(struct message) + text_size){
                    //sth went wrong, end client with 130 code
                    if (close(pfd[1].fd) < 0)
                        perror("close");
                    exit(EXIT_FAILURE);
                }
            } else {
               // printf("Read empty line, doing nothing\n");
            }
        }
        clear_line(read_line);
        if(pfd[1].revents & POLLIN) {
            //from server: read and printf
            unsigned short message_size;
            //czytam liczbę
            int got;
            got = read_all(pfd[1].fd, (char*)&message_size, sizeof(message_size));
            if (got < 0)
                perror("reading from stream socket");
            else if (got < sizeof(message_size)){
                //sth went wrong, end client with 130 code
                if (close(pfd[1].fd) < 0)
                    perror("close");
                exit(EXIT_FAILURE);
            } else {

                message_size = ntohs(message_size);
                if (message_size > 0 && message_size <= MAX_MESSAGE_SIZE){
                    //czytam resztę wiadomości
                    int got = read_all(pfd[1].fd, read_line, message_size);
                    if (got < 0)
                        perror("reading from stream socket");
                    else if (got < message_size){
                        //sth went wrong, end client with 130 code
                        if (close(pfd[1].fd) < 0)
                            perror("close");
                        exit(EXIT_FAILURE);
                    } else {
//                        printf("my fella wrote: -->");
                        printf("%.*s\n", message_size, read_line);
//                        printf("otrzymano: %d\n", message_size);

                    }
                }
            }
        }

    }
    exit(EXIT_SUCCESS);
}
