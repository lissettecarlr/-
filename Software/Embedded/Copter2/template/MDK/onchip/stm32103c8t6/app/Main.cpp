//#include "stdlib.h"
//#include "stdio.h"

#include "stm32f10x.h"
#include "Configuration.h"
#include "TaskManager.h"
#include "USART.h"
#include "I2C.h"
#include "Timer.h"
#include "ADC.h"
#include "PWM.h"
#include "flash.h"
#include "mpu6050.h"
#include "HMC5883L.h"
#include "Moto.h"
#include "Control.h"
#include "Communication.h"
#include "IMU.h"
#include "LED.h"


#define IMU_UPDATE_CONTROL_TIME  0.008
#define RCV_DATE_TIME            0.02
#define SEND_DATE_TIME           0.1
#define HINT_TIME                0.5


//Timer T1(TIM1,1,2,3); //使用定时器计，溢出时间:1S+2毫秒+3微秒
USART com(1,115200,false);
USART com2(2,115200,false);

Communication WIFI(com2); 


PWM pwm4(TIM4,1,1,1,1,24000);
ADC pressure(1); //PA1读取AD值
flash InfoStore(0x08000000+60*MEMORY_PAGE_SIZE,true);     //flash
//0页 存磁力计校准值  1页存PID

GPIO ledRedGPIO(GPIOB,5,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
GPIO ledYewGPIO(GPIOA,12,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);//LED GPIO
LED Red(ledRedGPIO);

I2C i2c(2);

Control control(pwm4);


int main()
{
	Red.On();
	tskmgr.DelayMs(500);
	mpu6050 MPU6050(i2c);
	tskmgr.DelayMs(500);
	HMC5883L mag(i2c);
	tskmgr.DelayS(1);
	IMU imu(MPU6050,mag);
	tskmgr.DelayS(1);
	
	
	double Updata_posture=0; //计算欧拉角 10ms
//	double Updata_Control=0; //控制 500Hz 2ms
	double Receive_date=0;  //接收数据  20ms
	double Send_date=0; //发送数据  100ms
	double Updata_hint=0; //更新提示信息状态 500ms
	
	float Vol = 0;//电压
	float magCV[6]={0};//用于保存磁力计数据
	
	ledRedGPIO.SetLevel(0);//用于表示是否处于上锁状态
	ledYewGPIO.SetLevel(0);//表示系统是否忙于做其他事
	
	pwm4.SetDuty(0,0,0,0);

	imu.init();
	
	//if(InfoStore.Read(0,0))
	//	mag.SetCalibrateRatioBias(InfoStore.Read(0,0),InfoStore.Read(0,2),InfoStore.Read(0,4),InfoStore.Read(0,6),InfoStore.Read(0,8),InfoStore.Read(0,10));
	mag.SetOffsetBias(231,-482,-446.5);
	mag.SetOffsetRatio(1,0.921,0.615);
	
		//control.SetPID_PIT(0.4,0,0.026);
		//control.SetPID_ROL(0.4,0,0.026);
	
	while(1)
	{			
		if(tskmgr.TimeSlice(Updata_posture,0.008) ) 
		{
				//更新、获得欧拉角
			imu.UpdateIMU();
			if(!WIFI.mClockState) //当解锁
			{
				control.PIDControl(imu.mAngle,MPU6050.GetGyrDegree(),WIFI.mRcvTargetThr,WIFI.mRcvTargetPitch,WIFI.mRcvTargetRoll,WIFI.mRcvTargetYaw);		
			}
		}

		
		if(tskmgr.TimeSlice(Receive_date,0.02) ) 
		{
			//接收
			WIFI.DataListening();
			
			if(WIFI.mAcc_Calibrate)//加速度校准
			{
			  WIFI.mAcc_Calibrate=false;
			}
			if( WIFI.mGyro_Calibrate)//角速度校准
			{
			  WIFI.mGyro_Calibrate=false;
				ledRedGPIO.SetLevel(0);
				imu.GyroCalibrate();
			}
			if(WIFI.mMag_Calibrate)//磁力计校准
			{
			  WIFI.mMag_Calibrate=false;
				ledRedGPIO.SetLevel(0);
				mag.StartCalibrate();
			}
		}
			
		if(tskmgr.TimeSlice(Send_date,0.1) )
		{
			//发送
			//
			if(imu.GyroIsCalibrated())
			{
				//static Vector3f x,y;
				//x=mag.GetOffsetBias();
				//y=mag.GetOffsetRatio();
//				com2<<imu.mAngle.x<<"\t"<<imu.mAngle.y<<"\t"<<imu.mAngle.z<<"\n";
				//com2<<x.x<<"  "<<x.y<<"  "<<x.z<<"  "<<y.x<<"  "<<y.y<<"  "<<y.z<<"  "<<"n";
//				static float x1=0,x2=0,y1=0,y2=0,z1=0,z2=0;
//				if(mag.GetDataRaw().x >= x1)
//					x1=mag.GetDataRaw().x;
//				if(mag.GetDataRaw().x<=x2)
//					x2=mag.GetDataRaw().x;
//				
//				if(mag.GetDataRaw().y >= y1)
//					y1=mag.GetDataRaw().y;
//				if(mag.GetDataRaw().y<=y2)
//					y2=mag.GetDataRaw().y;z
//				
//				if(mag.GetDataRaw().z >= z1)
//					z1=mag.GetDataRaw().z;
//				if(mag.GetDataRaw().z<=z2)
//					z2=mag.GetDataRaw().z;
//				
//				com2<<x1<<" "<<x2<<" "<<y1<<" "<<y2<<" "<<z1<<" "<<z2<<"  "<<mag.GetDataRaw().x<<" "<<mag.GetDataRaw().y<<" "<<mag.GetDataRaw().z<<"\n";
//				
//				
				
				//com<<MPU6050.GetAccRaw().x<<"\t"<<MPU6050.GetAccRaw().y<<"\t"<<MPU6050.GetAccRaw().z<<"\t"<<MPU6050.GetGyrRaw().x<<"\t"<<MPU6050.GetGyrRaw().y<<"\t"<<MPU6050.GetGyrRaw().z<<"\t"<<mag.GetDataRaw().x<<"\t"<<mag.GetDataRaw().y<<"\t"<<mag.GetDataRaw().z<<"\n";
				WIFI.SendCopterState(imu.mAngle.y,imu.mAngle.x,imu.mAngle.z,(u32)Vol,0,(u8)WIFI.mClockState);								
				
				WIFI.SendSensorOriginalData(MPU6050.GetAccRaw(),MPU6050.GetGyrRaw(),mag.GetDataRaw());
				WIFI.SendRcvControlQuantity();//发送接收到的舵量
								
			}
			if(WIFI.mGetPid)//如果请求了获取PID
			{
				WIFI.mGetPid=false;
				WIFI.SendPID(control.GetPID_PIT().P,control.GetPID_PIT().I,control.GetPID_PIT().D,
				control.GetPID_ROL().P,control.GetPID_ROL().I,control.GetPID_ROL().D,
				control.GetPID_YAW().P,control.GetPID_YAW().I,control.GetPID_YAW().D);
				WIFI.SendPID(control.GetPID_PIT_rate().P,control.GetPID_PIT_rate().I,control.GetPID_PIT_rate().D,
				control.GetPID_ROL_rate().P,control.GetPID_ROL_rate().I,control.GetPID_ROL_rate().D,
				control.GetPID_YAW_rate().P,control.GetPID_YAW_rate().I,control.GetPID_YAW_rate().D);
			}
			
		}
		
		if(tskmgr.TimeSlice(Updata_hint,0.5) ) //提示更新
		{
			if(WIFI.mClockState) //1锁定
				ledYewGPIO.SetLevel(0);
			else
				ledYewGPIO.SetLevel(1);
			
			if(mag.IsCalibrated())
				ledRedGPIO.SetLevel(1);

			if(imu.GyroIsCalibrated())
			{
				//InfoStore
				ledRedGPIO.SetLevel(1);
			}
	
			if(WIFI.mPidUpdata) //更新PID
			{
				WIFI.mPidUpdata=false;
				control.SetPID_PIT(WIFI.PID[0][0],WIFI.PID[0][1],WIFI.PID[0][2]);
				control.SetPID_ROL(WIFI.PID[0][3],WIFI.PID[0][4],WIFI.PID[0][5]);
				control.SetPID_YAW(WIFI.PID[0][6],WIFI.PID[0][7],WIFI.PID[0][8]);
				
				control.SetPID_PIT_rate(WIFI.PID[1][0],WIFI.PID[1][1],WIFI.PID[1][2]);
				control.SetPID_ROL_rate(WIFI.PID[1][3],WIFI.PID[1][4],WIFI.PID[1][5]);
				control.SetPID_YAW_rate(WIFI.PID[1][6],WIFI.PID[1][7],WIFI.PID[1][8]);
				
				Red.Blink(4,100,1);
			}
			Vol=pressure.Voltage_I(1,1,3,4.2)*100;//电压检测
		}
		
	}
}

