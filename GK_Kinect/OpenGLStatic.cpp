// OpenGLstatic.cpp : 实现文件
//

#include "stdafx.h"
#include "OpenGLstatic.h"
#include <math.h>

// COpenGLStatic

IMPLEMENT_DYNAMIC(COpenGLStatic, CStatic)

COpenGLStatic::COpenGLStatic()
{
	pInfoList = NULL;
	m_bLBtnDown = false;
	m_bRBtnDown = false;


	m_fBaseRotY = -90.0f;
	m_fBaseRotX = 0.0f;
	m_fTempRotZ = 0;
	m_fBaseDist = -5.0f;  //距离
	m_fTempRotY = m_fBaseRotY;
	m_fTempRotX = m_fBaseRotX ;
	m_fTempDist = m_fBaseDist ;

	//相机位置和视野
	double xRes = (70*3.14)/(DEPTH_WIDTH*180);
	double zRes = (60*3.14)/(DEPTH_HEIGHT*180);
	m_CameraPos.x = m_CameraPos.y = m_CameraPos.z = 0;
	m_arViewRect[0].x = 256*xRes;
	m_arViewRect[0].y = 2.0;
	m_arViewRect[0].z = 212*zRes;

	m_arViewRect[1].x = -256 * xRes;
	m_arViewRect[1].y = 2.0;
	m_arViewRect[1].z = 212 * zRes;

	m_arViewRect[2].x = -256 * xRes;
	m_arViewRect[2].y = 2.0;
	m_arViewRect[2].z = -212 * zRes;

	m_arViewRect[3].x = 256 * xRes;
	m_arViewRect[3].y = 2.0;
	m_arViewRect[3].z = -212 * zRes;

	m_fK = 1.0f/1;

	m_arMarkPoint = new trPoint[10];
	for (int i=0;i<10;i++)
	{
		m_arEnMarkPoint[i] = false;
	}

	quadratic=gluNewQuadric();
	m_arMarkCylinder = new trPoint[10];
	for (int i=0;i<10;i++)
	{
		m_arEnMarkCylinder[i] = false;
	}

	//人脸识别的显示
	m_posFace3D = new trPoint[50];
	m_nFaceNum = 0;

	m_fMarkK = m_fK;

	//视角点的坐标
	m_fX_Fix = 0.0;
	m_fY_Fix = -1.0;
	m_fZ_Fix = 0.0;

	nTrack = 0;
}

COpenGLStatic::~COpenGLStatic()
{
	if (wglGetCurrentContext() != NULL)
	{
		wglMakeCurrent(NULL, NULL);
	}
	if (m_hRC != NULL)
	{
		wglDeleteContext(m_hRC);
		m_hRC = NULL;
	}

	delete []m_arMarkPoint;
	delete []m_arMarkCylinder;

	delete []m_posFace3D;
}


BEGIN_MESSAGE_MAP(COpenGLStatic, CStatic)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()



// COpenGLStatic 消息处理程序


BOOL COpenGLStatic::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此添加专用代码和/或调用基类

	cs.lpszClass = ::AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC,
		::LoadCursor(NULL, IDC_ARROW), NULL, NULL);
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	return CStatic::PreCreateWindow(cs);
}


int COpenGLStatic::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	//Get a DC for the Client Area
	m_pDC = new CClientDC(this);

	//Failure to Get DC
	if( m_pDC == NULL )
		return FALSE;

	if( !SetupPixelFormat() )
		return FALSE;

	//Create Rendering Context
	m_hRC = ::wglCreateContext( m_pDC->GetSafeHdc() );

	//Failure to Create Rendering Context
	if( m_hRC == 0 )
		return FALSE;

	//Make the RC Current
	if( ::wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) == FALSE )
		return FALSE;

	InitGL();
	return 0;
}


