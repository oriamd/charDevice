#ifndef IOCTLREQUEST_H
#define IOCTLREQUEST_H
#include <linux/ioctl.h>

#define IOCTL_APP_TYPE 72
#define IOCTL_CHANGE_TAPE _IOW(IOCTL_APP_TYPE, 0, int )

#endif
