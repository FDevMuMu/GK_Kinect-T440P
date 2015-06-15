#include "stdafx.h"
#include "KinectDataStream.h"


CKinectDataStream::CKinectDataStream()
{
	m_ar3Dobj.resize(100);
	pInfoList = NULL;
}


CKinectDataStream::~CKinectDataStream()
{

}


void CKinectDataStream::BufOneFrame(stWP_K_3D_Object* in3DObj)
{
	m_ar3Dobj.push_back(*in3DObj);
}


void CKinectDataStream::SaveToBinFile(CString inFileName)
{
	if (m_ar3Dobj.size() > 0)
	{
		CFile fileWrite;		//记录文件
		BOOL fo_res;			//记录文件打开是否成功

		//打开文件
		fo_res = fileWrite.Open(inFileName, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite);
		if (fo_res == TRUE)
		{
			//开始写入数据
			int nNum = m_ar3Dobj.size();

			//依次写入模型数据
			for (int i = 0; i < nNum; i++)
			{
				fileWrite.Write(&(m_ar3Dobj[i]), sizeof(stWP_K_3D_Object));
			}

			fileWrite.Close();

			CString strTmp;
			strTmp.Format(L"保存了%d帧数据到%s", nNum, inFileName);
			PrintInfo(strTmp);
		}
	}
}


void CKinectDataStream::PrintInfo(CString inStr)
{
	if (pInfoList != NULL)
	{
		pInfoList->AddString(inStr);
		pInfoList->SetCurSel(pInfoList->GetCount() - 1);
	}
}


void CKinectDataStream::SaveToDefault()
{
	CString m_strFile;
	CTime tm;
	tm = CTime::GetCurrentTime();
	m_strFile.Format(L"%d-%d-%d_%d-%d-%d.k2", tm.GetYear(), tm.GetMonth(), tm.GetDay(), tm.GetHour(), tm.GetMinute(), tm.GetSecond());
	m_strFile = L"../../_KinectStream/" + m_strFile;

	SaveToBinFile(m_strFile);
}

static WP_K_3D_Object tmpObj;
void CKinectDataStream::LoadFromBinFile(CString inFileName)
{
	PrintInfo(inFileName);
	CFile fileRead;		//记录文件
	BOOL fo_res;			//记录文件打开是否成功

	//打开文件
	fo_res = fileRead.Open(inFileName, CFile::modeRead);
	if (fo_res == TRUE)
	{
		m_ar3Dobj.clear();

		//开始读取数据
		ULONGLONG nFileSize = fileRead.GetLength();
		int nNum = nFileSize / sizeof(tmpObj);

		CString str;
		str.Format(L"读取%d组数据(%s)", nNum, inFileName);
		PrintInfo(str);

		//循环读取
		for (int i = 0; i < nNum; i++)
		{
			fileRead.Read(&tmpObj, sizeof(WP_K_3D_Object));
			m_ar3Dobj.push_back(tmpObj);
		}

		fileRead.Close();
	}
}