void COpenGLStatic::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码

	WP_K_3D_Object* p3DObj = pDepthTo3D->pDisplay3DObject;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    // Clear Screen And Depth Buffer
	glLoadIdentity(); 

	glTranslatef(m_fX_Fix, m_fY_Fix, m_fZ_Fix);

	glTranslatef(0.0f, 0.0f, m_fTempDist); //m_fTempDist为眼睛视点到物体的距离

	GlRotatef(m_fTempRotX,'y');
	GlRotatef(m_fTempRotY,'x');

	//画相机位置和视野
	//GLDrawCamView();

	//画参考面
	GlDrawAxe();

	//////////////////////////////////////////////////////////////////////////
	//画立体图像点阵
	//glEnable(GL_POINT_SMOOTH);
	//glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glPointSize(2); 
	float tfR,tfG,tfB;
	trPoint tdPoint;
	glBegin(GL_POINTS);
	for (int y=0;y<DEPTH_HEIGHT;y++)
	{
		for (int x=0;x<DEPTH_WIDTH;x++)
		{
			if (p3DObj!= NULL)
			{
				if (p3DObj->en[x][y] == true)
				{
					tfB = (float)(p3DObj->B[x][y])/0xff;
					tfG = (float)(p3DObj->G[x][y])/0xff;
					tfR = (float)(p3DObj->R[x][y])/0xff;
					glColor3f(tfR,tfG,tfB);
					//glColor3f(1.0f,1.0f,1.0f);

					tdPoint.x = p3DObj->x[x][y];
					tdPoint.y = p3DObj->y[x][y];
					tdPoint.z = p3DObj->z[x][y];
					glVertex3f(tdPoint.x*m_fK,tdPoint.y*m_fK,tdPoint.z*m_fK);

					//float fx = (float)x/640;
					//float fy = (float)y/480;
					//glVertex3f(fx,1.0,fy);
				}
			}

		}
	}
	glEnd();
	/////////////
	//画相机位置
	/*glTranslatef(0,0,m_CameraPos.z);
	GlRotatef(pDataHolder->m_fCameraPitch,'X');
	m_CameraPos.z = pDataHolder->m_nCameraAboveGround * m_fK;
	glColor3f(1.0f,0.0f,0.0f);
	GlDrawStandCubiodLines(
		0,
		0,
		0,
		0.4f,
		0.2f,
		0.2f
		);
	GlRotatef(-pDataHolder->m_fCameraPitch, 'X');
	glTranslatef(0,0,-m_CameraPos.z);*/
	//////////////////////////////////////////////////////////////
	//画连接线
	glLineWidth(2.0f);
	for (int i=0;i<10;i++)
	{
		if (m_arEnMarkPoint[i] == true)
		{
			glColor3f(0.0f,1.0f,1.0f);
			GlDrawLine(m_CameraPos.x,m_CameraPos.y,m_CameraPos.z, m_arMarkPoint[i].x,m_arMarkPoint[i].y,m_arMarkPoint[i].z);
		}
	}
	glLineWidth(1.0f);
	//////////////////////////////////////////////////////////////
	//画圆柱
	glLineWidth(12.0f);
	for (int i=0;i<10;i++)
	{
		if (m_arEnMarkCylinder[i] == true)
		{
			glColor3f(0.0f,0.8f,0.8f);
			glTranslatef(m_arMarkCylinder[i].x,m_arMarkCylinder[i].y,m_arMarkCylinder[i].z);
			gluCylinder(quadratic,0.6f,0.6f,0.1f,32,32);
			glTranslatef(-m_arMarkCylinder[i].x,-m_arMarkCylinder[i].y,-m_arMarkCylinder[i].z);
		}
	}
	//////////////////////
	//画轨迹线
	if (nTrack > 1 )
	{
		glLineWidth(10.0f);
		glColor3f(1.0f, 0.0f, 1.0f);
		for (int i = 1; i < nTrack; i++)
		{
			GlDrawLine(arTrack[i - 1].x, arTrack[i - 1].y, arTrack[i - 1].z, arTrack[i].x, arTrack[i].y, arTrack[i].z);
		}

		glPointSize(12);
		trPoint tdPoint;
		glBegin(GL_POINTS);
		for (int i = 0; i < nTrack; i++)
		{
			glColor3f(0.0f,1.0f,0.0f);

			tdPoint.x = arTrack[i].x;
			tdPoint.y = arTrack[i].y;
			tdPoint.z = arTrack[i].z;
			glVertex3f(tdPoint.x*m_fK, tdPoint.y*m_fK, tdPoint.z*m_fK);
		}
		glEnd();
	}
	

	//////////////////////////////////////////////////////////////
	//画立方体线框（连通域对象）
	//if (pDataHolder != NULL)
	//{
	//	for (int i = 0; i<pDataHolder->nDisObjectNum; i++)
	//	{
	//		UINT nFaceIn = isFaceIn(i);
	//		switch(nFaceIn)
	//		{
	//		case 0: //没有人脸，只做一般显示
	//			glLineWidth(1.0f);
	//			glColor3f(0.0f,1.0f,0.0f);
	//			break;
	//		case 1: //有人脸，没举手
	//			glLineWidth(1.0f);
	//			glColor3f(1.0f,1.0f,0.0f);
	//			break;
	//		case 2: //有人脸，而且还举手了
	//			glLineWidth(5.0f);
	//			glColor3f(1.0f,0.0f,1.0f);
	//			break;
	//		}

	//		if (i == pDataHolder->nFocusObjectIndex)
	//		{
	//			//被关注的桌面物体
	//			glLineWidth(5.0f);
	//			glColor3f(1.0f, 1.0f, 0.0f);
	//		}
	//		
	//		GlDrawStandCubiodLines(
	//			pDataHolder->arDisObject[i].nCenterX * m_fK,
	//			pDataHolder->arDisObject[i].nCenterY * m_fK,
	//			pDataHolder->arDisObject[i].nCenterZ * m_fK,
	//			pDataHolder->arDisObject[i].nWidthX * m_fK,
	//			pDataHolder->arDisObject[i].nWidthY * m_fK,
	//			pDataHolder->arDisObject[i].nWidthZ * m_fK
	//			);
	//	}
	//}
	////////////////////////////////////////////////////////////////
	//glLineWidth(1.0f);
	////画人脸检测标记
	//if (pDepthTo3D->pDisplay3DObject->nFaceNum > 0)
	//{
	//	glColor3f(1.0f, 0.0f, 1.0f);
	//	glLineWidth(2.0f);
	//	for (int i = 0; i < pDepthTo3D->pDisplay3DObject->nFaceNum; i++)
	//	{
	//		float facex = pDepthTo3D->pDisplay3DObject->posFace3D[i].x * m_fK;
	//		float facey = pDepthTo3D->pDisplay3DObject->posFace3D[i].y * m_fK;
	//		float facez = pDepthTo3D->pDisplay3DObject->posFace3D[i].z * m_fK;
	//		GlDrawLine(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z, facex, facey, facez);
	//		glTranslatef(facex, facey, 0);
	//		gluCylinder(quadratic, 0.3f, 0.3f, 0.2f, 32, 32);
	//		glTranslatef(-facex, -facey, 0);
	//	}
	//	glLineWidth(1.0f);
	//}
	//////////////////////////////////////////////////////////////
	//招手检测
	/*glColor3f(0.0f,1.0f,1.0f);
	glLineWidth(5.0f);
	for (int i=0;i<50;i++)
	{
	if (pShare->bWavHand[i] == true)
	{
	float facex = m_posFace3D[i].x * m_fK;
	float facey = m_posFace3D[i].y * m_fK;
	float facez = m_posFace3D[i].z * m_fK;
	GlDrawLine(m_CameraPos.x,m_CameraPos.y,m_CameraPos.z, facex,facey,facez);
	}
	}
	glLineWidth(1.0f);*/
	//////////////////////////////////////////////////////////////
	//骨骼绘制（1、全身节点）
	/*glColor3f(0.0f, 1.0f, 1.0f);
	glLineWidth(1.0f);
	for (int i = 0; i < 6; i++)
	{
		if (pDataHolder->bSkeleton[i] == true)
		{
			for (int j = 0; j < 25; j++)
			{
				float skx = pDataHolder->arSkeleton[i][j].x * m_fK;
				float sky = pDataHolder->arSkeleton[i][j].y * m_fK;
				float skz = pDataHolder->arSkeleton[i][j].z * m_fK;
				GlDrawLine(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z, skx, sky, skz);
			}
		}
	}*/
	//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////
	//骨骼绘制（正经骨骼节点）
	//glColor3f(0.0f, 1.0f, 1.0f);
	//glLineWidth(1.0f);
	//for (int i = 0; i < 6; i++)
	//{
	//	if (pDepthTo3D->pDisplay3DObject->bSkeleton[i] == true)
	//	{
	//		/*for (int j = 0; j < 25; j++)
	//		{
	//		float skx = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].x * m_fK;
	//		float sky = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].y * m_fK;
	//		float skz = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].z * m_fK;
	//		GlDrawLine(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z, skx, sky, skz);
	//		}*/
	//		// Torso
	//		m_DrawLine(i, 3, 2);
	//		m_DrawLine(i, 2, 20);
	//		m_DrawLine(i, 20, 1);
	//		m_DrawLine(i, 1, 0);
	//		m_DrawLine(i, 20, 8);
	//		m_DrawLine(i, 20, 4);
	//		m_DrawLine(i, 0, 16);
	//		m_DrawLine(i, 0, 12);

	//		// Right Arm    
	//		m_DrawLine(i, 8, 9);
	//		m_DrawLine(i, 9, 10);
	//		m_DrawLine(i, 10, 11);
	//		m_DrawLine(i, 11, 23);
	//		m_DrawLine(i, 10, 24);

	//		// Left Arm
	//		m_DrawLine(i, 4, 5);
	//		m_DrawLine(i, 5, 6);
	//		m_DrawLine(i, 6, 7);
	//		m_DrawLine(i, 7, 21);
	//		m_DrawLine(i, 6, 22);

	//		// Right Leg
	//		m_DrawLine(i, 16, 17);
	//		m_DrawLine(i, 17, 18);
	//		m_DrawLine(i, 18, 19);

	//		// Left Leg
	//		m_DrawLine(i, 12, 13);
	//		m_DrawLine(i, 13, 14);
	//		m_DrawLine(i, 14, 15);
	//	}
	//}
	////////////////////////////////////////////////////////////////
	////骨骼绘制（2、头部节点）
	//glColor3f(1.0f, 0.0f, 1.0f);
	//glLineWidth(2.0f);
	//for (int i = 0; i < 6; i++)
	//{
	//	if (pDepthTo3D->pDisplay3DObject->bSkeleton[i] == true)
	//	{
	//		int j = 3;
	//		float skx = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].x * m_fK;
	//		float sky = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].y * m_fK;
	//		float skz = pDepthTo3D->pDisplay3DObject->arSkeleton[i][j].z * m_fK;
	//		GlDrawLine(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z, skx, sky, skz);
	//		glTranslatef(skx, sky, 0);
	//		gluCylinder(quadratic, 0.3f, 0.3f, 0.1f, 32, 32);
	//		glTranslatef(-skx, -sky, 0);
	//	}
	//}
	//glLineWidth(1.0f);
	////////////////////////////////////////////////////////////////
	////显示文字
	//if (pDataHolder->arStrPos.size() > 0)
	//{
	//	GlSelectFont(12, GB2312_CHARSET, "楷体_GB2312");
	//	glColor3f(1.0f, 1.0f, 0.0f);
	//	for (int i = 0; i < pDataHolder->arStrPos.size(); i++)
	//	{
	//		float tx = pDataHolder->arStrPos[i].x * m_fK;
	//		float ty = pDataHolder->arStrPos[i].y * m_fK;
	//		float tz = pDataHolder->arStrPos[i].z * m_fK;
	//		glTranslatef(0, 0, tz);
	//		glRasterPos2f(tx, ty);
	//		GlDrawTxt(pDataHolder->arString[i]);
	//		glTranslatef(0, 0, -tz);
	//	}
	//}
	///////////////////////////////////////////////////

	glFlush();
	SwapBuffers(dc.m_hDC);

	/////////////////////////////////////////////////////////
	/*
	//
	// just do something simple for testing
	//
	glClear(GL_COLOR_BUFFER_BIT);	
	//
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0f,2.0f,-1.0f,2.0f,-1.0f,1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(1.0f,0.0f,0.0f);
	glVertex3f(1.0f,1.0f,0.0f);
	glEnd();
	glFlush();
	*/
	////////////////////////////////////////////////////////////////////

	// 不为绘图消息调用 CStatic::OnPaint()
}


