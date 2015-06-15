#pragma once
#include "DepthTo3D.h"
#include "OpenGLStatic.h"
#include "KinectDataStream.h"
#include "DataProcess.h"

#define RTASK_NONE				0
#define RTASK_GOTO				1
#define RTASK_FIND_OBJ_DESK		2
#define RTASK_GRAB_OBJ			3
#define RTASK_PASS_OBJ_PERSON	4

#define RTASK_FOLLOW			5

#define MAX_LIFT_POS		9500	//升降位置最大值
#define MAX_SHOULDER_POS	4200	//肩膀抬起最大值

class CRobotCore
{
public:
	CRobotCore();
	~CRobotCore();


	COpenGLStatic* pGL;
	CDepthTo3D m_cDepthTo3D;
	CDataProcess m_DataProcess;

	HANDLE	m_hToProcessEvent;
	HANDLE	m_hShutdownEvent;
	HANDLE	m_hEventArray[2];
	BOOL	m_bThreadAlive;

	int nAutoTask;		//自动的任务模式

	CKinectDataStream m_cKinectDataStream;	//数据存储对象
	int nNumToSave;		//满多少帧后存储
	bool bLogFrames;	//记录Kinect数据
	CStatic* pDisShow;	

	void Procedure();
	void TackAction();
	static UINT PointCloudsThread(LPVOID pParam);
	void ProcessDepth(USHORT* inBuf, int inWidth, int inHeight, USHORT nMinDepth, USHORT nMaxDepth);
	void ProcessRGB(UCHAR* inBuf, int inWidth, int inHeight);
	void m_ProcessOneFrame();
	void ProcessEvent();
	void StartLogKinectStream(int inFrameNum);
};

