#include <stdlib.h>
#include "message.h"

ssize_t read_all(int fd, void *buf, size_t count){
    size_t received = 0;
    while (received < count){
        int result;
        result = read(fd, buf + received, count - received);
        if (result < 0){
            return -1;
        } else if (result == 0) {
            return received;
        } else {
            received += result;
        }
    }
    return received;
};

ssize_t write_all(int fd, const void *buf, size_t count){
    size_t written = 0;
    while (written < count){
        int result;
        result = write(fd, buf + written, count - written);
        if (result < 0){
            return  -1;
        } else if (result == 0){
            return result;
        } else {
            written += result;
        }
    }
    return written;
}

int is_port_number(char * num){
    size_t len = strlen(num);
    int i;
    for (i = 0; i < len; ++i) {
        if (!isdigit(*(num + i)))
            return 0;
    }
    long number = strtol(num, NULL, 10);
    if ((number < 1) || (number > MAX_PORT_NO))
        return 0;
    return 1;
}

int find_string_end(char * str){
    int index = strnlen(str, MAX_MESSAGE_SIZE);
    if (index > 0){
        if (*(str + index - 1) == '\n')
            return index - 1;
    }
    return index;
}

void copy_message_into_struct(struct message * mess, unsigned short size, char * buff){
    mess->lenght = htons(size);
    memcpy(&mess->text, buff, size);
}