#include "stm32f10x.h"
#include "Configuration.h"
#include "TaskManager.h"
#include "USART.h"
#include "Timer.h"
#include "ADC.h"
#include "flash.h"
#include "RemoteControl.h"
#include "LED.h"
#include "InputCapture_TIM.h"
#include "Communication.h"


InputCapture_TIM hunter(TIM4, 400, true, true, true, true); //TIM4 as InputCapture for remoter controller
USART com1(1,115200,true);
USART com2(2,115200,true);

Communication Hi(com1,com2); 

ADC pressure(1); //PA1读取AD值
//flash InfoStore(0x08000000+100*MEMORY_PAGE_SIZE,true);     //flash

//GPIO ledRedGPIO(GPIOB,0,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
GPIO ledBlueGPIO(GPIOB,5,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
//LED ledRed(ledRedGPIO);//LED red
//LED ledBlue(ledBlueGPIO);//LED blue
RemoteControl RC(&hunter,2,3,4,1); //P THR YAW ROLL


int main()
{
		u8 State=REMOTECONTROL_LOCK;
		u8 OldState=REMOTECONTROL_LOCK;
		double Receive_data=0;  //接收数据  10ms
		double RcUpdata=0;      //遥控器状态更新时间  20ms
		ledBlueGPIO.SetLevel(0);
	while(1)
	{	
		if(tskmgr.TimeSlice(Receive_data,0.01) ) //0.01
		{
			Hi.DataListening_SendPC();//监听飞机发送来的数据转发给PC
			Hi.DataListening_SendCopter();//监听PC发送来的数据转发给飞机
		}
		
		if(tskmgr.TimeSlice(RcUpdata,0.08) )
		{
			State=RC.Updata(80,2000);	
			Hi.SendData2Copter(RC.GetYawVal(),RC.GetThrottleVal(),RC.GetRollVal(),RC.GetPitchVal(),false);	
			
			if(State == REMOTECONTROL_LOCK &&  OldState ==REMOTECONTROL_UNLOCK )
			{
					OldState = REMOTECONTROL_LOCK;
					Hi.SendOrder(0XA0);
					ledBlueGPIO.SetLevel(1);//上锁状态为亮
			}
			else if(State ==REMOTECONTROL_UNLOCK && OldState ==REMOTECONTROL_LOCK)//解锁
			{	
					Hi.SendOrder(0XA1);
					OldState = REMOTECONTROL_UNLOCK;
					ledBlueGPIO.SetLevel(0);//解锁状态为灭
					//tskmgr.DelayS(1);
			}		
			if(State == REMOTECONTROL_UNLOCK) //如果解锁了才向上位机发送
					Hi.SendData2Copter(RC.GetYawVal(),RC.GetThrottleVal(),RC.GetRollVal(),RC.GetPitchVal(),true);			
			
			
		}	
		
	}
}


/*
遥控器丢失情况
1.上电丢失，一开始就没检查到遥控器 （全为0）
2.使用中丢失  这个比较麻烦。。断开后接收器依然发送断开时刻的数据
*/
