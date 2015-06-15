/************************************************************
Copyright (C), 2015-2100, WaterPlus. Co., Ltd.
Author:	张万杰
Version : 1.02
Email: zhangwanjie@126.com
***********************************************************/
#pragma once

// Kinect Header files
#include "RobotCore.h"
#include <Kinect.h>

class CKinect2
{
public:
	CKinect2();
	~CKinect2();

	CListBox* pInfoList;
	volatile bool bToExit;
	CRobotCore* pRobotCore;

	//RGB彩色
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	RGBQUAD*                m_pColorRGBX;
	unsigned char*			m_pColorBuf;
	DepthSpacePoint*		cColorToDepthPoints;

	//Depth深度
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
	DepthSpacePoint			*depthPoints;
	CameraSpacePoint		*cameraPoints;

	volatile bool m_bThreadAlive;

	//坐标映射
	ColorSpacePoint			*colorSpacePoints;

	// Current Kinect
	IKinectSensor*          m_pKinectSensor;
	// Color reader
	IColorFrameReader*      m_pColorFrameReader;
	// Depth reader
	IDepthFrameReader*      m_pDepthFrameReader;
	// Body reader
	IBodyFrameReader*       m_pBodyFrameReader;
	// CoordinateMapper
	ICoordinateMapper*      m_pCoordinateMapper;

	HRESULT Start();
	static UINT MainLoopThread(LPVOID pParam);

	void NewDepth(INT64 nTime, const UINT16* pBuffer, int nHeight, int nWidth, USHORT nMinDepth, USHORT nMaxDepth);
	void NewColor(INT64 nTime, RGBQUAD* pBuffer, int nWidth, int nHeight); 
	void NewBody(INT64 nTime, int nBodyCount, IBody** ppBodies);
	void PrintInfo(CString inStr);
	void Stop();
};


// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

