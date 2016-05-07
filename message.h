#ifndef MESSAGE_H

#define TRUE 1
#define FALSE 0
#define MAX_MESSAGE_SIZE 1000
#define PORT_NUM 20160
#define PORT_SIGN "20160"

struct message{
    unsigned short lenght;
    //char* text;
    char text[0];
} __attribute__((packed));

void test();

ssize_t read_all(int fd, void *buf, size_t count);

ssize_t write_all(int fd, const void *buf, size_t count);

//@TODO: read_all(fd, buf, len): -1 (err), liczba przecztanych bajtów
//@TODO: write_all(fd, buf, len): -1 (err), liczba wysłanych bajtów        ---> na wypadek zapchanego bufora

#define MESSAGE_H

#endif //MESSAGE_H
