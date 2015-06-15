#include "StdAfx.h"
#include "DepthTo3D.h"
#include <math.h>

static double xRes = (70.0f*3.14)/(512*180);
static double zRes = (60.0f*3.14)/(424*180);

CDepthTo3D::CDepthTo3D(void)
{
	pInfoList = NULL;
	m_nDepthWidth = DEPTH_WIDTH;
	m_nDepthHeight = DEPTH_HEIGHT;

	////////////////////////
	//3D
	pSrc3DObject = new WP_K_3D_Object;
	memset(pSrc3DObject, 0, sizeof(WP_K_3D_Object));
	pTransform3DObject = new WP_K_3D_Object;
	memset(pTransform3DObject, 0, sizeof(WP_K_3D_Object));
	pDisplay3DObject = new WP_K_3D_Object;
	memset(pDisplay3DObject, 0, sizeof(WP_K_3D_Object));

	b3DObjProcessing = false;

	//相机高度和角度
	m_fCameraAboveGround = 0.3;
	m_fCameraPitch = 15;
}


CDepthTo3D::~CDepthTo3D(void)
{
	////////////////////////
	//3D
	delete pSrc3DObject;
	delete pTransform3DObject;
	delete pDisplay3DObject;
	/////////////////////////
}

//深度图到3维点的换算
void CDepthTo3D::DepthTo3D(trPoint* inPoint)
{
	//计算x方向的偏差角度
	float dxAngle = inPoint->x*xRes;
	inPoint->x = inPoint->y * sin(dxAngle) ;

	//计算z轴
	float dzAngle = inPoint->z*zRes;
	inPoint->z = inPoint->y * sin(dzAngle);

	//计算y轴
	inPoint->y = inPoint->y*0.5;

}


void CDepthTo3D::Build3DObject(void)
{
	memcpy(pTransform3DObject, pSrc3DObject, sizeof(WP_K_3D_Object));
	BYTE* pRGBPixel;
	BYTE tfR,tfG,tfB;
	trPoint tdPoint;
	int nDepthIndex = 0;

	for (int y=0;y<DEPTH_HEIGHT;y++)
	{
		for (int x=0;x<DEPTH_WIDTH;x++)
		{
			nDepthIndex = y*DEPTH_WIDTH + x;
			if (pTransform3DObject->en[x][y] == true)
			{
				//pSrc3DObj->en[x][y] = true;		//使能该点

				int DepthToColorX = pTransform3DObject->DepthToColorX[nDepthIndex];
				int DepthToColorY = pTransform3DObject->DepthToColorY[nDepthIndex];
				if (DepthToColorX >= 0 && DepthToColorX < COLOR_WIDTH && DepthToColorY >=0 && DepthToColorY < COLOR_HEIGHT)
				{
					pRGBPixel = &(pTransform3DObject->srcRGB[(DepthToColorY*COLOR_WIDTH + DepthToColorX) * 3]);
					tfB = (*pRGBPixel);
					tfG = (*(pRGBPixel + 1));
					tfR = (*(pRGBPixel + 2));
				} 
				else
				{
					tfB = 0xff;
					tfG = 0xff;
					tfR = 0xff;
				}

				//存储颜色
				pTransform3DObject->R[x][y] = tfR;
				pTransform3DObject->G[x][y] = tfG;
				pTransform3DObject->B[x][y] = tfB;
				//glColor3f(tfR,tfG,tfB);


				/*tdPoint.x = x - m_nDepthWidth/2;
				tdPoint.y = pSrc3DObj->depthD16[y*DEPTH_WIDTH+x];
				tdPoint.z = -(y-m_nDepthHeight/2);

				DepthTo3D(&tdPoint);*/

				//存储坐标
				/*pSrc3DObj->x[x][y] = tdPoint.x;
				pSrc3DObj->y[x][y] = tdPoint.y;
				pSrc3DObj->z[x][y] = tdPoint.z;*/

			}
			else
			{
				pTransform3DObject->en[x][y] = false;
			}
		}
	}

	//进行三维变换，结果存在pDest3DObj中供显示
	SrcToDest();
}


