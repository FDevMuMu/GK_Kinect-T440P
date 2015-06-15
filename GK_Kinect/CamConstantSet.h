//CamConstantSet.h
//注意：当运动跟踪目标切换时 原来的积分量速度记录 等还在保存 会引起初运动不正常
//#include "stdafx.h"

#ifndef CamConstantSet_h
#define CamConstantSet_h

//这里设置一些 程序常量	21:39 2008-11-3
const int OMNI_VISION_CAM_WIDTH = 640;//320;//全景分辨率x
const int OMNI_VISION_CAM_HEIGHT= 480;//240;//全景分辨率y
const int MIRROR_CENTER_X		= OMNI_VISION_CAM_WIDTH/2;//镜子中心x
const int MIRROR_CENTER_Y		= OMNI_VISION_CAM_HEIGHT/2;//镜子中心y
const int MIRROR_RADIUS			= OMNI_VISION_CAM_HEIGHT/2;//镜子半径r
const int MIRROR_LIMIT_L		= MIRROR_CENTER_X-MIRROR_RADIUS;//镜子左边界	自动计算出的常量
const int MIRROR_LIMIT_R		= MIRROR_CENTER_X+MIRROR_RADIUS;//镜子左边界	自动计算出的常量
const int MIRROR_LIMIT_UP		= MIRROR_CENTER_Y+MIRROR_RADIUS;//镜子上边界	自动计算出的常量
const int MIRROR_LIMIT_DOWN		= MIRROR_CENTER_Y-MIRROR_RADIUS;//镜子下边界	自动计算出的常量

const int circle_mask_radius_min =0;//20//0 //设置全景掩膜 半径最小长度		//影响包括 HSV转换 二值化 连通域
const int circle_mask_radius_max =240;//176;//90  //设置全景掩膜 半径最大长度

//深度测量摄像机 程序常量
const int DEPTH_VISION_CAM_WIDTH = 640;//320;//全景分辨率x
const int DEPTH_VISION_CAM_HEIGHT= 480;//240;//全景分辨率y
const int DEPTH_VISION_CENTER_X		= DEPTH_VISION_CAM_WIDTH/2;//视野中心x
const int DEPTH_VISION_CENTER_Y		= DEPTH_VISION_CAM_HEIGHT/2;//视野中心y
//坐标原点摄像机镜头 平行于ccd y轴向上为y  平行于ccd x轴向右为x  垂直于ccd (平行于光轴)摄像方向(向前)为z   注:方向参考为平行于光轴看摄像机背面 视线同向于光轴
const float DEPTH_VISION_Z_DATA2METER_P	    = 1.0;//单位 毫米mm   z轴数据变成毫米制乘的比率系数 z_meter = z_data * DEPTH_VISION_Z_DATA2METER_P
//x_meter = z_data * DEPTH_VISION_ZX_DATA2METER_P  *(x_坐标 - DEPTH_VISION_CENTER_X)
const float DEPTH_VISION_ZX_DATA2METER_P	= 0.001512;//float(1.0/320.0);// x轴数据变成毫米制乘的比率系数   标定数据 z=1240mm 的平行x轴直线上 据中心320像素 距离中心600mm   600=1240*320*0.001512
//y_meter = y_data * DEPTH_VISION_ZX_DATA2METER_P  *(x_坐标 - DEPTH_VISION_CENTER_X)
const float DEPTH_VISION_ZY_DATA2METER_P	= 0.001512;//float(1.0/240.0);// y轴数据变成毫米制乘的比率系数 

#endif CamConstantSet_h