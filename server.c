#include <stdio.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include "message.h"
#include "err.h"

static int finish = FALSE;

/* Obsługa sygnału kończenia */
static void catch_int(int sig) {
    finish = TRUE;
    fprintf(stderr,
            "Signal %d catched. No new connections will be accepted.\n", sig);
}

int main(int argc, char *argv[]) {
    struct pollfd client[_POSIX_OPEN_MAX]; //@TODO: max_clients + 1??
    struct sockaddr_in server;
    char buf[MAX_BUFF_SIZE];
    size_t length;
    ssize_t rval;
    int msgsock, activeClients, i, j, ret, received;
    unsigned short message_size;
    char *port = PORT_SIGN;
    struct message *mess = NULL;

//check params
    if (argc == 2) {
        port = argv[1];
    } else if (argc != 1) {
        fprintf(stderr, "ERROR: Usage: %s port or without any parameters\n", argv[0]);
        exit(EXIT_FAILURE_PARAMS);
    }

    /* Po Ctrl-C kończymy */
    if (signal(SIGINT, catch_int) == SIG_ERR) {
        perror("Unable to change signal handler");
        exit(EXIT_FAILURE);
    }

    /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
    for (i = 0; i < _POSIX_OPEN_MAX; ++i) { //@TODO: max_clients + 1??
        client[i].fd = -1;
        client[i].events = POLLIN | POLLOUT; //@TODO: ??
        client[i].revents = 0;
    }
    activeClients = 0;

    /* Tworzymy gniazdko centrali */
    client[0].fd = socket(PF_INET, SOCK_STREAM, 0);
    if (client[0].fd < 0) {
        perror("Opening stream socket");
        exit(EXIT_FAILURE);
    }

    /* Co do adresu nie jesteśmy wybredni */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT_NUM);  //tylko na naszym porcie
    if (bind(client[0].fd, (struct sockaddr *) &server, (socklen_t)
    sizeof(server)) < 0) {
        perror("Binding stream socket");
        exit(EXIT_FAILURE);
    }

    /* Zapraszamy klientów */
    if (listen(client[0].fd, 5) == -1) {
        perror("Starting to listen");
        exit(EXIT_FAILURE);
    }

    /* Do pracy */
    do {
        for (i = 0; i < _POSIX_OPEN_MAX; ++i)
            client[i].revents = 0;

        /* Po Ctrl-C zamykamy gniazdko centrali */
        if (finish == TRUE && client[0].fd >= 0) {
            if (close(client[0].fd) < 0)
                perror("close");
            client[0].fd = -1;
        }

        /* Czekamy przez 5000 ms */
        ret = poll(client, _POSIX_OPEN_MAX, 5000);
        if (ret < 0)
            perror("poll");
        else if (ret > 0) {
            if (finish == FALSE && (client[0].revents & POLLIN)) {
                msgsock = accept(client[0].fd, (struct sockaddr *) 0, (socklen_t *) 0);
                if (msgsock == -1) {
                    perror("accept");
                } else {
                    for (i = 1; i < _POSIX_OPEN_MAX; ++i) {
                        if (client[i].fd == -1) {
                            client[i].fd = msgsock;
                            activeClients += 1;
                            break;
                        }
                    }
                    if (i >= _POSIX_OPEN_MAX) {
                        fprintf(stderr, "Too many clients\n");
                        if (close(msgsock) < 0)
                            perror("close");
                    }
                }
            }
            for (i = 1; i < _POSIX_OPEN_MAX; ++i) {
                if (client[i].fd != -1 && (client[i].revents & (POLLIN | POLLERR))) {
                    rval = read_all(client[i].fd, (char *) &message_size, sizeof(message_size));
                    if (rval < 0) {
                        perror("Reading stream message");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    } else if (rval == 0) {
                        fprintf(stderr, "Ending connection, some troubles reading from client\n");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    } else {
                        message_size = ntohs(message_size);
                        if (message_size > 0 && message_size <= MAX_MESSAGE_SIZE) {
                            received = read_all(client[i].fd, buf, message_size);

                            if (received == message_size) {
                                printf("-->%.*s\n", (int) received, buf);

                                mess = malloc(sizeof(struct message) + message_size);
                                copy_message_into_struct(mess, message_size, buf);

                                for (j = 1; j < _POSIX_OPEN_MAX; ++j) {
                                    if ((i != j) && (client[j].fd != -1) && (client[i].events & (POLLOUT))) {
                                        rval = write_all(client[j].fd, mess, sizeof(struct message) + message_size);
                                        if (rval < 0)
                                            perror("writing on stream socket");
                                        else if (rval < sizeof(struct message) + message_size) {
                                            fprintf(stderr, "Ending connection, some troubles writing to client\n");
                                            if (close(client[j].fd) < 0)
                                                perror("close");
                                            client[j].fd = -1;
                                            activeClients -= 1;
                                        }
                                    }
                                }
                                free(mess);
                            } else {
                                fprintf(stderr, "Ending connection, some trouble while reading message\n");
                                if (close(client[i].fd) < 0)
                                    perror("close");
                                client[i].fd = -1;
                                activeClients -= 1;
                                break;
                            }
                        } else {
                            fprintf(stderr, "Ending connection, wrong data got sent\n");
                            if (close(client[i].fd) < 0)
                                perror("close");
                            client[i].fd = -1;
                            activeClients -= 1;
                        }
                    }
                }
            }
        }
    } while (finish == FALSE || activeClients > 0);

    if (client[0].fd >= 0) if (close(client[0].fd) < 0)
        perror("Closing main socket");
    exit(EXIT_SUCCESS);
}