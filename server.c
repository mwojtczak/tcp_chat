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
static void catch_int (int sig) {
    finish = TRUE;
    fprintf(stderr,
            "Signal %d catched. No new connections will be accepted.\n", sig);
}

int main (int argc, char *argv[]) {
    struct pollfd client[_POSIX_OPEN_MAX]; //do trzymania klientów
    struct sockaddr_in server;
    char buf[MAX_MESSAGE_SIZE];
    size_t length;
    ssize_t rval;
    int msgsock, activeClients, i, ret;

//check params
    char* port;
    if (argc == 2){
        port = argv[1];
    }else if(argc == 1){
        port = PORT_SIGN;
    } else {
//        fatal("Usage: %s port or without any parameters", argv[0]); //nie można użyć fatal, gdyz wyrzuca inny nr
        fprintf(stderr, "ERROR: Usage: %s port or without any parameters\n", argv[0]);
        return 1;
    }

    /* Po Ctrl-C kończymy */
    if (signal(SIGINT, catch_int) == SIG_ERR) {
        perror("Unable to change signal handler");
        exit(EXIT_FAILURE);
    }

    /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
    for (i = 0; i < _POSIX_OPEN_MAX; ++i) {
        client[i].fd = -1;
        client[i].events = POLLIN | POLLOUT;
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
    if (bind(client[0].fd, (struct sockaddr*)&server,
             (socklen_t)sizeof(server)) < 0) {
        perror("Binding stream socket");
        exit(EXIT_FAILURE);
    }

    /* Dowiedzmy się, jaki to port i obwieśćmy to światu */
    length = sizeof(server);
    if (getsockname (client[0].fd, (struct sockaddr*)&server,
                     (socklen_t*)&length) < 0) {
        perror("Getting socket name");
        exit(EXIT_FAILURE);
    }
    printf("Socket port #%u\n", (unsigned)ntohs(server.sin_port));

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
                msgsock =
                        accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
                if (msgsock == -1)
                    perror("accept");
                else {
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
            //tutaj deklaruję strukturę
            unsigned short message_size;
            struct message message_to_send;
            for (i = 1; i < _POSIX_OPEN_MAX; ++i) {
                if (client[i].fd != -1
                    && (client[i].revents & (POLLIN | POLLERR))) {
//                    rval = read(client[i].fd, buf, BUF_SIZE);
                    rval = read(client[i].fd, (char*)&message_size, sizeof(message_size));
                    if (rval < 0) {
                        perror("Reading stream message");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    }
                    else if (rval == 0) {
                        fprintf(stderr, "Ending connection\n");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    }
                    else{
                        //przekonwertuj:
                        message_size = ntohs(message_size);
                        //doczytaj reszte wiadomości
                        int received = 0;
                        int to_be_received = message_size;
                        while (to_be_received){
                            rval = read(client[i].fd, buf + received, to_be_received);
                            if ((rval) < 0){
                                printf("SOME ERROR< leaving the loop");
                                break;
                            }
                            if (rval == 0){
                                //maybe end of file, maybe error, check if read all message
                            } else {
                                received += rval;
                                to_be_received -= rval;
                            }
                        }
//
//
                        if (received == message_size){
                            printf("wiadomość od %d: ", i);
                        }
                        printf("-->%.*s\n", (int)received, buf);

                        printf("wiadomość od %d: %d\n", i, message_size);

                        //buduję wiadomość message:
//                        message_to_send.lenght = htons(message_size);
                        //buduję właściwą wiadomość:
                        struct message * mess = NULL;
                        mess = malloc(sizeof(struct message) + message_size);
                        mess->lenght = htons(message_size);
                        memcpy(&mess->text, buf, message_size);
//                        message_to_send.text = buf;

                        //do każdego z clientów != i wyslij wiadomość
                        int j;
                        for (j = 1; j < _POSIX_OPEN_MAX; ++j) {
                            if ((i != j) && (client[j].fd != -1) && (client[i].events & (POLLOUT))){
                                if (write(client[j].fd, mess, sizeof(struct message) + message_size) < 0)
                                    perror("writing on stream socket");
//                                if (write(client[j].fd, buf, (int)rval) < 0)
//                                    perror("writing on stream socket");
//                                printf("writing to clients\n");
                            }

//                            printf("writing to clients\n");
                        }
                    }
                }
            }
        }
        else
            fprintf(stderr, "Do something else\n");
    } while (finish == FALSE || activeClients > 0);

    if (client[0].fd >= 0)
    if (close(client[0].fd) < 0)
        perror("Closing main socket");
    exit(EXIT_SUCCESS);
}