#pragma once

#include "Structs.h"

class CDepthTo3D
{
public:
	CDepthTo3D(void);
	~CDepthTo3D(void);

	CListBox* pInfoList;

	///////////////////////////////////////
	//3D
	bool b3DObjProcessing;
	CCriticalSection csSrc3DObject;

	WP_K_3D_Object* pSrc3DObject;
	WP_K_3D_Object* pTransform3DObject;
	WP_K_3D_Object* pDisplay3DObject;

	//相机高度和角度
	double m_fCameraAboveGround;
	double m_fCameraPitch;
	////////////////////////////////////////////

	void DepthTo3D(trPoint* inPoint);
	void Build3DObject(void);
	BYTE* m_rgb24;
	int m_nDepthWidth;
	int m_nDepthHeight;

	void SrcToDest(void);

	int GetColorXFromDepth(int inDepthX, int inDepthY);
	int GetColorYFromDepth(int inDepthX, int inDepthY);
	void Algo_DepthSmooth(void);
	void PrintInfo(CString inStr);
};

