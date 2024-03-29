#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch554.h"
#include "usb.h"
#include "motor.h"
#include "utils.h"

__xdata __at (0x0000) uint8_t  ep0_buffer[DEFAULT_ENDP0_SIZE];       //端点0 OUT&IN缓冲区，必须是偶地址
__xdata __at (0x0040) uint8_t  ep1_buffer[DEFAULT_ENDP1_SIZE];       //端点1上传缓冲区
__xdata __at (0x0080) uint8_t  ep2_buffer[2*MAX_PACKET_SIZE];        //端点2 IN & OUT缓冲区,必须是偶地址

uint16_t setup_len;
uint8_t setup_req,usb_cfg;
const uint8_t * p_desc;                                             //USB配置标志
//USB_SETUP_REQ setup_buf;                                            //暂存Setup包
#define USB_SETUP_BUF     ((PUSB_SETUP_REQ)ep0_buffer)

#define SET_LINE_CODING        0X20            // Configures DTE rate, stop-bits, parity, and number-of-character
#define GET_LINE_CODING        0X21            // This request allows the host to find out the currently configured line coding.
#define SET_CONTROL_LINE_STATE 0X22            // This request generates RS-232/V.24 style control signals.


/*设备描述符*/
__code uint8_t dev_desc[] = {0x12,0x01,0x10,0x01,0x02,0x00,0x00,DEFAULT_ENDP0_SIZE,
	0x86,0x1a,0x22,0x57,0x00,0x01,0x01,0x02,
	0x03,0x01
};
__code uint8_t config_desc[] ={
	0x09,0x02,0x43,0x00,0x02,0x01,0x00,0xa0,0x32,             //配置描述符（两个接口）
	//以下为接口0（CDC接口）描述符
	0x09,0x04,0x00,0x00,0x01,0x02,0x02,0x01,0x00,             //CDC接口描述符(一个端点)
	//以下为功能描述符
	0x05,0x24,0x00,0x10,0x01,                                 //功能描述符(头)
	0x05,0x24,0x01,0x00,0x00,                                 //管理描述符(没有数据类接口) 03 01
	0x04,0x24,0x02,0x02,                                      //支持Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State
	0x05,0x24,0x06,0x00,0x01,                                 //编号为0的CDC接口;编号1的数据类接口
	0x07,0x05,0x81,0x03,0x08,0x00,0xFF,                       //中断上传端点描述符
	//以下为接口1（数据接口）描述符
	0x09,0x04,0x01,0x00,0x02,0x0a,0x00,0x00,0x00,             //数据接口描述符
	0x07,0x05,0x02,0x02,0x40,0x00,0x00,                       //端点描述符
	0x07,0x05,0x82,0x02,0x40,0x00,0x00,                       //端点描述符
};
/*字符串描述符*/
//语言描述符
unsigned char __code lang_desc[]={0x04,0x03,0x09,0x04};
//序列号字符串描述符
unsigned char __code sn_desc[]={0x0C,0x03,
	'V',0,'1',0,'.',0,'0',0,'0',0
};
//产品字符串描述符
unsigned char __code product_desc[]={0x22,0x03,
	'M',0,'o',0,'t',0,'o',0,'r',0,' ',0,'C',0,'o',0,'n',0,'t',0,'r',0,'o',0,'l',0,'l',0,'e',0,'r',0
};
unsigned char __code manufacturer_desc[]={0x1E,0x03,
	'p',0,'a',0,'y',0,'-',0,'d',0,'e',0,'v',0,'i',0,'c',0,'e',0,'.',0,'c',0,'o',0,'m',0};
unsigned char __code cmd_ack_ok[]={'S'};
//unsigned char __code cmd_ack_fail[]={'F'};
//cdc参数初始化波特率为57600，1停止位，无校验，8数据位。
__xdata uint8_t line_codeing[7]={0x00,0xe1,0x00,0x00,0x00,0x00,0x08};

volatile __idata uint8_t usb_ep_count = 0;     //代表USB端点接收到的数据
volatile __idata uint8_t usb_buf_point = 0;    //取数据指针
volatile __idata uint8_t ep2_busy = 0;         //上传端点是否忙标志
volatile __idata uint8_t motor_en = 0;		   //motor running
volatile __idata uint8_t cmd_lock = 0;		   //command lock


