#ifndef IOCTLREQUEST_H
#define IOCTLREQUEST_H
#include <linux/ioctl.h>

#define MAJOR_NUM 100
#define IOCTL_CHANGE_TAPE _IOR(MAJOR_NUM, 0, char* )

#endif
