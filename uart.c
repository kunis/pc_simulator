#define DEV_NAME "/dev/ttyUSB0"
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <time.h>
#include <termios.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include<termios.h>
#include<stdio.h>
#include "uart.h"

static int uart_fd = -1, epoll_fd = -1;

#define printk printf
void dump(uint8_t *p,int size)
{

    int i = 1;
    while(size > 0){

        printk("0x%.2x ",*p);
        if(i%8 == 0)
            printk("\n");

        i++;
        p++;
        size--;
    }
    printk("\n");


}

static int epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL,NULL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 );
    return ret;
}

static int epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0);
    return ret;
}


int hm_init_uart(void)
{

    struct termios termios;
    int ret = -1;
    int size = 0;
    int fd = open(DEV_NAME, O_RDWR|O_NDELAY|O_NOCTTY ,0);
    printf("open:%s\n",DEV_NAME);
    uart_fd = fd;
    tcflush(fd, TCIOFLUSH);
    tcgetattr(fd, &termios);
    termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
            | INLCR | IGNCR | ICRNL | IXON);
    termios.c_oflag &= ~OPOST;
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    //termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag &= ~(CSIZE);

    termios.c_cflag |= CS8; 

#if 0 
    termios.c_cflag |= PARENB;
    termios.c_cflag &= ~PARODD ;
#endif   

    termios.c_cflag &= ~CRTSCTS;
//    termios.c_cflag |= CRTSCTS;

    tcsetattr(fd, TCSANOW, &termios);

    cfsetospeed(&termios, B115200);
    cfsetispeed(&termios, B115200);
    tcsetattr(fd, TCSANOW, &termios);

    tcflush(fd,TCIOFLUSH);
    epoll_fd   = epoll_create(2);
    epoll_register( epoll_fd, uart_fd);

    return 0;
}

uint32_t uart_flush(void)
{

    tcflush(uart_fd,TCIOFLUSH);
    return 0; 
}

#define MAX_PACKAGE_SIZE  (1024)
uint32_t uart_transfer(uint8_t *buf,uint32_t size ,uint32_t direct,uint32_t timeout)
{

    int len = 0;
    int all = 0;
    int transfer = 0;
    uint8_t *p = buf;

    if(direct == UART_WRITE){

        len = write(uart_fd,buf,size);
        if(len != (int)size)
            return -1;
        all = len;
    }else if(direct == UART_READ){

        if(size > MAX_PACKAGE_SIZE)
            transfer = MAX_PACKAGE_SIZE;
        else
            transfer = size;
        while(size > 0){
            struct epoll_event   events;
            int event = epoll_wait( epoll_fd, &events, 1, timeout );
            if(event < 0){
                printk("epoll_wait() unexpected error: \n");
                return -1;
            }
            len = read(uart_fd,p,transfer);
            if(len < 0 )
                continue;
            
            size -= len;
            if(size > MAX_PACKAGE_SIZE)
                transfer = MAX_PACKAGE_SIZE;
            else
                transfer = size;
            p+=len;
            all+=len;
        }
    }

    return len;

}