/*******************************************************************************
 * Function Name  : USBDeviceCfg()
 * Description    : USB设备模式配置
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void usb_device_cfg()
{
	USB_CTRL = 0x00;                                                           //清空USB控制寄存器
	USB_CTRL &= ~bUC_HOST_MODE;                                                //该位为选择设备模式
	USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB设备和内部上拉使能,在中断期间中断标志未清除前自动返回NAK
	USB_DEV_AD = 0x00;                                                         //设备地址初始化
	//     USB_CTRL |= bUC_LOW_SPEED;
	//     UDEV_CTRL |= bUD_LOW_SPEED;                                         //选择低速1.5M模式
	USB_CTRL &= ~bUC_LOW_SPEED;
	UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //选择全速12M模式，默认方式
	UDEV_CTRL = bUD_PD_DIS;  // 禁止DP/DM下拉电阻
	UDEV_CTRL |= bUD_PORT_EN;                                                  //使能物理端口
}
/*******************************************************************************
 * Function Name  : USBDeviceIntCfg()
 * Description    : USB设备模式中断初始化
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void usb_irq_cfg()
{
	USB_INT_EN |= bUIE_SUSPEND;                                               //使能设备挂起中断
	USB_INT_EN |= bUIE_TRANSFER;                                              //使能USB传输完成中断
	USB_INT_EN |= bUIE_BUS_RST;                                               //使能设备模式USB总线复位中断
	USB_INT_FG |= 0x1F;                                                       //清中断标志
	IE_USB = 1;                                                               //使能USB中断
	EA = 1;                                                                   //允许单片机中断
}
/*******************************************************************************
 * Function Name  : USBDeviceEndPointCfg()
 * Description    : USB设备模式端点配置，模拟兼容HID设备，除了端点0的控制传输，还包括端点2批量上下传
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void usb_endpoint_cfg()
{
	// TODO: Is casting the right thing here? What about endianness?
	UEP1_DMA = (uint16_t) ep1_buffer;                                           //端点1 发送数据传输地址
	UEP2_DMA = (uint16_t) ep2_buffer;                                           //端点2 IN数据传输地址
	UEP2_3_MOD = 0xCC;                                                         //端点2/3 单缓冲收发使能
	UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //端点2自动翻转同步标志位，IN事务返回NAK，OUT返回ACK

	UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                                 //端点1自动翻转同步标志位，IN事务返回NAK
	UEP0_DMA = (uint16_t) ep0_buffer;                                           //端点0数据传输地址
	UEP4_1_MOD = 0X40;                                                         //端点1上传缓冲区；端点0单64字节收发缓冲区
	UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //手动翻转，OUT事务返回ACK，IN事务返回NAK
}

void usb_irq(void) __interrupt (INT_NO_USB)                       //USB中断服务程序,使用寄存器组1
{
	uint16_t len;
	if(UIF_TRANSFER)                                                            //USB传输完成标志
	{
		switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
		{
			case UIS_TOKEN_IN | 1:                                                  //endpoint 1# 端点中断上传
				UEP1_T_LEN = 0;
				UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //默认应答NAK
				break;
			case UIS_TOKEN_IN | 2:                                                  //endpoint 2# 端点批量上传
					UEP2_T_LEN = 0;                                                 //预使用发送长度一定要清空
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;       //默认应答NAK
					ep2_busy = 0;                                              //清除忙标志
				break;
			case UIS_TOKEN_OUT | 2:                                                 //endpoint 3# 端点批量下传
				if ( U_TOG_OK )                                                     // 不同步的数据包将丢弃
				{
					usb_ep_count = USB_RX_LEN;
					usb_buf_point = 0;                                             //取数据指针复位
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //收到一包数据就NAK，主函数处理完，由主函数修改响应方式
				}
				break;
			case UIS_TOKEN_SETUP | 0:                                               //SETUP事务
				len = USB_RX_LEN;
				if(len == (sizeof(USB_SETUP_REQ)))
				{
					setup_len = ((uint16_t)USB_SETUP_BUF->wLengthH<<8) | (USB_SETUP_BUF->wLengthL);
					len = 0;                                                        // 默认为成功并且上传0长度
					setup_req = USB_SETUP_BUF->bRequest;
					if ( ( USB_SETUP_BUF->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )//非标准请求
					{
						switch( setup_req )
						{
							case GET_LINE_CODING:   	 //0x21  currently configured
								p_desc = line_codeing;
								len = sizeof(line_codeing);
								len = setup_len >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : setup_len;  // 本次传输长度
								memcpy(ep0_buffer,p_desc,len);
								setup_len -= len;
								p_desc += len;
								break;
							case SET_CONTROL_LINE_STATE: //0x22  generates RS-232/V.24 style control signals
								break;
							case SET_LINE_CODING:        //0x20  Configure
								break;
							default:
								len = 0xFF; //命令不支持
								break;
						}
					}
					else  //标准请求
					{
						switch(setup_req)   //请求码
						{
							case USB_GET_DESCRIPTOR:
								switch(USB_SETUP_BUF->wValueH)
								{
									case 1:                 //设备描述符
										p_desc = dev_desc;   //把设备描述符送到要发送的缓冲区
										len = sizeof(dev_desc);
										break;
									case 2:                 //配置描述符
										p_desc = config_desc;   //把设备描述符送到要发送的缓冲区
										len = sizeof(config_desc);
										break;
									case 3:
										if(USB_SETUP_BUF->wValueL == 0)
										{
											p_desc = lang_desc;
											len = sizeof(lang_desc);
										}
										else if(USB_SETUP_BUF->wValueL == 1)
										{
											p_desc = manufacturer_desc;
											len = sizeof(manufacturer_desc);
										}
										else if(USB_SETUP_BUF->wValueL == 2)
										{
											p_desc = product_desc;
											len = sizeof(product_desc);
										}
										else
										{
											p_desc = sn_desc;
											len = sizeof(sn_desc);
										}
										break;
									default:
										len = 0xff;  //不支持的命令或者出错
										break;
								}
								if ( setup_len > len )
								{
									setup_len = len;    //限制总长度
								}
								len = setup_len >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : setup_len;                            //本次传输长度
								memcpy(ep0_buffer,p_desc,len);                                  //加载上传数据
								setup_len -= len;
								p_desc += len;
								break;
							case USB_SET_ADDRESS:
								setup_len = USB_SETUP_BUF->wValueL;                              //暂存USB设备地址
								break;
							case USB_GET_CONFIGURATION:
								ep0_buffer[0] = usb_cfg;
								if ( setup_len >= 1 )
								{
									len = 1;
								}
								break;
							case USB_SET_CONFIGURATION:
								usb_cfg = USB_SETUP_BUF->wValueL;
								break;
							case USB_GET_INTERFACE:
								break;
							case USB_CLEAR_FEATURE:                                            //Clear Feature
								if( ( USB_SETUP_BUF->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* 清除设备 */
								{
									if( ( ( ( uint16_t )USB_SETUP_BUF->wValueH << 8 ) | USB_SETUP_BUF->wValueL ) == 0x01 )
									{
										if( config_desc[ 7 ] & 0x20 )
										{
											/* 唤醒 */
										}
										else
										{
											len = 0xFF;                                        /* 操作失败 */
										}
									}
									else
									{
										len = 0xFF;                                            /* 操作失败 */
									}
								}
								else if ( ( USB_SETUP_BUF->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// 端点
								{
									switch( USB_SETUP_BUF->wIndexL )
									{
										case 0x83:
											UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
											break;
										case 0x03:
											UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
											break;
										case 0x82:
											UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
											break;
										case 0x02:
											UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
											break;
										case 0x81:
											UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
											break;
										case 0x01:
											UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
											break;
										default:
											len = 0xFF;                                         // 不支持的端点
											break;
									}
								}
								else
								{
									len = 0xFF;                                                // 不是端点不支持
								}
								break;
							case USB_SET_FEATURE:                                          /* Set Feature */
								if( ( USB_SETUP_BUF->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* 设置设备 */
								{
									if( ( ( ( uint16_t )USB_SETUP_BUF->wValueH << 8 ) | USB_SETUP_BUF->wValueL ) == 0x01 )
									{
										if( config_desc[ 7 ] & 0x20 )
										{
											/* 休眠 */
#ifdef DEBUG
											printf( "suspend1\n" );                                                             //睡眠状态
#endif
											while ( XBUS_AUX & bUART0_TX )
											{
												;    //等待发送完成
											}
											SAFE_MOD = 0x55;
											SAFE_MOD = 0xAA;
											WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //USB或者RXD0/1有信号时可被唤醒
											PCON |= PD;                                                                 //睡眠
											SAFE_MOD = 0x55;
											SAFE_MOD = 0xAA;
											WAKE_CTRL = 0x00;
										}
										else
										{
											len = 0xFF;                                        /* 操作失败 */
										}
									}
									else
									{
										len = 0xFF;                                            /* 操作失败 */
									}
								}
								else if( ( USB_SETUP_BUF->bRequestType & 0x1F ) == USB_REQ_RECIP_ENDP )             /* 设置端点 */
								{
									if( ( ( ( uint16_t )USB_SETUP_BUF->wValueH << 8 ) | USB_SETUP_BUF->wValueL ) == 0x00 )
									{
										switch( ( ( uint16_t )USB_SETUP_BUF->wIndexH << 8 ) | USB_SETUP_BUF->wIndexL )
										{
											case 0x83:
												UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点3 IN STALL */
												break;
											case 0x03:
												UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点3 OUT Stall */
												break;
											case 0x82:
												UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
												break;
											case 0x02:
												UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
												break;
											case 0x81:
												UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
												break;
											case 0x01:
												UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点1 OUT Stall */
											default:
												len = 0xFF;                                    /* 操作失败 */
												break;
										}
									}
									else
									{
										len = 0xFF;                                      /* 操作失败 */
									}
								}
								else
								{
									len = 0xFF;                                          /* 操作失败 */
								}
								break;
							case USB_GET_STATUS:
								ep0_buffer[0] = 0x00;
								ep0_buffer[1] = 0x00;
								if ( setup_len >= 2 )
								{
									len = 2;
								}
								else
								{
									len = setup_len;
								}
								break;
							default:
								len = 0xff;                                                    //操作失败
								break;
						}
					}
				}
				else
				{
					len = 0xff;                                                         //包长度错误
				}
				if(len == 0xff)
				{
					setup_req = 0xFF;
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
				}
				else if(len <= DEFAULT_ENDP0_SIZE)                                                       //上传数据或者状态阶段返回0长度包
				{
					UEP0_T_LEN = len;
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1，返回应答ACK
				}
				else
				{
					UEP0_T_LEN = 0;  //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
					UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1,返回应答ACK
				}
				break;
			case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
				switch(setup_req)
				{
					case USB_GET_DESCRIPTOR:
						len = setup_len >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : setup_len;                                 //本次传输长度
						memcpy( ep0_buffer, p_desc, len );                                   //加载上传数据
						setup_len -= len;
						p_desc += len;
						UEP0_T_LEN = len;
						UEP0_CTRL ^= bUEP_T_TOG;                                             //同步标志位翻转
						break;
					case USB_SET_ADDRESS:
						USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | setup_len;
						UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
					default:
						UEP0_T_LEN = 0;   //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
						UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
						break;
				}
				break;
			case UIS_TOKEN_OUT | 0:       // endpoint0 OUT
				if(setup_req ==SET_LINE_CODING)  //设置串口属性
				{
					if( U_TOG_OK )
					{
						UEP0_T_LEN = 0;
						UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // 准备上传0包
					}
				}
				else
				{
					UEP0_T_LEN = 0;
					UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_NAK;      //状态阶段，对IN响应NAK
				}
				break;



			default:
				break;
		}
		UIF_TRANSFER = 0;    //写0清空中断
	}
	if(UIF_BUS_RST)          //设备模式USB总线复位中断
	{
#ifdef DEBUG
		printf( "reset\n" ); //睡眠状态
#endif
		motor_stop();
		UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
		UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
		USB_DEV_AD = 0x00;
		UIF_SUSPEND = 0;
		UIF_TRANSFER = 0;
		UIF_BUS_RST = 0;     //清中断标志
		usb_ep_count = 0;    //USB端点收到的长度
		usb_cfg = 0;       //清除配置值
		ep2_busy = 0;
		motor_en = 0;
		cmd_lock = 0;
	}
	if (UIF_SUSPEND)         //USB总线挂起/唤醒完成
	{
		UIF_SUSPEND = 0;
		if ( USB_MIS_ST & bUMS_SUSPEND )  //挂起
		{
			motor_stop();
#ifdef DEBUG
			printf( "suspend2\n" );        //睡眠状态
#endif
			while ( XBUS_AUX & bUART0_TX )
			{
				;    //等待发送完成
			}
			SAFE_MOD = 0x55;
			SAFE_MOD = 0xAA;
			WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;  //USB或者RXD0/1有信号时可被唤醒
			PCON |= PD;                                             //睡眠
			SAFE_MOD = 0x55;
			SAFE_MOD = 0xAA;
			WAKE_CTRL = 0x00;
		}
	}
	else {                                                          //意外的中断,不可能发生的情况
		USB_INT_FG = 0xFF;                                          //清中断标志

	}
}

