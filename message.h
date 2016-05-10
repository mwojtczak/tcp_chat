#ifndef MESSAGE_H

#define TRUE                1
#define FALSE               0
#define MAX_MESSAGE_SIZE    1000
#define MAX_BUFF_SIZE       1002
#define PORT_NUM            20160
#define PORT_SIGN           "20160"
#define MAX_PORT_NO         65535
#define MAX_CLIENTS         21
#define EXIT_FAILURE_PARAMS 1
#define EXIT_FAILURE_CLIENT 100
#define EXIT_FAILURE        130

struct message {
    unsigned short lenght;
    char text[0];
} __attribute__((packed));

ssize_t read_all(int fd, void *buf, size_t count);

ssize_t write_all(int fd, const void *buf, size_t count);

int is_port_number(char *num);

int find_string_end(char *str);

void copy_message_into_struct(struct message *mess, unsigned short size, char *buff);

#define MESSAGE_H

#endif //MESSAGE_H
