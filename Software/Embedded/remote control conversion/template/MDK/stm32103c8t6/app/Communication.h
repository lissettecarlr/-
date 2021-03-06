#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "stm32f10x.h"
#include "USART.h"

#define BYTE0(dwTemp)       ( *( (char *)(&dwTemp)		) )
#define BYTE1(dwTemp)       ( *( (char *)(&dwTemp) + 1) )
#define BYTE2(dwTemp)       ( *( (char *)(&dwTemp) + 2) )
#define BYTE3(dwTemp)       ( *( (char *)(&dwTemp) + 3) )


class Communication{
	
	private:
		USART &usart;
		USART &com433;
	
		bool Calibration(u8 *data,int lenth,u8 check);
	
	public:
		
	 Communication(USART &com1,USART &com2);
	 bool DataListening_SendPC();//数据接收监听
	 bool DataListening_SendCopter();//数据接收监听	

	//发送遥控器信息给飞机
		bool SendData2Copter(uint16_t Yaw,uint16_t Thr,uint16_t Roll,uint16_t Pitch,bool Dir);
		
	//0xa0 飞机锁定  0XA1飞机解锁
		bool SendOrder(u8 Order);
	
};



#endif