BOOL COpenGLStatic::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//return CStatic::OnEraseBkgnd(pDC);
	return TRUE;	// openGL does this
}


BOOL  COpenGLStatic::SetupPixelFormat(void)
{
	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
		1, // version number
		PFD_DRAW_TO_WINDOW | // support window
		PFD_SUPPORT_OPENGL | // support OpenGL
		//	PFD_DOUBLEBUFFER, // double buffered
		PFD_TYPE_RGBA, // RGBA type
		16, // 24-bit color depth
		0, 0, 0, 0, 0, 0, // color bits ignored
		0, // no alpha buffer
		0, // shift bit ignored
		0, // no accumulation buffer
		0, 0, 0, 0, // accumulation bits ignored
		16, // 16-bit z-buffer
		0, // no stencil buffer
		0, // no auxiliary buffer
		PFD_MAIN_PLANE, // main layer
		0, // reserved
		0, 0, 0 // layer masks ignored
	};

	int m_nPixelFormat = ::ChoosePixelFormat( m_pDC->GetSafeHdc(), &pfd );

	if ( m_nPixelFormat == 0 )
		return FALSE;

	return ::SetPixelFormat( m_pDC->GetSafeHdc(), m_nPixelFormat, &pfd );
}

void COpenGLStatic::PrintInfo(CString inInfo)
{
	if (pInfoList != NULL)
	{
		pInfoList->AddString(inInfo);
		pInfoList->SetCurSel(pInfoList->GetCount()-1);
	}
}

