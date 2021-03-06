#include "HMC5883L.h"




#ifdef HMC5883L_USE_TASKMANAGER
HMC5883L::HMC5883L(I2C &i2c,u16 maxUpdateFrequency)
{
	mI2C=&i2c;
	mMaxUpdateFrequency=maxUpdateFrequency;
	mIsCalibrated = true;
	mOffsetRatio(1,1,1);
	mOffsetBias(0,0,0);
	//this->Init();
}
#else
HMC5883L::HMC5883L(I2C &i2c)
{
	mI2C=&i2c;
	mIsCalibrated = true;
	mOffsetRatio(1,1,1);
	mOffsetRatio(0,0,0);
	//this->Init();
}
#endif
bool HMC5883L::Init(bool wait)
{
	if(wait)
		mI2C->WaitTransmitComplete();
	//测试磁力计是否存在
	if(!TestConnection(false))
	{
		DEBUG_LOG<<"mag connection error\n";
		return false;
	}
	
	unsigned char IIC_Write_Temp;
	IIC_Write_Temp = HMC5883L_AVERAGING_8 | HMC5883L_RATE_75 | HMC5883L_BIAS_NORMAL;   
	mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Config_RA,&IIC_Write_Temp,1,0,0); //Config Register A  :number of samples averaged->8  Data Output rate->30Hz
	IIC_Write_Temp = HMC5883L_GAIN_1090;   //00100000B
	mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Config_RB,&IIC_Write_Temp,1,0,0);//Config Register B:  Gain Configuration as : Sensor Field Range->(+-)1.3Ga ; Gain->1090LSB/Gauss; Output Range->0xF800-0x07ff(-2048~2047)
	IIC_Write_Temp = HMC5883L_MODE_CONTINUOUS;   //00000000B
	mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Mode,&IIC_Write_Temp,1,0,0);//Config Mode as: Continous Measurement Mode
	if(!mI2C->StartCMDQueue())
		return false;
	if(wait)
	{
		if(!this->mI2C->WaitTransmitComplete(true,true,false))//失败
		{
			this->mHealth=0;
			return false;
		}
		else
			this->mHealth=1;
	}
	
	return true;
}


unsigned char HMC5883L::GetHealth()
{
	return this->mHealth;
}

#ifdef HMC5883L_USE_TASKMANAGER
void HMC5883L::SetMaxUpdateFrequency(u16 maxUpdateFrequency)
{
	mMaxUpdateFrequency=maxUpdateFrequency;
}

u16 HMC5883L::GetMaxUpdateFrequency()
{
	return mMaxUpdateFrequency;
}
#endif
////////////////////////
///Verify the connection of sensor
///@param isClearIicCmdQueue if clear the iic command queue. 
///  true:clear  false:It will wait the completion of queue comand befor execute connection test
///@retval 0:error ocurred 1:exist
///@attention it will wait in this function until the connection status is detected
///////////////////////
bool HMC5883L::TestConnection(bool isClearIicCmdQueue)
{
	if(!isClearIicCmdQueue)
	{
		mI2C->WaitTransmitComplete();
	}
	else
	{
		//clear command queue
		this->mI2C->ClearCommand();
	}
	//test if i2c is healthy
	if(!this->mI2C->IsHealth())
	{
		this->mI2C->Init();
	}
		
	this->mData.identification_A=0;	
	this->mData.identification_B=0;
	this->mData.identification_C=0;
	//add read command
	this->mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Identification_A,0,0,&(this->mData.identification_A),3);//向队列添加新的读取字节的命令
	//start command queue
	this->mI2C->StartCMDQueue();
	//wait until received right data or error occured
	if(!this->mI2C->WaitTransmitComplete(true,true,false))//失败
	{
		this->mHealth=0;
		return false;
	}
	if((this->mData.identification_A==0x48) && (this->mData.identification_B==0x34) && (this->mData.identification_C==0x33))
	{
		this->mHealth=1;
		return true;
	}
	this->mHealth=0;
	return false;
}


/////////////////////
///Get status that if the data is ready
///@retval true:ready false:not ready
///@attention it will wait until command queue execute complete
/////////////////////
bool HMC5883L::GetReadyStatus()
{
	if(mData.mag_Status&0x01)
		return true;
	return false;
}

///////////////////////
///Get status that if the data register is locked 
///@retval true:locked false:not locked
///@attention it will wait until command queue execute complete
///////////////////////
bool HMC5883L::IsLocked()
{
	if(mData.mag_Status&0x02)
		return true;
	return false;
}



