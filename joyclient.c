/*
 * jstest.c  Version 1.2
 *
 * Copyright (c) 1996-1999 Vojtech Pavlik
 *
 * Sponsored by SuSE
 */

/*
 * This program can be used to test all the features of the Linux
 * joystick API, including non-blocking and select() access, as
 * well as version 0.x compatibility mode. It is also intended to
 * serve as an example implementation for those who wish to learn
 * how to write their own joystick using applications.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include <strings.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NAME_LENGTH 128

#define AXIS_CHAN(x) ((uint16_t)((int16_t)(x / 64) + 1500)) // 988 to 2012 us
#define BTN_CHAN(x) (x ? 2000 : 1000)

typedef struct __attribute__((packed))
{
  uint16_t chan01;
  uint16_t chan02;
  uint16_t chan03;
  uint16_t chan04;
  uint16_t chan05;
  uint16_t chan06;
  uint16_t chan07;
  uint16_t chan08;
  uint16_t chan09;
  uint16_t chan10;
  uint16_t chan11;
  uint16_t chan12;
  uint16_t chan13;
  uint16_t chan14;
  uint16_t chan15;
  uint16_t chan16;
} netmsg_t;

pthread_mutex_t lock;
netmsg_t msg;
struct sockaddr_in servaddr;

void* reader(void* arg) {
  char* buf[32];
  int sockfd;
  
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("\n Error : Connect Failed \n");
    exit(0);
  }

  while (1) {
    pthread_mutex_lock(&lock);

    memcpy(buf, &msg, 32);
    sendto(sockfd, buf, 32, 0, (struct sockaddr*)NULL, sizeof(servaddr));

    pthread_mutex_unlock(&lock);
    usleep(4000);
  }

  close(sockfd);
  return NULL;
}

int main(int argc, char **argv)
{
	int fd;
	unsigned char axes = 2;
	unsigned char buttons = 2;
  int version = 0x000800;
	int verbose = 0;

  if (argc != 3) {
    printf("%s: <remoteip> <joystick>\n", argv[0]);
    return 1;
  }

	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
		perror("jstest");
		return 1;
	}

	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &buttons);

	int *axis;
	char *button;
	struct js_event js;

	axis = calloc(axes, sizeof(int));
	button = calloc(buttons, sizeof(char));

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(7534);
  servaddr.sin_family = AF_INET;

  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Mutex initialization failed.\n");
    return 1;
  }

  pthread_t reader_thread;

  pthread_create(&reader_thread, NULL, reader, NULL);

	while (1) {
		if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
			perror("\njstest: error reading");
			return 1;
		}

		switch(js.type & ~JS_EVENT_INIT) {
		case JS_EVENT_BUTTON:
			button[js.number] = js.value;
			break;
		case JS_EVENT_AXIS:
			axis[js.number] = js.value;
			break;
		}

    pthread_mutex_lock(&lock);
    msg.chan01 = htons(AXIS_CHAN(axis[0]));
    msg.chan02 = htons(AXIS_CHAN(axis[1]));
    msg.chan03 = htons(AXIS_CHAN(axis[2]));
    msg.chan04 = htons(AXIS_CHAN(axis[3]));
    msg.chan05 = htons(AXIS_CHAN(axis[4]));
    msg.chan06 = htons(AXIS_CHAN(axis[5]));
    msg.chan07 = htons(AXIS_CHAN(axis[6]));
    msg.chan08 = htons(BTN_CHAN(button[0]));
    msg.chan09 = htons(BTN_CHAN(button[1]));
    msg.chan10 = htons(BTN_CHAN(button[2]));
    msg.chan11 = htons(992);
    msg.chan12 = htons(992);
    msg.chan13 = htons(992);
    msg.chan14 = htons(992);
    msg.chan15 = htons(992);
    msg.chan16 = htons(992);
    pthread_mutex_unlock(&lock);
    
  		printf("\r");
  		printf("Channels:");
  		printf("  1:%5" PRIu16, ntohs(msg.chan01));
  		printf("  2:%5" PRIu16, ntohs(msg.chan02));
  		printf("  3:%5" PRIu16, ntohs(msg.chan03));
  		printf("  4:%5" PRIu16, ntohs(msg.chan04));
  		printf("  5:%5" PRIu16, ntohs(msg.chan05));
  		printf("  6:%5" PRIu16, ntohs(msg.chan06));
  		printf("  7:%5" PRIu16, ntohs(msg.chan07));
  		printf("  8:%5" PRIu16, ntohs(msg.chan08));
  		printf("  9:%5" PRIu16, ntohs(msg.chan09));
  		printf(" 10:%5" PRIu16, ntohs(msg.chan10));
  		printf(" 11:%5" PRIu16, ntohs(msg.chan11));
  		printf(" 12:%5" PRIu16, ntohs(msg.chan12));
  		printf(" 13:%5" PRIu16, ntohs(msg.chan13));
  		printf(" 14:%5" PRIu16, ntohs(msg.chan14));
  		printf(" 15:%5" PRIu16, ntohs(msg.chan15));
  		printf(" 16:%5" PRIu16, ntohs(msg.chan16));
  		fflush(stdout);
	}

  pthread_join(reader_thread, NULL);

  close(fd);

	return -1;
}
