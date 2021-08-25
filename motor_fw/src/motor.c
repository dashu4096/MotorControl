#include "motor.h"                                                   
#include "utils.h"                                                   

void motor_1()
{
	motor_stop();
	M_1 = 1;
	M_2 = 0;
	M_3 = 1;
	M_4 = 0;
	M_5 = 1;
	M_6 = 1;
	mdelay(800);
}
void motor_2()
{
	motor_stop();
	M_1 = 1;
	M_2 = 0;
	M_3 = 1;
	M_4 = 1;
	M_5 = 0;
	M_6 = 1;
	mdelay(800);
}
void motor_3()
{
	motor_stop();
	M_1 = 0;
	M_2 = 1;
	M_3 = 1;
	M_4 = 0;
	M_5 = 1;
	M_6 = 1;
	mdelay(800);
}
void motor_4()
{
	motor_stop();
	M_1 = 0;
	M_2 = 1;
	M_3 = 1;
	M_4 = 1;
	M_5 = 0;
	M_6 = 1;
	mdelay(800);
}
void motor_5()
{
	motor_stop();
	M_1 = 1;
	M_2 = 1;
	M_3 = 1;
	M_4 = 0;
	M_5 = 1;
	M_6 = 0;
	mdelay(800);
}
void motor_6()
{
	motor_stop();
	M_1 = 1;
	M_2 = 1;
	M_3 = 1;
	M_4 = 1;
	M_5 = 0;
	M_6 = 0;
	mdelay(800);
}
void motor_stop()
{
	M_1 = 1;
	M_2 = 1;
	M_3 = 1;
	M_4 = 1;
	M_5 = 1;
	M_6 = 1;
}
void motor_init()
{
	config_pin1(1,1);
	config_pin1(1,2);
	config_pin1(1,3);                                                            
	config_pin1(1,4);    
	config_pin1(1,5);
	config_pin3(1,4);
	config_pin3(0,3);
	motor_stop();
}
