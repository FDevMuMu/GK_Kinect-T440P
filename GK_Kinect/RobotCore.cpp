#include "stdafx.h"
#include "RobotCore.h"

CRobotCore::CRobotCore()
{

	m_bThreadAlive = FALSE;
	m_hToProcessEvent = NULL;
	m_hShutdownEvent = NULL;

	nAutoTask = RTASK_NONE;

	nNumToSave = 100;
	bLogFrames = false;

	AfxBeginThread(PointCloudsThread, this);
}


CRobotCore::~CRobotCore()
{
	do
	{
		SetEvent(m_hShutdownEvent);
	} while (m_bThreadAlive);

	// Close Handles  
	if (m_hShutdownEvent != NULL)
		CloseHandle(m_hShutdownEvent);
	if (m_hToProcessEvent != NULL)
		CloseHandle(m_hToProcessEvent);
}

void CRobotCore::Procedure()
{
	TackAction();
}


static CString strTmp;
static int nLastAutoTask = RTASK_NONE;
void CRobotCore::TackAction()
{
	
}


void CRobotCore::ProcessDepth(USHORT* inBuf, int inWidth, int inHeight, USHORT nMinDepth, USHORT nMaxDepth)
{
	if (m_cDepthTo3D.b3DObjProcessing == true)
	{
		return;
	}
	m_cDepthTo3D.csSrc3DObject.Lock();
	//数据缓存
	memcpy(m_cDepthTo3D.pSrc3DObject->depthD16, inBuf, inWidth*inHeight * sizeof(USHORT));

	for (int y = 0; y < DEPTH_HEIGHT; y++)
	{
		for (int x = 0; x < DEPTH_WIDTH; x++)
		{
			if (inBuf[y*DEPTH_WIDTH + x] > nMinDepth && inBuf[y*DEPTH_WIDTH + x] < nMaxDepth)
			{
				m_cDepthTo3D.pSrc3DObject->en[x][y] = true;
			}
			else
			{
				m_cDepthTo3D.pSrc3DObject->en[x][y] = false;
			}

		}
	}
	m_cDepthTo3D.csSrc3DObject.Unlock();

	//激活一次3d点云处理
	SetEvent(m_hToProcessEvent);
}

void CRobotCore::ProcessRGB(UCHAR* inBuf, int inWidth, int inHeight)
{
	if (m_cDepthTo3D.b3DObjProcessing == true)
	{
		return;
	}
	m_cDepthTo3D.csSrc3DObject.Lock();

	//数据缓存
	memcpy(m_cDepthTo3D.pSrc3DObject->srcRGB, inBuf, inWidth*inHeight * 3);

	//临时显示
	//m_cFaceDetect.ShowFaceInSk(inBuf, inWidth, inHeight);
	//人脸检测
	//if (bFaceDetect == true)
	//{
	//	m_cFaceDetect.InRGB(inBuf, inWidth, inHeight);
	//	//memcpy(pGL->m_posFace3D, pShare->posFace3D, pShare->nFaceNum*sizeof(trPoint));
	//	//pGL->m_nFaceNum = pShare->nFaceNum;
	//}
	//else
	//{
	//	//pGL->m_nFaceNum = 0;
	//	m_cDataHolder.pSrc3DObject->nFaceNum = 0;
	//}

	m_cDepthTo3D.csSrc3DObject.Unlock();
}

UINT CRobotCore::PointCloudsThread(LPVOID pParam)
{
	CRobotCore* pRobotCore = (CRobotCore*)pParam;

	if (pRobotCore->m_hShutdownEvent != NULL)
		ResetEvent(pRobotCore->m_hShutdownEvent);
	else
		pRobotCore->m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (pRobotCore->m_hToProcessEvent != NULL)
		ResetEvent(pRobotCore->m_hToProcessEvent);
	else
		pRobotCore->m_hToProcessEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// initialize the event objects
	pRobotCore->m_hEventArray[0] = pRobotCore->m_hShutdownEvent;	// highest priority
	pRobotCore->m_hEventArray[1] = pRobotCore->m_hToProcessEvent;

	DWORD Event = 0;
	pRobotCore->m_bThreadAlive = TRUE;

	while (true)
	{
		Event = WaitForMultipleObjects(2, pRobotCore->m_hEventArray, FALSE, INFINITE);
		switch (Event)
		{
		case 0:
		{
			// Shutdown event.  This is event zero so it will be
			// the higest priority and be serviced first.
			pRobotCore->m_bThreadAlive = FALSE;

			// Kill this thread.  break is not needed, but makes me feel better.
			AfxEndThread(100);
			break;
		}
		case 1:	// ToProcess event
		{
			pRobotCore->m_ProcessOneFrame();

			//pRobotCore->m_cDataHolder.PrintInfo(L"[图像处理]完成一次3D处理");
			ResetEvent(pRobotCore->m_hToProcessEvent);
			break;
		}

		} // end switch
	}
	return 1L;
}


void CRobotCore::m_ProcessOneFrame()
{

	//拷贝轨迹数据
	memcpy(pGL->arTrack, m_DataProcess.arTrack, 100 * sizeof(st_trPoint));
	pGL->nTrack = m_DataProcess.nTrack;

	m_cDepthTo3D.b3DObjProcessing = true;
	m_cDepthTo3D.csSrc3DObject.Lock();
	
	//处理原始数据
	m_DataProcess.ProcessSrc(m_cDepthTo3D.pSrc3DObject);

	//从深度图构建3D模型
	m_cDepthTo3D.Build3DObject();

	//处理转换后的数据
	m_DataProcess.ProcessTransfom(m_cDepthTo3D.pDisplay3DObject);

	//数据记录
	if (bLogFrames == true)
	{
		m_cKinectDataStream.BufOneFrame(m_cDepthTo3D.pSrc3DObject);

		if (m_cKinectDataStream.m_ar3Dobj.size() > nNumToSave)
		{
			m_cKinectDataStream.SaveToDefault();
			bLogFrames = false;
		}
	}

	//解除锁定，等待下一帧图像填充进来
	m_cDepthTo3D.csSrc3DObject.Unlock();
	m_cDepthTo3D.b3DObjProcessing = false;
	pGL->Invalidate(FALSE);

	//显示数据
	m_DataProcess.DisplayPic(pDisShow);
}


void CRobotCore::ProcessEvent()
{
	//激活一次3d点云处理
	SetEvent(m_hToProcessEvent);
}


void CRobotCore::StartLogKinectStream(int inFrameNum)
{
	nNumToSave = inFrameNum;
	m_cKinectDataStream.m_ar3Dobj.clear();
	bLogFrames = true;
}