//[y]+远离 -靠近 [z]+高 -低
void CDepthTo3D::SrcToDest(void)
{
	//1、俯仰
	double fSin = sin(m_fCameraPitch*3.14 / 180);
	double fCos = cos(m_fCameraPitch*3.14 / 180);
	double ty, tz;
	for (int y=0;y<DEPTH_HEIGHT;y++)
	{
		for (int x=0;x<DEPTH_WIDTH;x++)
		{
			if (pTransform3DObject->en[x][y] == true)
			{
				ty = pTransform3DObject->y[x][y];
				tz = pTransform3DObject->z[x][y];
				pTransform3DObject->y[x][y] = ty * fCos + tz * (-fSin);
				pTransform3DObject->z[x][y] = ty * fSin + tz * fCos;
			}
		}
	}
	

	//2、左右选择

	//3、平移
	for (int y = 0; y < DEPTH_HEIGHT; y++)
	{
		for (int x = 0; x < DEPTH_WIDTH; x++)
		{
			if (pTransform3DObject->en[x][y] == true)
			{
				pTransform3DObject->z[x][y] += m_fCameraAboveGround;

				///////
				//pDataHolder->pTransform3DObject->y[x][y] *= 1.2;
				//////
			}
		}
	}
	
	memcpy(pDisplay3DObject, pTransform3DObject, sizeof(WP_K_3D_Object));
}

USHORT DepthNear(USHORT inBase,USHORT inNear)
{
	int diff = abs((int)inBase - inNear);
	if (diff > 100)
	{
		return inBase;
	}
	else
	{
		return inNear;
	}
}

void CDepthTo3D::Algo_DepthSmooth(void)
{
	PrintInfo(L"Algo_DepthSmooth");
	int tmpDepth = 0;
	//最外一圈不算了
	for (int y=1;y<DEPTH_HEIGHT-1;y++)
	{
		for (int x=1;x<DEPTH_WIDTH-1;x++)
		{
			tmpDepth = pSrc3DObject->depthD16[y*DEPTH_WIDTH + x];
			tmpDepth += DepthNear(pSrc3DObject->depthD16[y*DEPTH_WIDTH + x],pSrc3DObject->depthD16[(y + 1)*DEPTH_WIDTH + x]);
			tmpDepth += DepthNear(pSrc3DObject->depthD16[y*DEPTH_WIDTH + x], pSrc3DObject->depthD16[(y - 1)*DEPTH_WIDTH + x]);
			tmpDepth += DepthNear(pSrc3DObject->depthD16[y*DEPTH_WIDTH + x], pSrc3DObject->depthD16[y*DEPTH_WIDTH + x + 1]);
			tmpDepth += DepthNear(pSrc3DObject->depthD16[y*DEPTH_WIDTH + x], pSrc3DObject->depthD16[y*DEPTH_WIDTH + x - 1]);
			tmpDepth = tmpDepth/5;

			pSrc3DObject->depthD16[y*DEPTH_WIDTH + x] = tmpDepth;
		}
	 }
	}


int CDepthTo3D::GetColorXFromDepth(int inDepthX, int inDepthY)
{
	int nColorX = pTransform3DObject->DepthToColorX[inDepthY*DEPTH_WIDTH + inDepthX];

	if (nColorX < 0 || nColorX >= COLOR_WIDTH)
	{
		PrintInfo(L"nColorX < 0 || nColorX >= COLOR_WIDTH");
		for (int Range = 1; Range < DEPTH_WIDTH / 2; Range++)
		{
			for (int x = inDepthX - Range; x < inDepthX + Range; x++)
			{
				for (int y = inDepthY - Range; y < inDepthY + Range; y++)
				{
					nColorX = pTransform3DObject->DepthToColorX[y*DEPTH_WIDTH + x];
					if (nColorX >= 0 && nColorX < COLOR_WIDTH)
					{
						break;
					}
				}
			}
		}
	}

	return nColorX;
}


int CDepthTo3D::GetColorYFromDepth(int inDepthX, int inDepthY)
{
	int nColorY = pTransform3DObject->DepthToColorY[inDepthY*DEPTH_WIDTH + inDepthX];

	if (nColorY < 0 || nColorY >= COLOR_HEIGHT)
	{

		PrintInfo(L"nColorY < 0 || nColorY >= COLOR_HEIGHT");
		for (int Range = 1; Range < DEPTH_WIDTH / 2; Range++)
		{
			for (int x = inDepthX - Range; x < inDepthX + Range; x++)
			{
				for (int y = inDepthY - Range; y < inDepthY + Range; y++)
				{
					nColorY = pTransform3DObject->DepthToColorY[y*DEPTH_WIDTH + x];
					if (nColorY >= 0 && nColorY < COLOR_WIDTH)
					{
						break;
					}
				}
			}
		}
	}

	return nColorY;
}

void CDepthTo3D::PrintInfo(CString inStr)
{
	if (pInfoList != NULL)
	{
		pInfoList->AddString(inStr);
		pInfoList->SetCurSel(pInfoList->GetCount()-1);
	}
}

