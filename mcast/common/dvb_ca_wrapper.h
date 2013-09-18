/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __DVB_CA_WRAPPER_H
#define __user
unsigned int dvb_cam_poll(struct pollfd *pfd, int fdnum, int timeout_ms);
ssize_t dvb_cam_read(int dummy, char __user * buf, size_t count);
ssize_t dvb_cam_write(int dummy, char __user * buf, size_t count);
int dvb_cam_ioctl(int dummy, unsigned int cmd, void *parg);
int dvb_cam_open(const char* dummy, int dummy1);
int dvb_cam_close(int fd);

#endif