void COpenGLStatic::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bLBtnDown = true;
	lastLPnt = point;

	CStatic::OnLButtonDown(nFlags, point);
}

void COpenGLStatic::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_fBaseRotX = m_fTempRotX;
	m_fBaseRotY = m_fTempRotY;
	m_bLBtnDown = false;

	CStatic::OnLButtonUp(nFlags, point);
}

void COpenGLStatic::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bRBtnDown = true;
	lastRPnt = point;

	CStatic::OnRButtonDown(nFlags, point);
}

void COpenGLStatic::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_fBaseRotX = m_fTempRotX;
	m_fBaseDist = m_fTempDist;
	m_bRBtnDown = false;

	CStatic::OnRButtonUp(nFlags, point);
}


void COpenGLStatic::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_bLBtnDown == true)
	{
		float dx = (float)(point.x - lastLPnt.x)/2;
		m_fTempRotX = m_fBaseRotX + dx;

		float dy = (float)(point.y - lastLPnt.y) / 2;
		m_fTempRotY = m_fBaseRotY + dy;
		//不让转到地平线以下
		if(m_fTempRotY < -90)
		{
			m_fTempRotY = -90;
		}
		Invalidate(FALSE);
	}

	if (m_bRBtnDown == true)
	{
		float dx = (float)(point.x - lastRPnt.x)/2;
		m_fTempRotX = m_fBaseRotX + dx;

		float dy = (float)(point.y - lastRPnt.y) / 2;
		m_fTempDist = m_fBaseDist + dy;
		Invalidate(FALSE);
	}

	CStatic::OnMouseMove(nFlags, point);
}


