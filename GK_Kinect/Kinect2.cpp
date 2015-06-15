/************************************************************
Copyright (C), 2015-2100, WaterPlus. Co., Ltd.
Author:	张万杰
Version : 1.02
Email: zhangwanjie@126.com
***********************************************************/
#include "stdafx.h"
#include "Kinect2.h"


CKinect2::CKinect2() :
m_pKinectSensor(NULL),
m_pCoordinateMapper(NULL),
m_pBodyFrameReader(NULL),
m_pColorFrameReader(NULL),
m_pDepthFrameReader(NULL)
{
	bToExit = false;
	m_bThreadAlive = false;
	pInfoList = NULL;
	pRobotCore = NULL;
	// create heap storage for color pixel data in RGBX format
	m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];
	m_pColorBuf = new unsigned char[cColorWidth * cColorHeight *3];
	colorSpacePoints = new ColorSpacePoint[cColorWidth * cColorHeight];
	cColorToDepthPoints = new DepthSpacePoint[cColorWidth * cColorHeight];

	depthPoints = new DepthSpacePoint[cDepthWidth*cDepthHeight];
	for (int y = 0; y < DEPTH_HEIGHT; y++)
	{
		for (int x = 0; x < DEPTH_WIDTH; x++)
		{
			depthPoints[y*DEPTH_WIDTH + x].X = x;
			depthPoints[y*DEPTH_WIDTH + x].Y = y;
		}
	}
	cameraPoints = new CameraSpacePoint[cDepthWidth*cDepthHeight];

}


CKinect2::~CKinect2()
{
	Stop();
	do
	{
		bToExit = true;
		Sleep(100);
	} while (m_bThreadAlive);

	if (m_pColorRGBX)
	{
		delete[] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}

	if (m_pColorBuf)
	{
		delete[] m_pColorBuf;
		m_pColorBuf = NULL;
	}

	if (colorSpacePoints)
	{
		delete[] colorSpacePoints;
		colorSpacePoints = NULL;
	}

	if (cColorToDepthPoints)
	{
		delete[] cColorToDepthPoints;
		cColorToDepthPoints = NULL;
	}

	if (depthPoints)
	{
		delete[] depthPoints;
		depthPoints = NULL;
	}
	if (cameraPoints)
	{
		delete[] cameraPoints;
		cameraPoints = NULL;
	}

	// done with coordinate mapper
	SafeRelease(m_pCoordinateMapper);

	// done with body frame reader
	SafeRelease(m_pBodyFrameReader);

	// close the Kinect Sensor
	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}

	SafeRelease(m_pKinectSensor);
}


HRESULT CKinect2::Start()
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get the color reader
		IColorFrameSource* pColorFrameSource = NULL;
		// Initialize the Kinect and get the body reader
		IBodyFrameSource* pBodyFrameSource = NULL;

		// Initialize the Kinect and get coordinate mapper
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
		}

		SafeRelease(pColorFrameSource);


		// Initialize the Kinect and get the depth reader
		IDepthFrameSource* pDepthFrameSource = NULL;
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		SafeRelease(pDepthFrameSource);

		// Initialize the Kinect and get the body reader
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
		}

		SafeRelease(pBodyFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		//SetStatusMessage(L"No ready Kinect found!", 10000, true);
		PrintInfo(L"[Kinect] No ready Kinect found!");
		return E_FAIL;
	}

	bToExit = false;
	AfxBeginThread(MainLoopThread, this);

	return NOERROR;
}


