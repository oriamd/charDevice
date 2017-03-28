#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NETLINK_USER 31
#define NUM_OF_TAPES 10
#define DATA_MAX_SIZE 1001

//Defining mesging protocol with driver
#define WRITE_PROTOCOL "writetofileprotocolcode"
#define WRITE_PROTOCOL_SIZE 23
#define READ_PROTOCOL "readfromfileprotocolcode"
#define READ_PROTOCOL_SIZE 24
#define CHANGE_TAPE_PROTOCOL "chagetapeprotocolcode"
#define CHANGE_TAPE_PROTOCOL_SIZE 21

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

char dataStorage[DATA_MAX_SIZE];
//Current active tape
int currtape = 0;
//Names of all tape files
char tapeFilesNames[NUM_OF_TAPES][20] = {
        "tapes/tape0.txt","tapes/tape1.txt","tapes/tape2.txt","tapes/tape3.txt","tapes/tape4.txt","tapes/tape5.txt","tapes/tape6.txt","tapes/tape7.txt",
        "tapes/tape8.txt","tapes/tape9.txt"
};

int writeTape(char* data,int len);
int readTape();

int main()
{
    currtape = 0;
    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

     nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    readTape();
    strcpy(NLMSG_DATA(nlh), dataStorage);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    //Now sending kernel data to init his cache
    printf("tapedevice : Sending data to kernel\n");
    sendmsg(sock_fd,&msg,0);
    //Waiting for kernel to say hello and confirm receving init data
    printf("tapedevice : Waiting for kernel to say Hello \n");
    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    printf("tapedevice : Received message payload: %s\n", (char *)NLMSG_DATA(nlh));

    while(1){
        printf("tapedevice : Waiting for kernel command ... \n");
        /* Waiting for instructions from kernel */
        recvmsg(sock_fd, &msg, 0);
//        printf("tapedevice : Received message payload: %s\n", (char *)NLMSG_DATA(nlh));
        switch (mapMsg((char *)NLMSG_DATA(nlh))){
            case 1://Need to write In data to tape (The driver already have the new data)
                printf("tapedevice : Receved [Write] msg from kernel\n");
                writeTape((char *)NLMSG_DATA(nlh),NLMSG_PAYLOAD(nlh, 0));
                break;
            case 2:
                printf("tapedevice : Receved [Change tape] msg from kernel\n");
                //Updating current tape
                currtape = (int)(((char *)NLMSG_DATA(nlh))[CHANGE_TAPE_PROTOCOL_SIZE]-'0');
                //Caching the tape
                readTape();
                //LINUX CRASH HERE
                //*************************************************************************
                strcpy(NLMSG_DATA(nlh), dataStorage);
                sendmsg(sock_fd,&msg,0);
                //*************************************************************************
                printf("tapedevice : changed tape to %s\n",tapeFilesNames[currtape]);
                break;
        }

    }

    close(sock_fd);
}

//writing data to current tape
int writeTape(char* data,int len){
    int fd = open(tapeFilesNames[currtape], O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXG|S_IRWXO);
    int numOfBytes;

    if(fd == NULL){
        printf("tapedevice::writeTape : could not open file for write\n");
        return -1;
    }
    if((numOfBytes = write(fd,data,len))>0){
        printf("tapedevice::writeTape : successfully wrote %d bytes to %s\n",numOfBytes,tapeFilesNames[currtape]);
    }
    close(fd);
    return 0;
}

//reading tape to in Msg
int readTape(){
    int fd = open(tapeFilesNames[currtape], O_RDONLY|O_CREAT,S_IRWXU|S_IRWXG|S_IRWXO );
    int numOfBytes;

    if(fd < 0 ){
        perror("tapedevice::readTape :  read file ernno : ");
        return -1;
    }
    if((numOfBytes = read(fd,dataStorage,DATA_MAX_SIZE))>=0){
        printf("tapedevice::readTape : successfully read %d bytes from %s\n",numOfBytes,tapeFilesNames[currtape]);
    }else{
        printf("tapedevice::readTape : could not read from file\n");
        perror("tapedevice::readTape : read errno : ");
        return -1;
    }

    close(fd);
    return 0;
}


//Maps the msg tile to index
int mapMsg(char* msg){

    if(!strncmp(msg,CHANGE_TAPE_PROTOCOL,CHANGE_TAPE_PROTOCOL_SIZE)){
        return 2;
    }else{
        return 1;
    }

}

