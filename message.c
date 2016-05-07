#include <stdlib.h>
#include "message.h"

void test(){
    printf("z pliku message.h\n");
}

ssize_t read_all(int fd, void *buf, size_t count){
    size_t received = 0;
    while (received < count){
        int result;
        result = read(fd, buf + received, count - received);
        if (result < 0){
            //some error while reading occured
            return -1;
        } else if (result == 0) {
            //probably end of file: stop reading cause there is nothing else
            //@TODO: ?
            //break;
            return received;
        } else {
            received += result;
        }
    }
    return received;
};

ssize_t write(int fd, const void *buf, size_t count){
    size_t written = 0;
    while (written < count){
        int result;
        result = write(fd, buf + written, count - written);
        if (result < 0){
            //some error occured
            return  -1;
        } else if (result == 0){
            //happens when nothing was written, ex. full buffer: may cause never ending looping
            //@TODO: ?
            return result;
        } else {
            written += result;
        }
    }
    return written;
}