UINT CKinect2::MainLoopThread(LPVOID pParam)
{
	CKinect2* pKinect = (CKinect2*)pParam;

	pKinect->PrintInfo(L"[Kinect] MainLoopThread");

	if (!pKinect->m_pColorFrameReader)
	{
		return 0L;
	}
	IColorFrame* pColorFrame = NULL;

	if (!pKinect->m_pDepthFrameReader)
	{
		return 0L;
	}
	IDepthFrame* pDepthFrame = NULL;
	UINT16* pDepthBuf = NULL;

	pKinect->m_bThreadAlive = true;
	while (pKinect->bToExit == false)
	{
		//[1]深度图像
		HRESULT hr = pKinect->m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
		if (SUCCEEDED(hr))
		{
			//pKinect->PrintInfo(L"[Kinect] 深度Depth");
			INT64 nTime = 0;
			IFrameDescription* pFrameDescription = NULL;
			int nWidth = 0;
			int nHeight = 0;
			USHORT nDepthMinReliableDistance = 0;
			USHORT nDepthMaxDistance = 0;
			UINT nBufferSize = 0;

			hr = pDepthFrame->get_RelativeTime(&nTime);

			if (SUCCEEDED(hr))
			{
				hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
			}

			if (SUCCEEDED(hr))
			{
				hr = pFrameDescription->get_Width(&nWidth);
			}

			if (SUCCEEDED(hr))
			{
				hr = pFrameDescription->get_Height(&nHeight);
			}

			if (SUCCEEDED(hr))
			{
				hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
			}

			if (SUCCEEDED(hr))
			{
				// In order to see the full range of depth (including the less reliable far field depth)
				// we are setting nDepthMaxDistance to the extreme potential depth threshold
				nDepthMaxDistance = USHRT_MAX;

				// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
				//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
			}

			if (SUCCEEDED(hr))
			{
				hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pDepthBuf);
			}

			if (SUCCEEDED(hr))
			{
				pKinect->NewDepth(nTime, pDepthBuf, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
			}

			SafeRelease(pFrameDescription);
		}
		SafeRelease(pDepthFrame);

		//[2]坐标映射
		if (pDepthBuf != NULL)
		{
			HRESULT hr = pKinect->m_pCoordinateMapper->MapDepthFrameToColorSpace(DEPTH_WIDTH * DEPTH_HEIGHT, (UINT16*)pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->depthD16, cDepthWidth * cDepthHeight, pKinect->colorSpacePoints);
			if (SUCCEEDED(hr))
			{
				pKinect->pRobotCore->m_cDepthTo3D.csSrc3DObject.Lock();
				for (int i = 0; i < DEPTH_WIDTH * DEPTH_HEIGHT; i++)
				{
					pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->DepthToColorX[i] = pKinect->colorSpacePoints[i].X;
					pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->DepthToColorY[i] = pKinect->colorSpacePoints[i].Y;
				}
				pKinect->pRobotCore->m_cDepthTo3D.csSrc3DObject.Unlock();
			}

			hr = pKinect->m_pCoordinateMapper->MapColorFrameToDepthSpace(DEPTH_WIDTH * DEPTH_HEIGHT, (UINT16*)pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->depthD16, cColorWidth*cColorHeight, pKinect->cColorToDepthPoints);
			if (SUCCEEDED(hr))
			{
				pKinect->pRobotCore->m_cDepthTo3D.csSrc3DObject.Lock();
				for (int i = 0; i < cColorWidth * cColorHeight; i++)
				{
					pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->ColorToDepth[i].x = pKinect->cColorToDepthPoints[i].X;
					pKinect->pRobotCore->m_cDepthTo3D.pSrc3DObject->ColorToDepth[i].y = pKinect->cColorToDepthPoints[i].Y;
				}
				pKinect->pRobotCore->m_cDepthTo3D.csSrc3DObject.Unlock();
			}
		}

		//[3]彩色图像
		hr = pKinect->m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
		if (SUCCEEDED(hr))
		{
			//pKinect->PrintInfo(L"[Kinect] 彩色RGB");
			INT64 nTime = 0;
			IFrameDescription* pFrameDescription = NULL;
			int nWidth = 0;
			int nHeight = 0;
			ColorImageFormat imageFormat = ColorImageFormat_None;
			UINT nBufferSize = 0;
			RGBQUAD *pBuffer = NULL;

			hr = pColorFrame->get_RelativeTime(&nTime);

			if (SUCCEEDED(hr))
			{
				hr = pColorFrame->get_FrameDescription(&pFrameDescription);
			}
			if (SUCCEEDED(hr))
			{
				hr = pFrameDescription->get_Width(&nWidth);
			}
			if (SUCCEEDED(hr))
			{
				hr = pFrameDescription->get_Height(&nHeight);
			}
			if (SUCCEEDED(hr))
			{
				hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
			}
			if (SUCCEEDED(hr))
			{
				if (imageFormat == ColorImageFormat_Bgra)
				{
					hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
				}
				else if (pKinect->m_pColorRGBX)
				{
					pBuffer = pKinect->m_pColorRGBX;
					nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
					hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
				}
				else
				{
					hr = E_FAIL;
				}
			}
			if (SUCCEEDED(hr))
			{
				//获取彩色图像
				pKinect->NewColor(nTime, pBuffer, nWidth, nHeight);
			}

			SafeRelease(pFrameDescription);
		}

		SafeRelease(pColorFrame);

		//[4]骨骼坐标
		/*if (pKinect->m_pBodyFrameReader)
		{
			IBodyFrame* pBodyFrame = NULL;

			HRESULT hr = pKinect->m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);

			if (SUCCEEDED(hr))
			{
				INT64 nTime = 0;

				hr = pBodyFrame->get_RelativeTime(&nTime);

				IBody* ppBodies[BODY_COUNT] = { 0 };

				if (SUCCEEDED(hr))
				{
					hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
				}

				if (SUCCEEDED(hr))
				{
					pKinect->NewBody(nTime, BODY_COUNT, ppBodies);
				}

				for (int i = 0; i < _countof(ppBodies); ++i)
				{
					SafeRelease(ppBodies[i]);
				}
			}

			SafeRelease(pBodyFrame);
		}*/
	}

	pKinect->m_bThreadAlive = false;

	return 1L;
}

