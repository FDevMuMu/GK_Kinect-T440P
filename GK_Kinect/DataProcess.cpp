#include "stdafx.h"
#include "DataProcess.h"
#include <stdlib.h>
#include <iostream>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv/cv.h"  
//#include "opencv/highgui.h"  
#include "CamConstantSet.h"
#include "abc.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
using namespace std;
using namespace cv;

static unsigned char dataShow[320*240*3];	//界面右上角用来显示的数组
static BITMAPINFOHEADER bih;
////variable from abc.h
unsigned int DepthBuf_O_T[DEPTH_VISION_CAM_HEIGHT][DEPTH_VISION_CAM_WIDTH];
unsigned int  area_grow_data_obj[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];
int connect_area_s_e_w[100000][3];//[100000][3];//[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];//存储1000个连通域的 首 尾 标号 [首]connect_area_s_e[n][0] [尾]connect_area_s_e[n][1] [重量]connect_area_s_e[n][2]
int Num_track_L;//存储当前被跟踪连通体的个数
int Z_min_diff_set = 100;	  //联通域链接深度差（距离多远的东西认为是不同的物体）							
IplImage *pOut01;
IplImage *pOut02;

int DepthBuf_O[480][640];//原深度图
int DepthBuf_O_msy[480][640];//测参数用深度图
IplImage *pCannyImg1;
unsigned short *tempp;
long int xx = 0;
long int yy = 0;
long int msy = 0;
long int center_msy;//abc.h中返回的center,表示（x,y)那点对应在矩阵里的位置
//const openni::DepthPixel* pDepth;
IplImage *showxy_msy;
IplImage *showxz_msy;

///////////////


CDataProcess::CDataProcess()
{
	pInfoList = NULL;
	pInfoDis = NULL;

	memset(dataShow, 0, 320 * 240 * 3);
	memset(&bih, 0, sizeof(bih));
	bih.biSize = sizeof(bih);
	bih.biWidth = 320;
	bih.biHeight = 240;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
}


CDataProcess::~CDataProcess()
{
}


void CDataProcess::PrintList(CString inStr)
{
	if (pInfoList != NULL)
	{
		pInfoList->AddString(inStr);
		pInfoList->SetCurSel(pInfoList->GetCount() - 1);
	}
}


void CDataProcess::PrintDis(CString inStr)
{
	if (pInfoDis != NULL)
	{
		pInfoDis->SetWindowTextW(inStr);
	}
}

//显示数组dataShow
void CDataProcess::DisplayPic(CStatic *inDis)
{
	HWND hwnd = inDis->GetSafeHwnd();
	RECT rc;
	::GetWindowRect(hwnd, &rc);
	long lWidth = rc.right - rc.left;
	long lHeight = rc.bottom - rc.top;

	HDC hdc = ::GetDC(hwnd);
	PAINTSTRUCT ps;
	::BeginPaint(hwnd, &ps);

	SetStretchBltMode(hdc, COLORONCOLOR);
	StretchDIBits(
		hdc, 0, 0,
		lWidth, lHeight,
		0, 0, 320, 240,
		dataShow,
		(BITMAPINFO*)&bih,
		DIB_RGB_COLORS,
		SRCCOPY);

	::EndPaint(hwnd, &ps);
	::ReleaseDC(hwnd, hdc);
}

/*
typedef struct stWP_K_3D_Object
{
unsigned char srcRGB[COLOR_WIDTH*COLOR_HEIGHT * 3];  //RGB彩色数据（1092*1080）
unsigned short depthD16[DEPTH_WIDTH*DEPTH_HEIGHT];	//深度值数据（512*424）
int DepthToColorX[DEPTH_WIDTH*DEPTH_HEIGHT];		//深度数据坐标映射到RGB彩色数据里的坐标X值
int DepthToColorY[DEPTH_WIDTH*DEPTH_HEIGHT];		//深度数据坐标映射到RGB彩色数据里的坐标Y值
tdPoint ColorToDepth[COLOR_WIDTH*COLOR_HEIGHT];		//RGB彩色数据对应的深度数据坐标

bool en[DEPTH_WIDTH][DEPTH_HEIGHT];			//点云三维坐标点有效标记，标记为true时表示该三维坐标点有效，而且会在OpenGL里显示
float x[DEPTH_WIDTH][DEPTH_HEIGHT];			//点云对应的三维坐标点x值
float y[DEPTH_WIDTH][DEPTH_HEIGHT];			//点云对应的三维坐标点y值
float z[DEPTH_WIDTH][DEPTH_HEIGHT];			//点云对应的三维坐标点z值
unsigned char R[DEPTH_WIDTH][DEPTH_HEIGHT];	//三维坐标点对应的彩色数据的红色分量
unsigned char G[DEPTH_WIDTH][DEPTH_HEIGHT];	//三维坐标点对应的彩色数据的绿色分量
unsigned char B[DEPTH_WIDTH][DEPTH_HEIGHT];	//三维坐标点对应的彩色数据的蓝色分量
}WP_K_3D_Object;
*/

