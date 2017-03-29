#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "ioctlrequest.h"

int main(int argc,char* argv[]){
    int dummy;
    if(argc < 2){
        return -1;
    }
    char buff[100];
    int fd = open("/dev/TAPE_DEVICE",O_RDWR);
    if(fd==NULL){
        puts("userspace : could not open file\n");
        return -1;
    }
    //Case user wants to read from current tape
    if(!strcmp(argv[1],"read")){
        if(read(fd,buff,100)<0)
            perror("userspace read errno : ");
        printf("%s\n",buff);
    //Chase user wants to write to current tape
    }else if(!strcmp(argv[1],"write") && argc>2 ){
        if(write(fd,argv[2],strlen(argv[2]))<0);
            perror("userspace write errno : ");
    //Case user wants to Change tape
    }else if(!strcmp(argv[1],"change") && argc>2){
        strcpy(buff,argv[2]);
        if(buff[0] > '9' || buff[0] < '0' || strlen(argv[2])>1 ){
            printf("- No such commend found. \n- Try : read, write [string] , change [number 0 - 9]\n");
        }
        if(ioctl(fd, IOCTL_CHANGE_TAPE, buff)<0) {
            perror("userspace write errno : ");
        }

    }else{
        printf("- No such commend found. \n- Try : read, write [string] , change [number 0 - 9]\n");
    }
    return 0;
}