void COpenGLStatic::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	GLsizei width, height;
	width = cx;
	height = cy;
	if (height == 0)                                        // Prevent A Divide By Zero By
	{
		height = 1;                                        // Making Height Equal One
	}

	glViewport(0, 0, width, height);                        // Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
	glLoadIdentity();                                    // Reset The Projection Matrix
	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);                            // Select The Modelview Matrix
	glLoadIdentity();                                    // Reset The Modelview Matrix
}


void COpenGLStatic::InitGL(void)
{
	glShadeModel(GL_SMOOTH);                            // Enable Smooth Shading
	glClearColor(0.0, 0.0, 0.0, 0.0);// Black Background
	glClearDepth(1.0f);                                    // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);                            // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);                                // The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);    // Really Nice Perspective Calculations
	
	//SetupLights();
}


void COpenGLStatic::GlDrawLine(float x1,float y1,float z1,float x2,float y2,float z2)//画线函数
{ 
	glBegin(GL_LINES);  //双顶点线段
	glVertex3f( x1, y1, z1);//1点
	glVertex3f( x2, y2, z2);//2点
	glEnd();
}

void COpenGLStatic::GlRotatef(float angle, char axes)
{
	switch (axes)
	{
	case 'x':
	case 'X':
		glRotatef(angle,1,0,0);
		break;
	case 'y':
	case 'Y':
		glRotatef(angle,0,1,0);
		break;
	case 'z':
	case 'Z':
		glRotatef(angle,0,0,1);
		break;
	default:
		break;
	}
}


