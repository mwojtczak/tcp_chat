#ifndef MESSAGE_H

#define TRUE 1
#define FALSE 0
//#define MAX_MESSAGE_SIZE 1000
#define MAX_MESSAGE_SIZE 10
#define MAX_BUFF_SIZE 12

#define PORT_NUM 20160
#define PORT_SIGN "20160"
#define MAX_PORT_NO 65535
#define MAX_CLIENTS 20

struct message{
    unsigned short lenght;
    //char* text;
    char text[0];
} __attribute__((packed));

struct message1{
    unsigned short lenght;
    //char* text;
} __attribute__((packed));

void test();

ssize_t read_all(int fd, void *buf, size_t count);

ssize_t write_all(int fd, const void *buf, size_t count);

int is_port_number(char * num);

int find_string_end(char * str);

#define MESSAGE_H

#endif //MESSAGE_H
