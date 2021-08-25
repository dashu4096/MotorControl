#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "ch554.h"                                                   
SBIT(M_1, 0x90, 1);//p1.1
SBIT(M_2, 0x90, 2);//p1.2
SBIT(M_3, 0x90, 3);//p1.3
SBIT(M_4, 0x90, 4);//p1.4
SBIT(M_5, 0x90, 5);//p1.5
SBIT(M_6, 0xB0, 4);//p3.4
SBIT(M_DET, 0xB0, 3);

extern void motor_1();
extern void motor_2();
extern void motor_3();
extern void motor_4();
extern void motor_5();
extern void motor_6();
extern void motor_stop();
extern void motor_init();
#endif