//////////////////////
///更新数据
/////////////////////
u8 HMC5883L::Update(bool wait,Vector3<int> *mag)
{
	#ifdef HMC5883L_USE_TASKMANAGER
	static double oldTime=0,timeNow=0;
	timeNow = TaskManager::Time();
	if((timeNow-oldTime)< (1.0/this->mMaxUpdateFrequency))
	{
		return MOD_BUSY;
	}
	oldTime=timeNow;
	#endif
	
	
	if(wait)
		mI2C->WaitTransmitComplete();
	//this->mData.mag_Status=0x00;
	mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_XOUT_M,0,0,&(mData.mag_XH),6,true,this);//向队列添加新的读取字节的命令
	mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Status,0,0,&(mData.mag_Status),1);//向队列添加新的读取字节的命令
	if(this->IsLocked())
	{
		static unsigned char IIC_Write_Temp=0;//数据被锁存，三种方式恢复：1：全部读取完毕；2：模式改变；3：配置发生变化
//		IIC_Write_Temp = HMC5883L_MODE_SINGLE;   //00000000B
//		mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Mode,&IIC_Write_Temp,1,0,0);//Config Mode as: Continous Measurement Mode
		IIC_Write_Temp = HMC5883L_MODE_CONTINUOUS;   //00000000B
		mI2C->AddCommand(HMC5883_ADDRESS,HMC5883_Mode,&IIC_Write_Temp,1,0,0);//Config Mode as: Continous Measurement Mode
	}
	if(!mI2C->StartCMDQueue())
	{
		this->mHealth=0;
		mI2C->ClearCommand();//清除队列中的命令
		mI2C->Init();
		this->Init(wait);
		return false;
	}
	if(wait)
		if(!mI2C->WaitTransmitComplete(true,true,false))
		{
			this->Init(wait);
			return false;
		}
	//校准
	if(!mIsCalibrated)
	{
		static int limitValue[6]={0,0,0,0,0,0};//三个轴的极限值（最小最大值）
		static bool firstTime=true;
		static float lastUpdateLimitValueTime = 0;//记录最近更新最大最小值的时间
		static int magRaw[3];
		
		if(GetNoCalibrateDataRaw().x <2000 && GetNoCalibrateDataRaw().x>-2000)
			 magRaw[0] = GetNoCalibrateDataRaw().x;
		if(GetNoCalibrateDataRaw().y <2000 && GetNoCalibrateDataRaw().y>-2000)
			 magRaw[1] = GetNoCalibrateDataRaw().y;
		if(GetNoCalibrateDataRaw().z <2000 && GetNoCalibrateDataRaw().z>-2000)
			 magRaw[2] = GetNoCalibrateDataRaw().z;
		
//		if(!(GetNoCalibrateDataRaw().x>3000) && !(GetNoCalibrateDataRaw().y>3000) &&!(GetNoCalibrateDataRaw().z>3000))
//		{
//		  magRaw[0] = GetNoCalibrateDataRaw().x;
//		  magRaw[1] = GetNoCalibrateDataRaw().y;
//		  magRaw[2] = GetNoCalibrateDataRaw().z;//原始值
//		}
		
		if(firstTime)//刚开始校准,给初值
		{
			firstTime = false;
			lastUpdateLimitValueTime = TaskManager::Time();
			for(u8 i=0;i<3;++i)
			{
				limitValue[2*i] = magRaw[i];
				limitValue[2*i+1] = magRaw[i];
			}
		}
		else
		{
			for(u8 i=0;i<3;++i)
			{
				if(magRaw[i]<limitValue[2*i])
				{
					limitValue[2*i] = magRaw[i];
					lastUpdateLimitValueTime = TaskManager::Time();
				}
				else if(magRaw[i]>limitValue[2*i+1])
				{
					limitValue[2*i+1] = magRaw[i];
					lastUpdateLimitValueTime = TaskManager::Time();
				}
			}
			//三个轴的区间
			xMaxMinusMin = limitValue[1] -limitValue[0];
			yMaxMinusMin = limitValue[3] -limitValue[2];
			zMaxMinusMin = limitValue[5] -limitValue[4];
			if(TaskManager::Time() - lastUpdateLimitValueTime > 10)//10秒钟保持极限值不变，停止校准
			{
				if(xMaxMinusMin>yMaxMinusMin && xMaxMinusMin>zMaxMinusMin)
				{
					mOffsetRatio.x = 1.0;
					mOffsetRatio.y = 1.0*xMaxMinusMin/yMaxMinusMin;
					mOffsetRatio.z = 1.0*xMaxMinusMin/zMaxMinusMin;
				}
				else if(yMaxMinusMin>xMaxMinusMin && yMaxMinusMin>zMaxMinusMin)
				{
					mOffsetRatio.x = 1.0*yMaxMinusMin/xMaxMinusMin;
					mOffsetRatio.y = 1.0;
					mOffsetRatio.z = 1.0*yMaxMinusMin/zMaxMinusMin;
				}
				else if(zMaxMinusMin>xMaxMinusMin && zMaxMinusMin>yMaxMinusMin)
				{
					mOffsetRatio.x = 1.0*xMaxMinusMin/yMaxMinusMin;
					mOffsetRatio.y = 1.0*xMaxMinusMin/yMaxMinusMin;
					mOffsetRatio.z = 1.0;
				}
				mOffsetBias.x = mOffsetRatio.x*(xMaxMinusMin*0.5f - limitValue[1]);
				mOffsetBias.y = mOffsetRatio.y*(yMaxMinusMin*0.5f - limitValue[3]);
				mOffsetBias.z = mOffsetRatio.z*(zMaxMinusMin*0.5f - limitValue[5]);
				//检查校准值有效性
				{
			    
				}
				//标志复位
				firstTime = true;
				mIsCalibrated = true;
			}
		}
	}
	return true;
}