void COpenGLStatic::GlDrawSolidCube(float x,float y,float z,float inSize)	//画立方体
{ 
	glBegin(GL_QUADS);
	//Z轴对面
	glVertex3f(x-inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z-inSize);

	glVertex3f(x-inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);

	//X轴对面
	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z-inSize);

	glVertex3f(x-inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z-inSize);

	//Y轴对面
	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z-inSize);

	glVertex3f(x-inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z-inSize);
	glEnd();

	//画线
	glColor3f(1.0f, 1.0f, 1.0f);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	//Z轴对面
	glVertex3f(x-inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y+inSize, z-inSize);
	glVertex3f(x+inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y-inSize, z-inSize);


	glVertex3f(x-inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);
	glVertex3f(x-inSize, y+inSize, z+inSize);

	//两个Z面之间连线
	glVertex3f(x-inSize, y-inSize, z-inSize);
	glVertex3f(x-inSize, y-inSize, z+inSize);

	glVertex3f(x-inSize, y+inSize, z-inSize);
	glVertex3f(x-inSize, y+inSize, z+inSize);

	glVertex3f(x+inSize, y-inSize, z-inSize);
	glVertex3f(x+inSize, y-inSize, z+inSize);

	glVertex3f(x+inSize, y+inSize, z-inSize);
	glVertex3f(x+inSize, y+inSize, z+inSize);
	glEnd();
}


void COpenGLStatic::GlDrawStandCubiodLines(float x,float y,float z,float inXWidth, float inYWidth, float inZWidth)	//画立方体外框线
{ 
	//画线
	glLineWidth(1.0f);
	GLfloat halfXWidth = inXWidth/2;
	GLfloat halfYWidth = inYWidth/2;
	GLfloat halfZWidth = inZWidth/2;
	glBegin(GL_LINES);
	//Z轴对面
	glVertex3f(x-halfXWidth, y-halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y-halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y-halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x-halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x-halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x-halfXWidth, y-halfYWidth, z-halfZWidth);


	glVertex3f(x-halfXWidth, y+halfYWidth, z+halfZWidth);
	glVertex3f(x+halfXWidth, y+halfYWidth, z+halfZWidth);
	glVertex3f(x+halfXWidth, y+halfYWidth, z+halfZWidth);
	glVertex3f(x+halfXWidth, y-halfYWidth, z+halfZWidth);
	glVertex3f(x+halfXWidth, y-halfYWidth, z+halfZWidth);
	glVertex3f(x-halfXWidth, y-halfYWidth, z+halfZWidth);
	glVertex3f(x-halfXWidth, y-halfYWidth, z+halfZWidth);
	glVertex3f(x-halfXWidth, y+halfYWidth, z+halfZWidth);

	//两个Z面之间连线
	glVertex3f(x-halfXWidth, y-halfYWidth, z-halfZWidth);
	glVertex3f(x-halfXWidth, y-halfYWidth, z+halfZWidth);

	glVertex3f(x-halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x-halfXWidth, y+halfYWidth, z+halfZWidth);

	glVertex3f(x+halfXWidth, y-halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y-halfYWidth, z+halfZWidth);

	glVertex3f(x+halfXWidth, y+halfYWidth, z-halfZWidth);
	glVertex3f(x+halfXWidth, y+halfYWidth, z+halfZWidth);
	glEnd();
}


void COpenGLStatic::SetupLights(void)
{
	GLfloat ambientLight[]={1.0f,1.0f,1.0f,0.5f};//白色主光源环境光  
	GLfloat diffuseLight[]={1.0f,1.0f,1.0f,1.0f};//白色主光源漫反射  
	GLfloat specularLight[]={1.0f,1.0f,1.0f,1.0f};//白色主光源镜面光  

	GLfloat lightPos[]={1.0f,0.0f,0.0f,0.0f};  //光源位置,最后一个值：0-无限远定向光源；非0-定位光源
	GLfloat spotLightPos[]={0.0f,0.0f,20.0f,5.0f}; //射灯位置
	GLfloat spotDir[]={-1.0f,-1.0f,-1.0f};            //射灯方向

	glEnable(GL_LIGHTING);                          //启用光照  
	//glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);   //设置环境光源  
	//glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);   //设置漫反射光源  
	//glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight); //设置镜面光源  
	glLightfv(GL_LIGHT0,GL_POSITION,lightPos);      //设置灯光位置  
	glEnable(GL_LIGHT0);                            //打开白色主光源  

	glEnable(GL_COLOR_MATERIAL);                        //启用材质的颜色跟踪  
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);   //指定材料着色的面  
	//glMaterialfv(GL_FRONT,GL_SPECULAR,specularLight);   //指定材料对镜面光的反应  
	glMateriali(GL_FRONT,GL_SHININESS,100);             //指定反射系数 
}


