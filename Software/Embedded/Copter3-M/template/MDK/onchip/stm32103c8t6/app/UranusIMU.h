#ifndef __UARNUSIMU_H
#define __UARNUSIMU_H

#include "stm32f10x.h"
#include "USART.h"
#include "Vector3.h"

class UarnusIMU{
	
	private:
		USART &mCom;
		Vector3<int> acc;
	  Vector3<int> gyr;
	  Vector3<int> mag;
	  Vector3f angle; //X���,Y����
	
	 
	public:
		//currectCrc: ���ڱ���У��ֵ��16λ�ȱ�����ַ��Э���ǵ�λ��ǰ��λ�ں�У��ֵ
	  //src������У�������
	  //lengthInBytes �����鳤��
	   void crc16_update(uint16_t *currectCrc, const uint8_t *src, uint32_t lengthInBytes);	
	
	  UarnusIMU(USART &com);
	  bool Update();
	
	  Vector3<int> GetAcc();
	//  Vector3<int> GetOrgGyr();
  	  Vector3<int> GetGyr();
//	  Vector3f GetGyrDegree();
	  Vector3<int> GetMag();
	  Vector3f GetAngle();
	
};



#endif
