#include "ch554.h"                                                   
#include "utils.h"                                                   

#define  OSC_EN_XT     0                 //外部晶振使能，默认开启内部晶振

void udelay(uint16_t n)
{
#ifdef	FREQ_SYS
#if		FREQ_SYS <= 6000000
	n >>= 2;
#endif
#if		FREQ_SYS <= 3000000
	n >>= 2;
#endif
#if		FREQ_SYS <= 750000
	n >>= 4;
#endif
#endif
	while ( n ) {  // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
		++ SAFE_MOD;  // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef	FREQ_SYS
#if		FREQ_SYS >= 14000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 16000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 18000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 20000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 22000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 24000000
		++ SAFE_MOD;
#endif
#endif
		-- n;
	}
}

void mdelay(uint16_t n)
{
	while (n) {
#ifdef	DELAY_MS_HW
		while ( ( TKEY_CTRL & bTKC_IF ) == 0 );
		while ( TKEY_CTRL & bTKC_IF );
#else
		udelay( 1000 );
#endif
		-- n;
	}
}                                         

void config_clock()
{
#if OSC_EN_XT	
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
	CLOCK_CFG |= bOSC_EN_XT;                          //使能外部晶振
	CLOCK_CFG &= ~bOSC_EN_INT;                        //关闭内部晶振 
#endif	
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
#if FREQ_SYS == 24000000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x06;  // 24MHz	
#endif	
#if FREQ_SYS == 16000000		
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x05;  // 16MHz	
#endif
#if FREQ_SYS == 12000000		
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x04;  // 12MHz
#endif	
#if FREQ_SYS == 6000000		
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x03;  // 6MHz	
#endif	
#if FREQ_SYS == 3000000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x02;  // 3MHz	
#endif
#if FREQ_SYS == 750000	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x01;  // 750KHz
#endif
#if FREQ_SYS == 187500	
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x00;  // 187.5KHz	
#endif
	SAFE_MOD = 0x00;
}

void chip_reset()
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |=bSW_RESET;
}

void config_pin1(uint8_t mode,uint8_t pin)
{
	switch(mode) {
		case 0:
			P1_MOD_OC = P1_MOD_OC & ~(1<<pin);
			P1_DIR_PU = P1_DIR_PU &	~(1<<pin);	
			break;
		case 1:
			P1_MOD_OC = P1_MOD_OC & ~(1<<pin);
			P1_DIR_PU = P1_DIR_PU |	(1<<pin);				
			break;		
		case 2:
			P1_MOD_OC = P1_MOD_OC | (1<<pin);
			P1_DIR_PU = P1_DIR_PU &	~(1<<pin);				
			break;		
		case 3:
			P1_MOD_OC = P1_MOD_OC | (1<<pin);
			P1_DIR_PU = P1_DIR_PU |	(1<<pin);			
			break;
		default:
			break;			
	}
}

void config_pin3(uint8_t mode,uint8_t pin)
{
	switch(mode) {
		case 0:
			P3_MOD_OC = P3_MOD_OC & ~(1<<pin);
			P3_DIR_PU = P3_DIR_PU &	~(1<<pin);	
			break;
		case 1:
			P3_MOD_OC = P3_MOD_OC & ~(1<<pin);
			P3_DIR_PU = P3_DIR_PU |	(1<<pin);				
			break;		
		case 2:
			P3_MOD_OC = P3_MOD_OC | (1<<pin);
			P3_DIR_PU = P3_DIR_PU &	~(1<<pin);				
			break;		
		case 3:
			P3_MOD_OC = P3_MOD_OC | (1<<pin);
			P3_DIR_PU = P3_DIR_PU |	(1<<pin);			
			break;
		default:
			break;			
	}
}
#ifdef DEBUG
void uart0_init()
{
    volatile uint32_t x;
    volatile uint8_t x2;

    SM0 = 0;
    SM1 = 1;
    SM2 = 0;                                                                   //串口0使用模式1
                                                                               //使用Timer1作为波特率发生器
    RCLK = 0;                                                                  //UART0接收时钟
    TCLK = 0;                                                                  //UART0发送时钟
    PCON |= SMOD;
    x = 10 * FREQ_SYS / UART0_BAUD / 16;                                       //如果更改主频，注意x的值不要溢出
    x2 = x % 10;
    x /= 10;
    if ( x2 >= 5 ) x ++;                                                       //四舍五入

    TMOD = TMOD & ~ bT1_GATE & ~ bT1_CT & ~ MASK_T1_MOD | bT1_M1;              //0X20，Timer1作为8位自动重载定时器
    T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                                        //Timer1时钟选择
    TH1 = 0-x;                                                                 //12MHz晶振,buad/12为实际需设置波特率
    TR1 = 1;                                                                   //启动定时器1
    TI = 1;
    REN = 1;                                                                   //串口0接收使能
}

uint8_t uart0_read()
{
    while(RI == 0);                                                            //查询接收，中断方式可不用
    RI = 0;
    return SBUF;
}

void uart0_write(uint8_t data)
{
        SBUF = data;                                                              //查询发送，中断方式可不用下面2条语句,但发送前需TI=0
        while(TI ==0);
        TI = 0;
}
#if SDCC < 370
void putchar(char c)
{
    while (!TI) /* assumes UART is initialized */
    ;
    TI = 0;
    SBUF = c;
}

char getchar() {
    while(!RI); /* assumes UART is initialized */
    RI = 0;
    return SBUF;
}
#else
int putchar(int c)
{
    while (!TI) /* assumes UART is initialized */
    ;
    TI = 0;
    SBUF = c & 0xFF;

    return c;
}

int getchar() {
    while(!RI); /* assumes UART is initialized */
    RI = 0;
    return SBUF;
}
#endif
#endif
