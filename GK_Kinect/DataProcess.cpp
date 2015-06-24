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
#include <stdio.h>  
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
int pointx; //用于存储鼠标获得的(x,y)坐标
int pointy; //用于存储鼠标获得的(x,y)坐标
//const openni::DepthPixel* pDepth;
IplImage *showxy_msy;
IplImage *showxz_msy;
//////不要在这个cpp里修改//////#define showphoto_msy//测试时显示连通域的部分,定义表示关闭,注释掉表示关闭//开启时非常浪费时间,大约6ms处理一帧,关闭时只需0.5ms,定义在abc.h中 
//////不要在这个cpp里修改//////#define mousedebug //重复显示一帧图像, 通过鼠标获取所指连通域的三个筛选值(面积比 长宽比 距离与面积的乘积) 注释掉表示关闭 ,定义在abc.h中
//////不要在这个cpp里修改//////#define Screenball //是否增加筛选球的一部 注释掉表示关闭,筛选不要和鼠标同时开启,定义在abc.h中

//#define measuretime //测连通域时间,显示在下方文本中 注释掉表示关闭, 注意测时程序不能和鼠标程序同时开启

///////////////
////mouse click
#ifdef mousedebug
void on_mouse(int event, int x, int y, int flags, void* ustc)
{
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5, 0, 1, CV_AA);

//	if (event == CV_EVENT_LBUTTONDOWN)
	if (event == CV_EVENT_MOUSEMOVE)
	{
		//CvPoint pt = cvPoint(x, y);
		//char temp[100];
		//sprintf(temp, "(%d,%d)", pt.x, pt.y);
		/*cvPutText(pOut02, temp, pt, &font, cvScalar(255, 255, 255, 0));
		cvCircle(pOut02, pt, 2, cvScalar(255, 0, 0, 0), CV_FILLED, CV_AA, 0);*/
		pointx = x; pointy = y;
		//cvShowImage("win02", pOut02);
     }
}
#endif


static int drawtrack = 0;
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

	nTrack = 0;

	//////////////////////
	//arTrack[0].x = 0;//两坐标系里y,z意义不同
	//arTrack[0].y = 0;
	//arTrack[0].z = 0;
	//drawtrack = 1;
	//////////////////////
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



void CDataProcess::ProcessSrc(stWP_K_3D_Object* in3DObj)
{
	
}


static int n = 0;
static int initializephoto = 0;
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


	if (initializephoto == 0)//对图片的初始化只执行一次, 因为这个函数一帧一调用, 所以调用第一次后ainitializephoto设为1
	{
#ifdef showphoto_msy
		cvNamedWindow("win02", 0);
#endif
		CvSize size;
		size.height = 480;
		size.width = 640;
#ifdef showphoto_msy		
		pOut02 = cvCreateImage(size, 8, 3);
#endif
		//cvNamedWindow("pOut02", 0);//几个并没有使用的图
		/*pOut01 = cvCreateImage(size, 8, 3);
		pCannyImg1 = cvCreateImage(size, 8, 3);

		showxy_msy = cvCreateImage(size, 8, 3);
		showxz_msy = cvCreateImage(size, 8, 3);*/
		//cvNamedWindow("pOut01", 0);
#ifdef mousedebug
		cvSetMouseCallback("win02", on_mouse, NULL);
#endif
		initializephoto = 1;
	}
	//////////////////////////////////////////////
	//示例：文本输出到单行控件
	/*CString strTmp;
	int a = 1;
	strTmp.Format(L"正在处理原始数据 a= %d", a);
	PrintDis(strTmp);*/
	///////////////////////////////////////////////


#ifdef mousedebug
	while (1)
	{
#endif
		char key = 0;
		int i = 0, j = 0;
		long int temp = 0;
		for (i = 0; i < 480; i++)
		{
			for (j = 0; j < 640; j++)
			{
				if (i >= 0 && i < 424 && j >= 0 && j < 512)
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
#ifdef measuretime		
		//********************
		//********************测试时间的起始点
		LARGE_INTEGER Frequency, CountEnd, CountStart;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&CountStart);
		double dfElapseMS = 0;
		double dfElapseMS1 = 0;
		double dfElapseMS2 = 0;
		double dfElapseMS3 = 0;
#endif

		ballReturnValue ball;
		bool_max_connectivity_analyze2_1_OBJ(&ball);

#ifdef measuretime	
		//****************************************************************************************************
		QueryPerformanceCounter(&CountEnd);
		dfElapseMS = (double)((double)(CountEnd.QuadPart - CountStart.QuadPart + 10) / (double)Frequency.QuadPart)*1000.0;
		//****************************************************************************************************第一个时间节点
		//////////////////////////////////////////////测试连通域计算的时间并输出在下方文本框中
		CString strTmp;
		strTmp.Format(L"正在处理原始数据 time= %f", dfElapseMS);
		PrintDis(strTmp);
		///////////////////////////////////////////////
#endif

#ifdef showphoto_msy
		cvShowImage("win02", pOut02);
		cvWaitKey(10);
#ifdef mousedebug
		if (cvWaitKey(20) == 'N')
			return;
#endif


#endif
#ifdef mousedebug
	}
#endif
    ////*******************画轨迹
	if (ball.x > 0 && ball.y > 0)
	{
	
	//arTrack[drawtrack].x = ball.x;//两坐标系里y,z意义不同
	//arTrack[drawtrack].y = ball.z;
	//arTrack[drawtrack].z = ball.y;
	int nBallx = ball.x;
	int nBally = ball.y;
	arTrack[drawtrack].x = in3DObj->x[nBallx][nBally];//两坐标系里y,z意义不同
	arTrack[drawtrack].y = in3DObj->y[nBallx][nBally];
	arTrack[drawtrack].z = in3DObj->z[nBallx][nBally];

	CString strInfo;
	strInfo.Format(L"arTrack[%d] = (%.2f , %.2f , %.2f)", drawtrack, arTrack[drawtrack].x, arTrack[drawtrack].y, arTrack[drawtrack].z);
	PrintList(strInfo);

	drawtrack++;
	nTrack = drawtrack;
	}
	else
	{

		
		PrintList(L"看不到球");
		
	}




	//********************draw point in black picture
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
