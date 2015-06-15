#include "StdAfx.h"
#include "C3DObjFile.h"

#define WP_K_3DFILE_HEADER		0x5A
#define WP_K_DEPTHFILE_HEADER	0x5B

typedef struct stWP3dFileHeader
{
	UINT header;
	int num;
}WP3dFileHeader;

C3DObjFile::C3DObjFile(void)
{

}


C3DObjFile::~C3DObjFile(void)
{
}


void C3DObjFile::Save3DObjFile(CString inFileName, WP_K_3D_Object* inp3DObj,int inNum)
{
	if (inp3DObj != NULL)
	{
		CFile fileWrite;		//记录文件
		BOOL fo_res;			//记录文件打开是否成功

		//打开文件
		fo_res = fileWrite.Open(inFileName,CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
		if (fo_res == TRUE)
		{
			//开始写入数据
			WP3dFileHeader fileHeader;
			fileHeader.header = WP_K_3DFILE_HEADER;
			fileHeader.num = inNum;
			//写入文件头
			fileWrite.Write(&fileHeader,sizeof(fileHeader));

			//依次写入模型数据
			for (int i=0;i<inNum;i++)
			{
				fileWrite.Write(&(inp3DObj[i]),sizeof(WP_K_3D_Object));
			}

			fileWrite.Close();
		} 
	}
}


int C3DObjFile::Load3DObjFile(CString inFileName, WP_K_3D_Object* inArObjects, int inMax)
{
	int num =0;
	if (inArObjects != NULL)
	{
		CFile fileRead;		//记录文件
		BOOL fo_res;			//记录文件打开是否成功

		//打开文件
		fo_res = fileRead.Open(inFileName,CFile::modeRead);
		if (fo_res == TRUE)
		{
			//开始读取数据
			WP3dFileHeader fileHeader;
			
			fileRead.Read(&fileHeader,sizeof(fileHeader));

			if (fileHeader.header != WP_K_3DFILE_HEADER)
			{
				AfxMessageBox(L"读入3D文件失败：文件头信息不正确！");
				fileRead.Close();
				return 0;
			}
			if (inMax >= fileHeader.num)
			{
				num = fileHeader.num;
			} 
			else
			{
				num = inMax;
			}

			//一次性读出数据
			fileRead.Read(inArObjects,num*sizeof(WP_K_3D_Object));

			fileRead.Close();
		} 
	}
	return num;
}