void cmd_ack(uint8_t* data, uint8_t len) {
	if(!ep2_busy)   //端点不繁忙（空闲后的第一包数据，只用作触发上传）
	{
		//写上传端点
		//memcpy(ep2_buffer+MAX_PACKET_SIZE,&cmd_ack_ok[0],1);
		memcpy(ep2_buffer+MAX_PACKET_SIZE, data, len);
		UEP2_T_LEN = len;                                           //预使用发送长度一定要清空
		UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;   //应答ACK
		ep2_busy = 1;
	}
}

//主函数
void main()
{
	config_clock();
	mdelay(10);                                                     //修改主频等待内部晶振稳定,必加
#ifdef DEBUG
	uart0_init();                                                   //串口0,可以用于调试
	printf("start ...\n");
#endif
	motor_init();
	usb_device_cfg();
	usb_endpoint_cfg();                                             //端点配置
	usb_irq_cfg();                                                  //中断初始化
	UEP0_T_LEN = 0;
	UEP1_T_LEN = 0;                                                 //预使用发送长度一定要清空
	UEP2_T_LEN = 0;                                                 //预使用发送长度一定要清空

	while(1)
	{
		if (M_DET && motor_en) {
#ifdef DEBUG
			printf("motor_stop\n");
#endif
			motor_stop();
			motor_en = 0;
		}

		if(usb_cfg)
		{
			if(usb_ep_count)   //USB接收端点有数据
			{
				switch(ep2_buffer[usb_buf_point]) {
					case 0x1B:
						cmd_lock = 1;
						break;
					case 0x30:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_1();
						}
						break;
					case 0x31:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_2();
						}
						break;
					case 0x32:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_3();
						}
						break;
					case 0x33:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_4();
						}
						break;
					case 0x34:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_5();
						}
						break;
					case 0x35:
						if (cmd_lock && motor_en == 0) {
							cmd_ack(cmd_ack_ok,1);
							cmd_lock = 0;
							motor_en = 1;
							motor_6();
						}
						break;
					case 0x40:
#ifdef DEBUG
						printf("reset\n");
						mdelay(10);
#endif
						motor_stop();
						chip_reset();
						break;
					case 0x43:
						if (cmd_lock) {
							uint8_t unique_id [4];
							unique_id[0] = *(uint8_t __code*)(0x3FFF);
							unique_id[1] = *(uint8_t __code*)(0x3FFE);
							unique_id[2] = *(uint8_t __code*)(0x3FFD);
							unique_id[3] = *(uint8_t __code*)(0x3FFC);
							cmd_ack(unique_id, 4);
							cmd_lock = 0;
						}
						break;
					default:
#ifdef DEBUG
						printf("wakeup\n");
#endif
						cmd_lock = 0;
						break;
				}
#ifdef DEBUG
				uart0_write(ep2_buffer[usb_buf_point]);
#endif
				usb_ep_count--;
				usb_buf_point++;
				if(usb_ep_count==0)
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;

			}
		}
	}
}
