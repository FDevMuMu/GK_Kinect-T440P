#pragma once

#include "Structs.h"

class CDataProcess
{
public:
	CDataProcess();
	~CDataProcess();

	CListBox* pInfoList;
	CEdit* pInfoDis;

	void ProcessSrc(stWP_K_3D_Object* in3DObj);
	void ProcessTransfom(stWP_K_3D_Object* in3DObj);
	void PrintList(CString inStr);
	void PrintDis(CString inStr);
	void DisplayPic(CStatic *inDis);
};

