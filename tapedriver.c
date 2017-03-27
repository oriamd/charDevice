#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <asm/uaccess.h>
#include "ioctlrequest.h"

#define DEVICE_NAME "TAPE_DEVICE"
#define TRUE 1
#define FALSE !TRUE
#define NETLINK_USER 31
#define DATA_MAX_SIZE 1001
//Defining mesging protocol with device
#define WRITE_PROTOCOL "writetofileprotocolcode"
#define READ_PROTOCOL "readfromfileprotocolcode"
#define CHANGE_TAPE_PROTOCOL "chagetapeprotocolcode"

//Read handeling routine
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
//Write handeling routine
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
//Open handeling routine
static int device_open(struct inode*, struct file*);
//release handeling routine
static int device_release(struct inode*, struct file*);
//IOCTL handeling routine
static long device_ioctl(struct file*, unsigned int ioctl_no, unsigned long ioctl_param);

//cache contain cached data from current tape
static char cache[DATA_MAX_SIZE];
static char msg[100] ="Hello from kernel";
static void nl_recv_msg(struct sk_buff*);
static void send_to_device(const char*,const int);

static int major_number= 0;
static char is_opened = FALSE;

struct sock *nl_sk = NULL;

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl= device_ioctl,
    .open = device_open,
    .release = device_release
};

// INIT
static int initmodule(void){
    if((major_number = register_chrdev(0, DEVICE_NAME, &fops))<0){
        printk("tapedriver::init : register_chrdevfailed with status = %d\n", major_number);
        return major_number;
    }
    printk("tapedriver::Init : Device driver was successfully loaded. The major number is: %d\n", major_number);
    printk("tapedriver::Init : mknod/dev/%s c %d 0\n", DEVICE_NAME, major_number);

    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if(nl_sk== NULL){
        printk("tapedriver::Init : Error creating netlinksocket.\n");
        return -1;
    }
    printk(KERN_INFO "tapedriver::init : Init_mosule Success\n");
    return 0;
}

//EXIT
static void exitmodule(void){
    printk(KERN_INFO "tapedriver::exit : exit_musile Stated\n");
    unregister_chrdev(major_number, DEVICE_NAME);
    netlink_kernel_release(nl_sk);
}

module_init(initmodule);
module_exit(exitmodule);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Ori Amd");
MODULE_DESCRIPTION("Doing a whole lot of nothing.");

//READ
static ssize_t device_read(struct file* file, char* buffer, size_t length, loff_t* offset){
    printk("tapedriver::read : called!!!!\n");  //Debug
    //copying local data to user
    copy_to_user(buffer,cache,length);
    return length;
}

//WRITE
static ssize_t device_write(struct file* file, const char* buffer, size_t length, loff_t* offset){
    printk("tapedriver::write : called now writing.\n");
    copy_from_user(cache,buffer,length);
    cache[length] = '\0';
    send_to_device(cache,length);
    return length;
}

//IOCTL
static long device_ioctl(struct file* fs, unsigned int ioctl_no, unsigned long ioctl_param){
    char buffer[10];
    switch (ioctl_no){
        case IOCTL_CHANGE_TAPE:{
            strcpy(msg,CHANGE_TAPE_PROTOCOL);
            strcat(msg,ioctl_param);
            send_to_device(msg,strlen(msg));
        }
        break;
    }
    return 0;
}


//OPEN
static int device_open(struct inode* node, struct file* f){
    if (is_opened)
        return -EBUSY;
    is_opened = TRUE;
    return 0;
}

//RELEASE
static int device_release(struct inode* node, struct file* f){
    is_opened = FALSE;
    return 0;
}

static struct nlmsghdr *nlh;
static int pid;
static struct sk_buff *skb_out;
static int msg_size;
static int res;

//Recive Netlink msg from device
static void nl_recv_msg(struct sk_buff *skb) {

    printk(KERN_INFO "apedriver::nl_recv_msg : Entering: %s\n", __FUNCTION__);

    msg_size=strlen(msg);

    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "tapedriver::nl_recv_msg : Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
    //Saving data
    strncpy(cache,(char*)nlmsg_data(nlh),msg_size);
    pid = nlh->nlmsg_pid; /*pid of sending process */
    //Now sending hello msg
    strcpy(msg,"Hello from kernel");
    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out)
    {

        printk(KERN_ERR "tapedriver::nl_recv_msg : Failed to allocate new skb\n");
        return;

    }
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh),msg,msg_size);

    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_INFO "tapedriver::nl_recv_msg : Error while sending bak to user\n");

}

//Sending msg to device through Netlink
static void send_to_device(const char* str,const int len){
    msg_size = len;
    skb_out = nlmsg_new(msg_size,0);
    if(!skb_out)
    {

        printk(KERN_ERR "tapedriver::send_to_device : Failed to allocate new skb\n");
        return;

    }
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh),str,msg_size);

    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_INFO "tapedriver::send_to_device : Error while sending bak to user\n");
}

