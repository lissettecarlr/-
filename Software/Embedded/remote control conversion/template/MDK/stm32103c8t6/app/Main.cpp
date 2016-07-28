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


InputCapture_TIM hunter(TIM4, 400, true, true, true, true); //TIM4 as InputCapture for remoter controller
USART com(1,115200,false);

ADC pressure(1); //PA1读取AD值
//flash InfoStore(0x08000000+100*MEMORY_PAGE_SIZE,true);     //flash

//GPIO ledRedGPIO(GPIOB,0,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
//GPIO ledBlueGPIO(GPIOB,1,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
//LED ledRed(ledRedGPIO);//LED red
//LED ledBlue(ledBlueGPIO);//LED blue
RemoteControl RC(&hunter,1,3,2,4);

volatile double curTime=0, oldTime=0, deltaT=0;
int main()
{
	while(1)
	{	
		curTime = tskmgr.Time();
		deltaT = curTime-oldTime;
		if(deltaT >= 0.1)
		{
			
			oldTime = curTime;
			u8 state=RC.Updata(100,2000);

			if(state==REMOTECONTROL_UNLOCK)
			{
				com<<"UNLock"<<state<<"THR:"<<RC.GetThrottleVal()<<"\t YAW:"<<RC.GetYawVal()<<"\t ROLL:"<<RC.GetRollVal()<<"\t PITCH:"<<RC.GetPitchVal()<<"\n";
			}
			if(state==REMOTECONTROL_LOCK)
			{
				//com<<"Lock"<<state<<"\t"<<RC[1]<<"\t"<<RC[2]<<"\t"<<RC[3]<<"\t"<<RC[4]<<"\n";
				//com<<"PIT:"<<RC.GetOriginalValue(1)<<"\tTHR:"<<RC.GetOriginalValue(2)<<"\tYAW:"<<RC.GetOriginalValue(3)<<"\tROLL:"<<RC.GetOriginalValue(4)<<"\n";
				com<<"Lock"<<state<<"THR:"<<RC.GetThrottleVal()<<"\t YAW:"<<RC.GetYawVal()<<"\t ROLL:"<<RC.GetRollVal()<<"\t PITCH:"<<RC.GetPitchVal()<<"\n";
			}
		}
	}
}
