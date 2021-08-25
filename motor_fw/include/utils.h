#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

extern void udelay(uint16_t n);
extern void mdelay(uint16_t n);
extern void config_clock();
extern void config_pin1(uint8_t mode,uint8_t pin);
extern void config_pin3(uint8_t mode,uint8_t pin);
extern void chip_reset();
#ifdef DEBUG
#ifndef  UART0_BAUD
#define  UART0_BAUD    57600
#endif
void uart0_init( );//T1作为波特率发生器
uint8_t uart0_read( );//CH554 UART0查询方式接收一个字节
void  uart0_write(uint8_t data);//CH554UART0发送一个字节
#if SDCC < 370
void putchar(char c);
char getchar();
#else
int putchar(int c);
int getchar(void);
#endif
#endif

#endif
