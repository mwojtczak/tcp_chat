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
//            printf("got 0 bytes\n");
            return received;
        } else {
//            printf("got some bytes %d\n", result);
            received += result;
//            printf("got all bytes %d\n", received);
        }
    }
//    printf("returning: %d\n", received);
    return received;
};

ssize_t write_all(int fd, const void *buf, size_t count){
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

//1 if is, 0 if isnt
int is_port_number(char * num){
    size_t len = strlen(num);
    int i;
    for (i = 0; i < len; ++i) {
        if (!isdigit(*(num + i)))
            return 0;
    }
    //convert to number & check if is from right range
    long number = strtol(num, NULL, 10);
    if ((number < 1) || (number > MAX_PORT_NO))
        return 0;
    return 1;
}


//@TODO: znaleźć w str miejsce wystąpienia pierwszego \n lub strlen(str): o 1 wcześniej zakończyć dł stringa

//zwraca pozycję pierwszego napotkanego \n lub \0: słowem pozycję DO KTÓREJ należy czytać stringa: czyli jego właściwą długość bez tych znaków
//wyślij tyle znaków do bufora, ile zwróciła ta funkcja
int find_string_end(char * str){
    int index = strnlen(str, MAX_MESSAGE_SIZE); // @TODO: MAX_MASS_SIZe
    const char *ptr = strchr(str, '\n');
    if(ptr) {
        if (ptr - str < index)
            index = ptr - str;
    }
    return index;
}

void copy_message_into_struct(struct message * mess, unsigned short size, char * buff){
    mess->lenght = htons(size);
    memcpy(&mess->text, buff, size);
}