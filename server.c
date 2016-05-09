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

static int activeClients;
struct pollfd client[_POSIX_OPEN_MAX]; //@TODO: max_clients + 1??

static void catch_int(int sig) {
    finish = TRUE;
    fprintf(stderr,
            "Signal %d catched. No new connections will be accepted.\n", sig);
}

int check_params(int argc, char *argv[]){
    if (argc == 2) {
        return atoi(argv[1]);
    } else if (argc == 1) {
        return PORT_NUM;
    } else {
        fprintf(stderr, "ERROR: Usage: %s port or without any parameters\n", argv[0]);
        exit(EXIT_FAILURE_PARAMS);
    }
}

void initiate_client_data(){
    int i;
    for (i = 0; i < _POSIX_OPEN_MAX; ++i) { //@TODO: max_clients + 1??
        client[i].fd = -1;
        client[i].events = POLLIN | POLLOUT; //@TODO: ??
        client[i].revents = 0;
    }
    activeClients = 0;
}

void check_errors(int data, char * error_message){
    if (data < 0) {
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

void clear_revents(){
    int i;
    for (i = 0; i < _POSIX_OPEN_MAX; ++i)
        client[i].revents = 0;
}

void try_accept_new_client(){
    int i, msgsock;
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
}

void close_client(int i, char * err_mess){
    fprintf(stderr, "%s\n", err_mess);
    if (close(client[i].fd) < 0)
        perror("close");
    client[i].fd = -1;
    activeClients -= 1;
}

void send_to_all(unsigned short message_size, char * buf, int i){
    struct message * mess = malloc(sizeof(struct message) + message_size);
    copy_message_into_struct(mess, message_size, buf);

    int j, rval;
    for (j = 1; j < _POSIX_OPEN_MAX; ++j) {
        if ((i != j) && (client[j].fd != -1) && (client[i].events & (POLLOUT))) {
            rval = write_all(client[j].fd, mess, sizeof(struct message) + message_size);
            if (rval < 0)
                perror("writing on stream socket");
            else if (rval < sizeof(struct message) + message_size) {
                close_client(j, "Ending connection, some trouble while writing message");
            }
        }
    }
    free(mess);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in server;
    char buf[MAX_BUFF_SIZE];
    size_t length;
    ssize_t rval;
    int msgsock, activeClients, i, j, ret, received;
    unsigned short message_size;
    int port;
    struct message *mess = NULL;

    port = check_params(argc, argv);

    if (signal(SIGINT, catch_int) == SIG_ERR) {
        perror("Unable to change signal handler");
        exit(EXIT_FAILURE);
    }

    initiate_client_data();

    client[0].fd = socket(PF_INET, SOCK_STREAM, 0);
    check_errors(client[0].fd, "Opening stream socket");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);  //tylko na naszym porcie //@TODO htons(port), port = PORT_NUM lub atoi(argv[1])

    check_errors(bind(client[0].fd, (struct sockaddr *) &server, (socklen_t)sizeof(server)), "Binding stream socket");

    check_errors(listen(client[0].fd, 20), "Starting to listen");

    activeClients = 0;
    do {
        clear_revents();

        if (finish == TRUE && client[0].fd >= 0) {
            check_errors(close(client[0].fd), "close");
        }

        /* Czekamy przez 5000 ms */
        ret = poll(client, _POSIX_OPEN_MAX, 0); //@TODO: 5000 czy 0?
        if (ret < 0)
            perror("poll");
        else if (ret > 0) {
            try_accept_new_client();
            for (i = 1; i < _POSIX_OPEN_MAX; ++i) {
                if (client[i].fd != -1 && (client[i].revents & (POLLIN | POLLERR))) {
                    rval = read_all(client[i].fd, (char *) &message_size, sizeof(message_size));
                    if (rval < 0) {
                        close_client(i, "Reading stream message");
                    } else if (rval == 0) {
                        close_client(i,  "Ending connection, some troubles reading from client");
                    } else {
                        message_size = ntohs(message_size);
                        if (message_size > 0 && message_size <= MAX_MESSAGE_SIZE) {
                            received = read_all(client[i].fd, buf, message_size);
                            if (received == message_size) {
                                send_to_all(message_size, buf, i);
//                                printf("%d ", message_size);
//                                printf("-->%.*s\n", (int) received, buf);
                            } else {
                                close_client(i, "Ending connection, some trouble while reading message");
                            }
                        } else {
                            close_client(i, "Ending connection, wrong data got sent");
                        }
                    }
                }
            }
        }
    } while (finish == FALSE || activeClients > 0);

    if (client[0].fd >= 0)
        if (close(client[0].fd) < 0)
            perror("Closing main socket");
    exit(EXIT_SUCCESS);
}