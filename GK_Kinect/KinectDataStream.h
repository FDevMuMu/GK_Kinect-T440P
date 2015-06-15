#pragma once

#include "Structs.h"


class CKinectDataStream
{
public:
	CKinectDataStream();
	~CKinectDataStream();

	CListBox* pInfoList;

	vector<stWP_K_3D_Object> m_ar3Dobj;
	void BufOneFrame(stWP_K_3D_Object* in3DObj);
	void SaveToBinFile(CString inFileName);
	void PrintInfo(CString inStr);
	void SaveToDefault();
	void LoadFromBinFile(CString inFileName);
};

