
// GK_KinectDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GK_Kinect.h"
#include "GK_KinectDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static int nPlayIndex = 0;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGK_KinectDlg 对话框



CGK_KinectDlg::CGK_KinectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGK_KinectDlg::IDD, pParent)
	, m_nPlayInterval(1000)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGK_KinectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INFOLIST, m_cInfoList);
	DDX_Text(pDX, IDC_INP_PLAY_INTERVAL, m_nPlayInterval);
	DDX_Control(pDX, IDC_DIS_TMR, m_cDisTmr);
	DDX_Control(pDX, IDC_DIS_INFO, m_cDisInfo);
	DDX_Control(pDX, IDC_INFO_VIEW, m_cInfoView);
	DDX_Control(pDX, IDC_EDIT2, timeMSY);
}

BEGIN_MESSAGE_MAP(CGK_KinectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_KINECT_OPEN, &CGK_KinectDlg::OnBnClickedKinectOpen)
	ON_BN_CLICKED(IDC_KINECT_CLOSE, &CGK_KinectDlg::OnBnClickedKinectClose)
	ON_BN_CLICKED(IDC_KINECT_LOG, &CGK_KinectDlg::OnBnClickedKinectLog)
	ON_BN_CLICKED(IDC_DATA_LOAD, &CGK_KinectDlg::OnBnClickedDataLoad)
	ON_BN_CLICKED(IDC_DATA_PLAY, &CGK_KinectDlg::OnBnClickedDataPlay)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_DATA_PAUSE, &CGK_KinectDlg::OnBnClickedDataPause)
	ON_BN_CLICKED(IDC_DATA_ONE_FRAME, &CGK_KinectDlg::OnBnClickedDataOneFrame)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CGK_KinectDlg 消息处理程序

BOOL CGK_KinectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	InitGL();
	m_cRobotCore.m_cKinectDataStream.pInfoList = &m_cInfoList;
	m_cRobotCore.m_DataProcess.pInfoList = &m_cInfoList;
	m_cRobotCore.m_DataProcess.pInfoDis = &m_cDisInfo;
	m_cRobotCore.pDisShow = &m_cInfoView;

	m_kinect.pRobotCore = &m_cRobotCore;
	m_kinect.pInfoList = &m_cInfoList;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGK_KinectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGK_KinectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CGK_KinectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CGK_KinectDlg::OnBnClickedKinectOpen()
{
	// TODO:  在此添加控件通知处理程序代码
	m_kinect.Start();
}


void CGK_KinectDlg::OnBnClickedKinectClose()
{
	// TODO:  在此添加控件通知处理程序代码
	m_kinect.Stop();
}


void CGK_KinectDlg::OnBnClickedKinectLog()
{
	// TODO:  在此添加控件通知处理程序代码
	m_cRobotCore.StartLogKinectStream(50);
}


void CGK_KinectDlg::OnBnClickedDataLoad()
{
	// TODO:  在此添加控件通知处理程序代码
	CFileDialog fdlg(TRUE, L"k2", L"Default.k2", NULL, L"Kinect2数据流文件类型二(*.k2)|*.k2", this);
	if (fdlg.DoModal() != IDOK)
	{
		return;
	}

	CString strFn = fdlg.GetPathName();
	m_cRobotCore.m_cKinectDataStream.LoadFromBinFile(strFn);

	//回放数据
	OnBnClickedDataPlay();
}


void CGK_KinectDlg::OnBnClickedDataPlay()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	nPlayIndex = 0;
	SetTimer(TMR_PLAY, m_nPlayInterval, NULL);
}


void CGK_KinectDlg::InitGL()
{
	m_oglStatic.Create(L"openGLstatic", WS_VISIBLE, CRect(0, 0, 800, 600), this, IDC_GL_VIEW);
	m_oglStatic.pInfoList = &m_cInfoList;
	m_oglStatic.pDepthTo3D = &(m_cRobotCore.m_cDepthTo3D);
	m_cRobotCore.pGL = &m_oglStatic;
}


void CGK_KinectDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == TMR_PLAY)
	{
		if (nPlayIndex >= m_cRobotCore.m_cKinectDataStream.m_ar3Dobj.size())
		{
			nPlayIndex = 0;
			KillTimer(TMR_PLAY);
			return;
		}

		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Lock();
		memcpy(m_cRobotCore.m_cDepthTo3D.pSrc3DObject, &(m_cRobotCore.m_cKinectDataStream.m_ar3Dobj[nPlayIndex]), sizeof(WP_K_3D_Object));
		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Unlock();
		m_cRobotCore.ProcessEvent();

		CString strTmp;
		strTmp.Format(L"正在回放第 [%d] 帧", nPlayIndex + 1);
		m_cDisTmr.SetWindowTextW(strTmp);

		nPlayIndex++;
		
		//////////////////////////////////////////////////////////
		/*if (nPlayIndex %2 == 0)
		{
		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Lock();
		m_3dFile.Load3DObjFile(L"../../_3D_DataLog/公司桌面0.3df2", m_cRobotCore.m_cDepthTo3D.pSrc3DObject, 1);
		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Unlock();

		m_cRobotCore.m_cDepthTo3D.m_fCameraAboveGround = 1.5f;
		m_cRobotCore.m_cDepthTo3D.m_fCameraPitch = -28;

		m_cRobotCore.ProcessEvent();
		}
		else
		{
		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Lock();
		m_3dFile.Load3DObjFile(L"../../_3D_DataLog/家里桌面1.3df2", m_cRobotCore.m_cDepthTo3D.pSrc3DObject, 1);
		m_cRobotCore.m_cDepthTo3D.csSrc3DObject.Unlock();

		m_cRobotCore.m_cDepthTo3D.m_fCameraAboveGround = 1.5f;
		m_cRobotCore.m_cDepthTo3D.m_fCameraPitch = -28;

		m_cRobotCore.ProcessEvent();
		}*/
		/////////////////////////////////////////////////////////
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CGK_KinectDlg::OnBnClickedDataPause()
{
	// TODO:  在此添加控件通知处理程序代码
	KillTimer(TMR_PLAY);
}


void CGK_KinectDlg::OnBnClickedDataOneFrame()
{
	// TODO:  在此添加控件通知处理程序代码
	int nCurPlayIndex = nPlayIndex - 1;
	if (nCurPlayIndex >= 0 && nCurPlayIndex < m_cRobotCore.m_cKinectDataStream.m_ar3Dobj.size())
	{
		CString m_strFile;
		CTime tm;
		tm = CTime::GetCurrentTime();
		m_strFile.Format(L"%d-%d-%d_%d-%d-%d.k2", tm.GetYear(), tm.GetMonth(), tm.GetDay(), tm.GetHour(), tm.GetMinute(), tm.GetSecond());
		m_strFile = L"../../_KinectStream/" + m_strFile;

		CFile fileWrite;		//记录文件
		BOOL fo_res;			//记录文件打开是否成功

		//打开文件
		fo_res = fileWrite.Open(m_strFile, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite);
		if (fo_res == TRUE)
		{
			//开始写入数据
			fileWrite.Write(&(m_cRobotCore.m_cKinectDataStream.m_ar3Dobj[nCurPlayIndex]), sizeof(stWP_K_3D_Object));

			fileWrite.Close();

			CString strTmp;
			strTmp.Format(L"当前帧数据被到%s", m_strFile);
			m_cRobotCore.m_cKinectDataStream.PrintInfo(strTmp);
		}
	}
}


void CGK_KinectDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	m_kinect.Stop();
	Sleep(500);
	CDialogEx::OnClose();
}