void COpenGLStatic::GlDrawAxe(void)
{
	glLineWidth(1.0f);
	//画参考面
	glDisable(GL_TEXTURE_2D);                       //关闭贴图
	//glDisable(GL_LIGHTING);                         //关闭光照
	glPushAttrib(GL_CURRENT_BIT);					//保存现有颜色属性
	for(int i = -100; i <= 100; i+=2)
	{
		if(i == 0)
			glColor3f(1, 1, 1);
		else
			glColor3f(0.3f, 0.3f, 0.3f);
		GlDrawLine(-100,i,0,100,i,0);
		GlDrawLine(i,-100,0,i,100,0);
	}
	glPopAttrib();									//恢复前一属性
	//glEnable(GL_LIGHTING);							//光照有效
	glEnable(GL_TEXTURE_2D);                        //贴图有效/
}


void COpenGLStatic::Init2(void)
{
	GLfloat mat_specular[] = {1.0,1.0,1.0,1.0};
	GLfloat mat_shininess[] = { 50.0 };
	GLfloat light_position[] = { 1.0,1.0,1.0,0.0 };
	GLfloat white_light[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat lmodel_ambient[] = { 0.1, 0.1, 0.1, 1.0 };

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT , lmodel_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
}


void COpenGLStatic::GLDrawCamView(void)
{
	//画线
	glLineWidth(1.0f);
	glColor4f(0.0f, 1.0f, 0.0f, 0.1f);

	glBegin(GL_LINES);
	//Z轴对面
	for(int i=0;i<4;i++)
	{
		glVertex3f(m_CameraPos.x, m_CameraPos.y, m_CameraPos.z);
		glVertex3f(m_arViewRect[i].x, m_arViewRect[i].y, m_arViewRect[i].z);
	}
	glEnd();
}

void COpenGLStatic::SetCamPos(float inHeight, float inPitch, float inRotate)
{
	m_CameraPos.z = inHeight*m_fK;
}


void COpenGLStatic::MarkPoint(int inIndex, int inX, int inY, int inZ)
{
	if (inIndex < 0 || inIndex >= 10)
	{
		return;
	}
	m_arMarkPoint[inIndex].x = inX*m_fMarkK;
	m_arMarkPoint[inIndex].y = inY*m_fMarkK;
	m_arMarkPoint[inIndex].z = inZ*m_fMarkK;
	m_arEnMarkPoint[inIndex] = true;
}


void COpenGLStatic::CleanMarkPoints(void)
{
	for (int i=0;i<10;i++)
	{
		m_arEnMarkPoint[i] = false;
	}
}

void  COpenGLStatic::MarkCylinder(int inIndex, int inX, int inY, int inZ)
{
	if (inIndex < 0 || inIndex >= 10)
	{
		return;
	}
	m_arMarkCylinder[inIndex].x = inX*m_fMarkK;
	m_arMarkCylinder[inIndex].y = inY*m_fMarkK;
	m_arMarkCylinder[inIndex].z = inZ*m_fMarkK;
	m_arEnMarkCylinder[inIndex] = true;
}

void  COpenGLStatic::CleanMarkCylinder(void)
{
	for (int i=0;i<10;i++)
	{
		m_arEnMarkCylinder[i] = false;
	}
}


UINT COpenGLStatic::isFaceIn(int inCubiodIndex)
{
	UINT res = 0;
	//trPoint* pPosFace3D = pDepthTo3D->pDisplay3DObject->posFace3D;
	//for (int i = 0; i<pDepthTo3D->pDisplay3DObject->nFaceNum; i++)
	//{
	//	if (
	//		pPosFace3D[i].x > pDataHolder->arDisObject[inCubiodIndex].nCenterX - pDataHolder->arDisObject[inCubiodIndex].nWidthX / 2 &&
	//		pPosFace3D[i].x < pDataHolder->arDisObject[inCubiodIndex].nCenterX + pDataHolder->arDisObject[inCubiodIndex].nWidthX / 2 &&
	//		pPosFace3D[i].y > pDataHolder->arDisObject[inCubiodIndex].nCenterY - pDataHolder->arDisObject[inCubiodIndex].nWidthY / 2 &&
	//		pPosFace3D[i].y < pDataHolder->arDisObject[inCubiodIndex].nCenterY + pDataHolder->arDisObject[inCubiodIndex].nWidthY / 2 &&
	//		pPosFace3D[i].z > pDataHolder->arDisObject[inCubiodIndex].nCenterZ - pDataHolder->arDisObject[inCubiodIndex].nWidthZ / 2 &&
	//		pPosFace3D[i].z < pDataHolder->arDisObject[inCubiodIndex].nCenterZ + pDataHolder->arDisObject[inCubiodIndex].nWidthZ / 2
	//		)
	//	{
	//		/*if (pDepthTo3D->bWavHand[i] == true)
	//		{
	//			res = 2;
	//		} 
	//		else*/
	//		{
	//			res = 1;
	//		}
	//		break;
	//	}
	//}

	//
	//for (int i = 0; i<6; i++)
	//{
	//	if (pDepthTo3D->pDisplay3DObject->bSkeleton[i] == true)
	//	{
	//		trPoint* pJoint = pDepthTo3D->pDisplay3DObject->arSkeleton[i];
	//		if (
	//			pJoint[3].x > pDataHolder->arDisObject[inCubiodIndex].nCenterX - pDataHolder->arDisObject[inCubiodIndex].nWidthX / 2 &&
	//			pJoint[3].x < pDataHolder->arDisObject[inCubiodIndex].nCenterX + pDataHolder->arDisObject[inCubiodIndex].nWidthX / 2 &&
	//			pJoint[3].y > pDataHolder->arDisObject[inCubiodIndex].nCenterY - pDataHolder->arDisObject[inCubiodIndex].nWidthY / 2 &&
	//			pJoint[3].y < pDataHolder->arDisObject[inCubiodIndex].nCenterY + pDataHolder->arDisObject[inCubiodIndex].nWidthY / 2 &&
	//			pJoint[3].z > pDataHolder->arDisObject[inCubiodIndex].nCenterZ - pDataHolder->arDisObject[inCubiodIndex].nWidthZ / 2 &&
	//			pJoint[3].z < pDataHolder->arDisObject[inCubiodIndex].nCenterZ + pDataHolder->arDisObject[inCubiodIndex].nWidthZ / 2
	//			)
	//		{
	//			/*if (pDepthTo3D->bWavHand[i] == true)
	//			{
	//			res = 2;
	//			}
	//			else*/
	//			{
	//				res = 1;
	//			}
	//			break;
	//		}
	//	}
	//}
	return res;
}

void COpenGLStatic::GlSelectFont(int size, int charset, const char* face)
{
	HFONT hFont = CreateFontA(size, 0, 0, 0, FW_MEDIUM, 0, 0, 0,
		charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
	HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), hFont);
	DeleteObject(hOldFont);
}

void COpenGLStatic::GlDrawTxt(CString inStr)
{
	int len, i;
	HDC hDC = wglGetCurrentDC();
	GLuint list = glGenLists(1);

	len = inStr.GetLength();
	WCHAR* pBuf = inStr.GetBuffer(0);

	// 逐个输出字符
	for (i = 0; i < len; ++i)
	{
		wglUseFontBitmapsW(hDC, pBuf[i], 1, list);
		glCallList(list);
	}

	// 回收所有临时资源
	glDeleteLists(list, 1);
}





void COpenGLStatic::m_DrawLine(int inSKIndex, int inFrom, int inTo)
{
	/*float fromX = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inFrom].x * m_fK;
	float fromY = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inFrom].y * m_fK;
	float fromZ = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inFrom].z * m_fK;

	float toX = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inTo].x * m_fK;
	float toY = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inTo].y * m_fK;
	float toZ = pDepthTo3D->pDisplay3DObject->arSkeleton[inSKIndex][inTo].z * m_fK;

	GlDrawLine(fromX, fromY, fromZ, toX, toY, toZ);*/
}