/// <summary>
/// Handle new depth data
/// <param name="nTime">timestamp of frame</param>
/// <param name="pBuffer">pointer to frame data</param>
/// <param name="nWidth">width (in pixels) of input image data</param>
/// <param name="nHeight">height (in pixels) of input image data</param>
/// <param name="nMinDepth">minimum reliable depth</param>
/// <param name="nMaxDepth">maximum reliable depth</param>
/// </summary>
void CKinect2::NewDepth(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth)
{
	m_pCoordinateMapper->MapDepthPointsToCameraSpace(cDepthWidth*cDepthHeight, depthPoints, cDepthWidth*cDepthHeight, pBuffer, cDepthWidth*cDepthHeight, cameraPoints);
	pRobotCore->m_cDepthTo3D.csSrc3DObject.Lock();
	for (int y = 0; y < DEPTH_HEIGHT; y++)
	{
		for (int x = 0; x < DEPTH_WIDTH; x++)
		{
			int tmpIndex = y*DEPTH_WIDTH + x;
			pRobotCore->m_cDepthTo3D.pSrc3DObject->x[x][y] = cameraPoints[tmpIndex].X;
			pRobotCore->m_cDepthTo3D.pSrc3DObject->y[x][y] = cameraPoints[tmpIndex].Z;
			pRobotCore->m_cDepthTo3D.pSrc3DObject->z[x][y] = cameraPoints[tmpIndex].Y;
		}
	}
	pRobotCore->m_cDepthTo3D.csSrc3DObject.Unlock();

	pRobotCore->ProcessDepth((USHORT*)pBuffer,nWidth,nHeight,nMinDepth,nMaxDepth);
}

void CKinect2::NewColor(INT64 nTime, RGBQUAD* pBuffer, int nWidth, int nHeight)
{
	if (pRobotCore != NULL)
	{
		//RGBA转换成RGB24
		UCHAR* pSrc = (UCHAR*)pBuffer;
		UCHAR* pDst = m_pColorBuf;
		for (int i = 0; i < 1920 * 1080;i++)
		{
		memcpy(pDst, pSrc, 3);
		pSrc += 4;
		pDst += 3;
		}
		pRobotCore->ProcessRGB(m_pColorBuf, nWidth, nHeight);
	}
}


/// <summary>
/// Handle new body data
/// <param name="nTime">timestamp of frame</param>
/// <param name="nBodyCount">body data count</param>
/// <param name="ppBodies">body data in frame</param>
/// </summary>
void CKinect2::NewBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{
	//HRESULT hr;
	//for (int i = 0; i < nBodyCount; ++i)
	//{
	//	IBody* pBody = ppBodies[i];
	//	if (pBody)
	//	{
	//		BOOLEAN bTracked = false;
	//		hr = pBody->get_IsTracked(&bTracked);

	//		pRobotCore->m_cDepthTo3D.csSrc3DObject.Lock();
	//		if (SUCCEEDED(hr) && bTracked)
	//		{
	//			//第i个骨架被识别出来了
	//			Joint joints[JointType_Count];
	//			//D2D1_POINT_2F jointPoints[JointType_Count];
	//			HandState leftHandState = HandState_Unknown;
	//			HandState rightHandState = HandState_Unknown;

	//			pBody->get_HandLeftState(&leftHandState);
	//			pBody->get_HandRightState(&rightHandState);

	//			hr = pBody->GetJoints(_countof(joints), joints);
	//			if (SUCCEEDED(hr))
	//			{
	//				//获取脸部坐标
	//				ColorSpacePoint tmpClrPoint = { 0 };
	//				m_pCoordinateMapper->MapCameraPointsToColorSpace(1, &(joints[3].Position), 1, &tmpClrPoint);
	//				pRobotCore->m_cDepthTo3D.pSrc3DObject->FaceInColorX[i] = tmpClrPoint.X;
	//				pRobotCore->m_cDepthTo3D.pSrc3DObject->FaceInColorY[i] = tmpClrPoint.Y;
	//				//获取关节点数据
	//				for (int j = 0; j < _countof(joints); ++j)
	//				{
	//					pRobotCore->m_cDepthTo3D.pSrc3DObject->arSkeleton[i][j].x = joints[j].Position.X;
	//					pRobotCore->m_cDepthTo3D.pSrc3DObject->arSkeleton[i][j].y = joints[j].Position.Z;
	//					pRobotCore->m_cDepthTo3D.pSrc3DObject->arSkeleton[i][j].z = joints[j].Position.Y;
	//					
	//				}

	//			}
	//			pRobotCore->m_cDepthTo3D.pSrc3DObject->bSkeleton[i] = true;
	//		}
	//		else
	//		{
	//			//第i个骨架消失了
	//			pRobotCore->m_cDepthTo3D.pSrc3DObject->bSkeleton[i] = false;
	//		}
	//		pRobotCore->m_cDepthTo3D.csSrc3DObject.Unlock();
	//	}
	//}

	////pRobotCore->m_bSkeletonUpdate = true;
	//pRobotCore->ProcessEvent();
}

void CKinect2::PrintInfo(CString inStr)
{
	if (pInfoList != NULL)
	{
		pInfoList->AddString(inStr);
		pInfoList->SetCurSel(pInfoList->GetCount() - 1);
	}
}


void CKinect2::Stop()
{
	bToExit = true;
	if (m_pKinectSensor != NULL)
	{
		m_pKinectSensor->Close();
	}
}
