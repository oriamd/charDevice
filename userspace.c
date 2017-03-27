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
    if(!strcmp(argv[1],"read")){
        if(read(fd,buff,100)<0)
            perror("userspace read errno : ");
        printf("%s\n",buff);
    }else if(!strcmp(argv[1],"write") && argc>2 ){
        if(write(fd,argv[2],strlen(argv[2]))<0);
            perror("userspace write errno : ");
    }else if(!strcmp(argv[1],"change") && argc>2){
        printf("igothrer\n");
        dummy = atoi(argv[2]);
        printf("%d\n",dummy);
        if(ioctl(fd, IOCTL_CHANGE_TAPE, dummy)<0) {
            perror("userspace write errno : ");
        }
    }else{
        printf("- No such commend found. \n- Try : read, write [string arg] , change [int arg]\n");
    }
    return 0;
}