Vector3<int>  HMC5883L::GetDataRaw()
{
	Vector3<int> temp;
	temp.x= mOffsetRatio.x *(((signed short int)(mData.mag_XH<<8)) | mData.mag_XL) +mOffsetBias.x;
	temp.y= mOffsetRatio.y *(((signed short int)(mData.mag_YH<<8)) | mData.mag_YL) +mOffsetBias.y;
	temp.z= mOffsetRatio.z *(((signed short int)(mData.mag_ZH<<8)) | mData.mag_ZL) +mOffsetBias.z;
	return temp;
}

Vector3<int> HMC5883L::GetNoCalibrateDataRaw()
{
	Vector3<int> temp;
	temp.x= (((signed short int)(mData.mag_XH<<8)) | mData.mag_XL) ;
	temp.y= (((signed short int)(mData.mag_YH<<8)) | mData.mag_YL) ;
	temp.z=((signed short int)(mData.mag_ZH<<8)) | mData.mag_ZL;
	return temp;
}


float HMC5883L::Heading()
{
	double headingDegrees,headingRadians;
	int x,y/*,z*/;
	x=(s16)(this->mData.mag_XH<<8|this->mData.mag_XL);
	y=(s16)(this->mData.mag_YH<<8|this->mData.mag_YL);
//	z=(int)(IMU->mag_ZH<<8|IMU->mag_ZL);
	
  headingRadians = atan2((double)((y)/*-offsetY*/),(double)((x)/*-offsetX*/));
  // Correct for when signs are reversed.
  if(headingRadians < 0)
    headingRadians += 2*PI;
 
  headingDegrees = headingRadians * 180/M_PI;
  headingDegrees += MagnetcDeclination; //the magnetc-declination angle 
 
  // Check for wrap due to addition of declination.
  if(headingDegrees > 360)
    headingDegrees -= 360;
 
  return headingDegrees;
}


double HMC5883L::GetUpdateInterval()
{
	return Interval();
}



bool HMC5883L::StartCalibrate()
{
	mIsCalibrated = false;
	return true;
}

bool HMC5883L::IsCalibrated()
{
	return mIsCalibrated;
}
//获取磁力计校准的偏置
Vector3f HMC5883L::GetOffsetBias()
{
	return mOffsetBias;
}

//获取磁力计校准的比例
Vector3f HMC5883L::GetOffsetRatio()
{
	return mOffsetRatio;
}

//获取磁力计校准的偏置
void HMC5883L::SetOffsetBias(float x,float y,float z)
{
	Vector3f bias(x,y,z);
	mOffsetBias = bias;
}

//获取磁力计校准的比例
void HMC5883L::SetOffsetRatio(float x,float y,float z)
{
	Vector3f ratio(x,y,z);
	mOffsetRatio = ratio;
}