/************************************************************************/
/*处理未曾旋转变换的数据                                                */
/************************************************************************/
static int a = 0;


void CDataProcess::ProcessSrc(stWP_K_3D_Object* in3DObj)
{
	if (a == 0)
	{
		//cvNamedWindow("win02", 0);
		CvSize size; //=  cvGetSize(BGR);//得到摄像头图像大小
		size.height = 480;
		size.width = 640;
		
		pOut02 = cvCreateImage(size, 8, 3);
		//cvNamedWindow("pOut02", 0);
      pOut01 = cvCreateImage(size, 8, 3);
		pCannyImg1 = cvCreateImage(size, 8, 3);

		showxy_msy = cvCreateImage(size, 8, 3);
		showxz_msy = cvCreateImage(size, 8, 3);
		//cvNamedWindow("pOut01", 0);
		
		a = 1;
	}
	//////////////////////////////////////////////
	//示例：文本输出到单行控件
	/*CString strTmp;
	int a = 1;
	strTmp.Format(L"正在处理原始数据 a= %d", a);
	PrintDis(strTmp);*/
	///////////////////////////////////////////////
	///////creat windows
	

	    char key = 0;
	    int i = 0, j = 0;
		long int temp = 0;
		//tempp = (unsigned short*)in3DObj->depthD16[DEPTH_WIDTH*DEPTH_HEIGHT];

		for (i = 0; i < 480; i++)
		{
			for (j = 0; j<640; j++)
			{
				if (i>=0 && i<424 && j>=0 && j < 512)
				{
					//temp = 512 * i + j;
					DepthBuf_O[i][j] = in3DObj->depthD16[temp];//5 >> 3;//原始深度存入DepthBuf_O数组中
					DepthBuf_O_msy[i][j] = in3DObj->depthD16[temp];//5 >> 3;//原始深度存入DepthBuf_O数组中
					temp = temp + 1;
				}

				else
				{
					DepthBuf_O[i][j] = 0;
					DepthBuf_O_msy[i][j] = 0;
				}

			}
		}
		
		//SYSTEMTIME sys1; GetLocalTime(&sys1);
		//********************
		//********************测试时间的起始点
		LARGE_INTEGER Frequency, CountEnd, CountStart;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&CountStart);
		double dfElapseMS = 0;
		double dfElapseMS1 = 0;
		double dfElapseMS2 = 0;
		double dfElapseMS3 = 0;






		bool_max_connectivity_analyze2_1_OBJ();

		//****************************************************************************************************
		QueryPerformanceCounter(&CountEnd);
		dfElapseMS = (double)((double)(CountEnd.QuadPart - CountStart.QuadPart + 10) / (double)Frequency.QuadPart)*1000.0;
		//****************************************************************************************************第一个时间节点





       // SYSTEMTIME sys2; GetLocalTime(&sys2);
		
		CString s1;
	
		s1.Format(_T("%f"), dfElapseMS);
		AfxMessageBox(s1);
		
		//cvShowImage("win02", pOut02);
		//cvWaitKey(10);

	/*	int x, y;
	
		for (x = 0; x < 320 ; x++)
		{
			for (y = 0; y < 240 ; y++)
			{
				dataShow[(y * 320 + x) * 3] = 0xff;
				dataShow[(y * 320 + x) * 3 + 1] = 0xff;
				dataShow[(y * 320 + x) * 3 + 2] = 0xff;
			}
		}*/

	}


static int n = 0;

/************************************************************************/
/*处理已经旋转变换的数据                                                */
/************************************************************************/
void CDataProcess::ProcessTransfom(stWP_K_3D_Object* in3DObj)
{   
	
	//////////////////////////////////////////////
	//示例：文本输出到多行列表
	/*CString strTmp;
	strTmp.Format(L"正在处理变换数据 n= %d", n);
	PrintList(strTmp);*/

	//////////////////////////////////
	//示例：操作界面右上角黑色区域的显示
	//if (n>=240)
	//{
	//n = 0;
	//}
	//for (int i = 0; i < 320 * 3 ;i++)
	//{
	//if (i%3 == 2)
	//{
	//dataShow[n * 320 * 3*2 + i] = 0xff;
	//}
	//}
	//n++;
	//////////////////////////////////
	//示例：操作最后的3D显示

	//让一些行的三维点变色
	/*for (int y = 10; y < 150;y++)
	{
	for (int x = 0; x < 512;x++)
	{
	in3DObj->R[x][y] = 0;
	in3DObj->G[x][y] = 0xff;
	in3DObj->B[x][y] = 0;
	}
	}*/

	//让一些列的三维点不显示
	/*for (int y = 0; y < 424; y++)
	{
	for (int x = 100; x < 200; x++)
	{
	in3DObj->en[x][y] = false;
	}
	}*/
	/////////////////////////////////




}
