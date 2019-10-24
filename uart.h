#ifndef __UART_TRANSFER__
#define __UART_TRANSFER__

#define UART_READ     0x1
#define UART_WRITE    0x2

uint32_t uart_flush(void);
int hm_init_uart(void);
uint32_t uart_transfer(uint8_t *buf,uint32_t size ,uint32_t direct,uint32_t timeout);
#endif
