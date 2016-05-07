#ifndef MESSAGE_H

#define MAX_MESSAGE_SIZE 1000

struct message{
    unsigned short lenght;
    //char* text;
    char text[0];
} __attribute__((packed));

void test(){
    printf("z pliku message.h\n");
}

//@TODO: read_all(fd, buf, len): -1 (err), liczba przecztanych bajtów
//@TODO: write_all(fd, buf, len): -1 (err), liczba wysłanych bajtów        ---> na wypadek zapchanego bufora

#define MESSAGE_H

#endif //MESSAGE_H
