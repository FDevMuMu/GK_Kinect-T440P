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

static unsigned char dataShow[320*240*3];	//�������Ͻ�������ʾ������
static BITMAPINFOHEADER bih;
////variable from abc.h
unsigned int DepthBuf_O_T[DEPTH_VISION_CAM_HEIGHT][DEPTH_VISION_CAM_WIDTH];
unsigned int  area_grow_data_obj[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];
int connect_area_s_e_w[100000][3];//[100000][3];//[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];//�洢1000����ͨ��� �� β ��� [��]connect_area_s_e[n][0] [β]connect_area_s_e[n][1] [����]connect_area_s_e[n][2]
int Num_track_L;//�洢��ǰ��������ͨ��ĸ���
int Z_min_diff_set = 100;	  //��ͨ��������Ȳ�����Զ�Ķ�����Ϊ�ǲ�ͬ�����壩							
IplImage *pOut01;
IplImage *pOut02;

int DepthBuf_O[480][640];//ԭ���ͼ
int DepthBuf_O_msy[480][640];//����������ͼ
IplImage *pCannyImg1;
unsigned short *tempp;
long int xx = 0;
long int yy = 0;
long int msy = 0;
long int center_msy;//abc.h�з��ص�center,��ʾ��x,y)�ǵ��Ӧ�ھ������λ��
int pointx; //���ڴ洢����õ�(x,y)����
int pointy; //���ڴ洢����õ�(x,y)����
//const openni::DepthPixel* pDepth;
IplImage *showxy_msy;
IplImage *showxz_msy;
//////��Ҫ�����cpp���޸�//////#define showphoto_msy//����ʱ��ʾ��ͨ��Ĳ���,�����ʾ�ر�,ע�͵���ʾ�ر�//����ʱ�ǳ��˷�ʱ��,��Լ6ms����һ֡,�ر�ʱֻ��0.5ms,������abc.h�� 
//////��Ҫ�����cpp���޸�//////#define mousedebug //�ظ���ʾһ֡ͼ��, ͨ������ȡ��ָ��ͨ�������ɸѡֵ(����� ����� ����������ĳ˻�) ע�͵���ʾ�ر� ,������abc.h��
//////��Ҫ�����cpp���޸�//////#define Screenball //�Ƿ�����ɸѡ���һ�� ע�͵���ʾ�ر�,ɸѡ��Ҫ�����ͬʱ����,������abc.h��

//#define measuretime //����ͨ��ʱ��,��ʾ���·��ı��� ע�͵���ʾ�ر�, ע���ʱ�����ܺ�������ͬʱ����

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
	//arTrack[0].x = 0;//������ϵ��y,z���岻ͬ
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

//��ʾ����dataShow
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
unsigned char srcRGB[COLOR_WIDTH*COLOR_HEIGHT * 3];  //RGB��ɫ���ݣ�1092*1080��
unsigned short depthD16[DEPTH_WIDTH*DEPTH_HEIGHT];	//���ֵ���ݣ�512*424��
int DepthToColorX[DEPTH_WIDTH*DEPTH_HEIGHT];		//�����������ӳ�䵽RGB��ɫ�����������Xֵ
int DepthToColorY[DEPTH_WIDTH*DEPTH_HEIGHT];		//�����������ӳ�䵽RGB��ɫ�����������Yֵ
tdPoint ColorToDepth[COLOR_WIDTH*COLOR_HEIGHT];		//RGB��ɫ���ݶ�Ӧ�������������

bool en[DEPTH_WIDTH][DEPTH_HEIGHT];			//������ά�������Ч��ǣ����Ϊtrueʱ��ʾ����ά�������Ч�����һ���OpenGL����ʾ
float x[DEPTH_WIDTH][DEPTH_HEIGHT];			//���ƶ�Ӧ����ά�����xֵ
float y[DEPTH_WIDTH][DEPTH_HEIGHT];			//���ƶ�Ӧ����ά�����yֵ
float z[DEPTH_WIDTH][DEPTH_HEIGHT];			//���ƶ�Ӧ����ά�����zֵ
unsigned char R[DEPTH_WIDTH][DEPTH_HEIGHT];	//��ά������Ӧ�Ĳ�ɫ���ݵĺ�ɫ����
unsigned char G[DEPTH_WIDTH][DEPTH_HEIGHT];	//��ά������Ӧ�Ĳ�ɫ���ݵ���ɫ����
unsigned char B[DEPTH_WIDTH][DEPTH_HEIGHT];	//��ά������Ӧ�Ĳ�ɫ���ݵ���ɫ����
}WP_K_3D_Object;
*/

/************************************************************************/
/*����δ����ת�任������                                                */
/************************************************************************/



void CDataProcess::ProcessSrc(stWP_K_3D_Object* in3DObj)
{
	
}


static int n = 0;
static int initializephoto = 0;
/************************************************************************/
/*�����Ѿ���ת�任������                                                */
/************************************************************************/
void CDataProcess::ProcessTransfom(stWP_K_3D_Object* in3DObj)
{

	//////////////////////////////////////////////
	//ʾ�����ı�����������б�
	/*CString strTmp;
	strTmp.Format(L"���ڴ���任���� n= %d", n);
	PrintList(strTmp);*/

	//////////////////////////////////
	//ʾ���������������ϽǺ�ɫ�������ʾ
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
	//ʾ������������3D��ʾ

	//��һЩ�е���ά���ɫ
	/*for (int y = 10; y < 150;y++)
	{
	for (int x = 0; x < 512;x++)
	{
	in3DObj->R[x][y] = 0;
	in3DObj->G[x][y] = 0xff;
	in3DObj->B[x][y] = 0;
	}
	}*/

	//��һЩ�е���ά�㲻��ʾ
	/*for (int y = 0; y < 424; y++)
	{
	for (int x = 100; x < 200; x++)
	{
	in3DObj->en[x][y] = false;
	}
	}*/
	/////////////////////////////////


	if (initializephoto == 0)//��ͼƬ�ĳ�ʼ��ִֻ��һ��, ��Ϊ�������һ֡һ����, ���Ե��õ�һ�κ�ainitializephoto��Ϊ1
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
		//cvNamedWindow("pOut02", 0);//������û��ʹ�õ�ͼ
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
	//ʾ�����ı���������пؼ�
	/*CString strTmp;
	int a = 1;
	strTmp.Format(L"���ڴ���ԭʼ���� a= %d", a);
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
					DepthBuf_O[i][j] = in3DObj->depthD16[temp];//5 >> 3;//ԭʼ��ȴ���DepthBuf_O������
					DepthBuf_O_msy[i][j] = in3DObj->depthD16[temp];//5 >> 3;//ԭʼ��ȴ���DepthBuf_O������
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
		//********************����ʱ�����ʼ��
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
		//****************************************************************************************************��һ��ʱ��ڵ�
		//////////////////////////////////////////////������ͨ������ʱ�䲢������·��ı�����
		CString strTmp;
		strTmp.Format(L"���ڴ���ԭʼ���� time= %f", dfElapseMS);
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
    ////*******************���켣
	if (ball.x > 0 && ball.y > 0)
	{
	
	//arTrack[drawtrack].x = ball.x;//������ϵ��y,z���岻ͬ
	//arTrack[drawtrack].y = ball.z;
	//arTrack[drawtrack].z = ball.y;
	int nBallx = ball.x;
	int nBally = ball.y;
	arTrack[drawtrack].x = in3DObj->x[nBallx][nBally];//������ϵ��y,z���岻ͬ
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

		
		PrintList(L"��������");
		
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
