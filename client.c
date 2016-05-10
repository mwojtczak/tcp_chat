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

struct pollfd pfd[2];

int prepare_line(char *line) {
    return strnlen(line, MAX_MESSAGE_SIZE);
}

void clear_line(char *line) {
    memset(line, '\0', strnlen(line, MAX_MESSAGE_SIZE));
}

char * check_params(int argc, char *argv[]){
    if (argc == 2) {
        return PORT_SIGN;
    }
    else if (argc == 3) {
        if (!is_port_number(argv[2])) {
            fprintf(stderr, "ERROR: Usage: %s host [port]", argv[0]);
            exit(EXIT_FAILURE_PARAMS);
        }
        return argv[2];
    } else {
        fprintf(stderr, "ERROR: Usage: %s host [port]", argv[0]);
        exit(EXIT_FAILURE_PARAMS);
    }
}

void check_errors(int data, char * error_message){
    if (data < 0) {
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

void send_to_server(struct pollfd pfd, char * read_line, unsigned short text_size){
    struct message * mess = malloc(sizeof(struct message) + text_size);
    copy_message_into_struct(mess, text_size, read_line);
    int sent = write(pfd.fd, mess, sizeof(struct message) + text_size);
    free(mess);
    if (sent < 0) {
        perror("writing on stream socket");
        exit(EXIT_FAILURE);
    } else if (sent < sizeof(struct message) + text_size) {
        fprintf(stderr, "Ending connection, some trouble sending message.\n");
        if (close(pfd.fd) < 0)
            perror("close");
        exit(EXIT_FAILURE);
    }
}

void try_reading_from_stdin(struct pollfd pfd, struct pollfd server, char * read_line){
    if (pfd.revents & POLLIN) {
        unsigned short read_size = read(pfd.fd, read_line, MAX_MESSAGE_SIZE);
        if (read_size < 0){
            perror("reading from stdin");
            exit(EXIT_FAILURE);
        }
        read_size = (unsigned short) find_string_end(read_line);
        if (read_size > 0) {
            send_to_server(server, read_line, read_size);
        }
    }
}

void try_reading_from_socket(struct pollfd pfd, char * read_line){
    if (pfd.revents & POLLIN) {
        unsigned short message_size;
        int got = read_all(pfd.fd, (char *) &message_size, sizeof(message_size));
        if (got < 0) {
            perror("reading from stream socket");
            exit(EXIT_FAILURE);
        } else if (got < sizeof(message_size)) {
            fprintf(stderr, "Ending connection, some trouble reading message.\n");
            if (close(pfd.fd) < 0)
                perror("close");
            exit(EXIT_FAILURE);
        } else {
            message_size = ntohs(message_size);
            if (message_size > 0 && message_size <= MAX_MESSAGE_SIZE) {
                got = read_all(pfd.fd, read_line, message_size);
                if (got < 0)
                    perror("reading from stream socket");
                else if (got < message_size) {
                    fprintf(stderr, "Ending connection, some trouble while reading message.\n");
                    if (close(pfd.fd) < 0)
                        perror("close");
                    exit(EXIT_FAILURE_CLIENT);
                } else {
                    printf("%.*s\n", message_size, read_line);
                }
            } else {
                fprintf(stderr, "Wrong size of data.\n");
                exit(EXIT_FAILURE_CLIENT);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int rc, sock, action;
    struct addrinfo addr_hints, *addr_result;
    char read_line[MAX_BUFF_SIZE];
    char *port;

    port = check_params(argc, argv);

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        syserr("socket");
    }

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_flags = 0;
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    rc = getaddrinfo(argv[1], port, &addr_hints, &addr_result);

    if (rc != 0) {
        syserr("getaddrinfo: %s", gai_strerror(rc));
    }

    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) != 0) {
        syserr("connect");
    }
    freeaddrinfo(addr_result);

    pfd[0].fd = fileno(stdin);
    pfd[0].events = POLLIN;
    pfd[1].fd = sock;
    pfd[1].events = POLLIN | POLLOUT;

    while (TRUE) {
        memset(read_line, 0, MAX_BUFF_SIZE);
        action = poll(pfd, 2, 0);
        if (action < 0){
            perror("poll");
            exit(EXIT_FAILURE);
        }
        try_reading_from_stdin(pfd[0], pfd[1], read_line);
        clear_line(read_line);
        try_reading_from_socket(pfd[1], read_line);

    }
    exit(EXIT_SUCCESS);
}
