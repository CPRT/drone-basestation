// C library headers
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#define TICKS_TO_US(x)  ((x - 992) * 5 / 8 + 1500)
#define US_TO_TICKS(x)  ((x - 1500) * 8 / 5 + 992)

typedef struct __attribute__((packed))
{
    int chan01: 11;
    int chan02: 11;
    int chan03: 11;
    int chan04: 11;
    int chan05: 11;
    int chan06: 11;
    int chan07: 11;
    int chan08: 11;
    int chan09: 11;
    int chan10: 11;
    int chan11: 11;
    int chan12: 11;
    int chan13: 11;
    int chan14: 11;
    int chan15: 11;
    int chan16: 11;
} channels_t;

unsigned char crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

uint8_t crc8(const uint8_t * ptr, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i=0; i<len; i++)
        crc = crc8tab[crc ^ *ptr++];
    return crc;
}

void rc_channels_packed(uint8_t* buf, channels_t chan) {
  // Assumes buf is big enough
  buf[0] = 0xC8;
  buf[1] = 24;
  buf[2] = 0x16;
  memcpy(buf + 3, &chan, 22);
  buf[25] = crc8(buf + 2, 23);
}

int main() {
  int serial_port = open("/dev/ttyAMA2", O_RDWR);

  struct termios tty;

  if(tcgetattr(serial_port, &tty) != 0) {
      printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
      return 1;
  }

  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
  tty.c_cflag |= CS8; // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO; // Disable echo
  tty.c_lflag &= ~ECHOE; // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo
  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  cfsetispeed(&tty, B921600);
  cfsetospeed(&tty, B921600);

  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
      printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
      return 1;
  }
  int sockfd; /* socket */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char netbuf[1024]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    perror("ERROR opening socket");

  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(7534);

  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    perror("ERROR on binding");

  clientlen = sizeof(clientaddr);
  while (1) {
    bzero(netbuf, 1024);
    n = recvfrom(sockfd, netbuf, 1024, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      perror("ERROR in recvfrom"); 

    netmsg_t netmsg;
    memcpy(&netmsg, netbuf, n);

    channels_t chan;
    chan.chan01 = US_TO_TICKS(ntohs(netmsg.chan01));
    chan.chan02 = US_TO_TICKS(ntohs(netmsg.chan02));
    chan.chan03 = US_TO_TICKS(ntohs(netmsg.chan03));
    chan.chan04 = US_TO_TICKS(ntohs(netmsg.chan04));
    chan.chan05 = US_TO_TICKS(ntohs(netmsg.chan05));
    chan.chan06 = US_TO_TICKS(ntohs(netmsg.chan06));
    chan.chan07 = US_TO_TICKS(ntohs(netmsg.chan07));
    chan.chan08 = US_TO_TICKS(ntohs(netmsg.chan08));
    chan.chan09 = US_TO_TICKS(ntohs(netmsg.chan09));
    chan.chan10 = US_TO_TICKS(ntohs(netmsg.chan10));
    chan.chan11 = US_TO_TICKS(ntohs(netmsg.chan11));
    chan.chan12 = US_TO_TICKS(ntohs(netmsg.chan12));
    chan.chan13 = US_TO_TICKS(ntohs(netmsg.chan13));
    chan.chan14 = US_TO_TICKS(ntohs(netmsg.chan14));
    chan.chan15 = US_TO_TICKS(ntohs(netmsg.chan15));
    chan.chan16 = US_TO_TICKS(ntohs(netmsg.chan16));

    uint8_t buf[26];
    rc_channels_packed(buf, chan);
  
    write(serial_port, buf, 26);
  }

  close(serial_port);
  return 0; // success
}
