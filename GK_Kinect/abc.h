#include <string>
#include <stdlib.h>  
#include <iostream> 

//【1】  
//#include <XnCppWrapper.h>  
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "CamConstantSet.h"

using namespace std;

extern  unsigned char dataShow[320 * 240 * 3];	//界面右上角用来显示的数组
extern unsigned int DepthBuf_O_T[DEPTH_VISION_CAM_HEIGHT][DEPTH_VISION_CAM_WIDTH];
extern unsigned int  area_grow_data_obj[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];
extern int connect_area_s_e_w[100000][3];//[100000][3];//[DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH][3];//存储1000个连通域的 首 尾 标号 [首]connect_area_s_e[n][0] [尾]connect_area_s_e[n][1] [重量]connect_area_s_e[n][2]
extern int Num_track_L;//存储当前被跟踪连通体的个数
extern int Z_min_diff_set;	  //此参数的意义及范围？？？？？？？？？？？							
extern IplImage *pOut01;
extern IplImage *pOut02;
extern int DepthBuf_O[480][640];//原深度图
extern int DepthBuf_O_msy[480][640];//测试参数用深度图
extern IplImage* pCannyImg1;
float AXc = 0, AYc = 0, Xc = 0, Yc = 0, weight_msy= 0;//////每个X与此点质量的乘积之和，每个Y与此点质量的乘积之和，质心X的位置，质心Y的位置,总质量
extern long int center_msy;//abc.h中返回的center,表示（x,y)那点对应在矩阵里的位置
//extern const openni::DepthPixel* pDepth;
char str[50];
extern int pointx;//用于存储鼠标获得的(x,y)坐标
extern int pointy;
/////////////////////////////////MSY TEST////////////////////////////
extern IplImage *showxy_msy;
extern IplImage *showxz_msy;
int pointxy = 1;

//#define showphoto_msy//测试时显示连通域的部分,定义表示关闭,注释掉表示关闭//开启时非常浪费时间,大约6ms处理一帧,关闭时只需0.5ms
//#define mousedebug //重复显示一帧图像, 通过鼠标获取所指连通域的三个筛选值(面积比 长宽比 距离与面积的乘积) 注释掉表示关闭
#define Screenball //是否增加筛选球的一部 注释掉表示关闭,筛选不要和鼠标同时开启
void DrawPoint(int x, int y, unsigned char R, unsigned char G, unsigned char B, IplImage *img)
{
	if (x>img->width)		x = img->width;
	if (x<0)				x = 0;
	if (y>img->height)		y = img->height;
	if (y<0)				y = 0;
	char *	data = img->imageData + (img->width*y + x) * 3;
	*data++ = B;
	*data++ = G;
	*data++ = R;
}


float ball_move_distance(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float distance;
	distance = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) + (z1 - z2)*(z1 - z2));
	return distance;
}

int ball_move_judge(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
{
	float distance1, distance2;
	distance1 = ball_move_distance(x1, y1, z1, x2, y2, z2);
	distance2 = ball_move_distance(x2, y2, z2, x3, y3, z3);
	if (distance1>50 && distance1<500 && distance2>50 && distance2<500)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


float least_square_method(float *posX, float *posY, float *posW, int num)
{
	double Exy = 0.0;
	double Ex = 0.0;
	double Ey = 0.0;
	double Ex2 = 0.0;
	double totalnum = 0.0;
	double a, b;//ax+by+c=0
	double k;
	if (num <= 1)
	{
		return(0);
	}
	for (int ct = 0; ct<num; ct++)
	{
		Exy = Exy + posX[ct] * posY[ct] * posW[ct];
		Ex = Ex + posX[ct] * posW[ct];
		Ey = Ey + posY[ct] * posW[ct];
		Ex2 = Ex2 + posX[ct] * posX[ct] * posW[ct];
		totalnum = totalnum + posW[ct];
	}
	a = totalnum*Exy - Ex*Ey;
	b = totalnum*Ex2 - Ex*Ex;
	//	k=-a/b;
	return (-atan2(a, b));  //返回hu度值
	//	return (-atan2(a,b)*180.0/3.1415926);  //返回角度值
	//	return k;  //返回角度值
}


void clean(IplImage *img)
{
	char *	data = img->imageData;
	for (int i = 0; i<img->width*img->height * 3; i++)
		*data++ = 0;

}
class ballReturnValue
{
public:
    float x;
	float y;
	long  int z;
	
};



long int bool_max_connectivity_analyze2_1_OBJ(ballReturnValue *ballRV)
{
	
	long int center = 0;

	register unsigned long int  boolset = 3000;//阈值

	register unsigned long int  area_grow_data_arel = 0;//纪录currently当前这一圈有几个像素


	int Z_min_diff = Z_min_diff_set;//50;	//这是一个可调值关系到深度的分辨粗略程度
//#ifdef showphoto_msy
//	const char X_step = 2;		//像素点的步长 隔点扫描 用于减少运算量
//	const char Y_step = 2;		//像素点的步长 隔点扫描 用于减少运算量
//#endif

	const char X_step = 1;		//像素点的步长 隔点扫描 用于减少运算量
	const char Y_step = 1;

	const int x320 = DEPTH_VISION_CAM_WIDTH;//320;//320;//处理图像数据的大小设置  x值必须和原数据相同否则出现 行错误 
	const int y240 = DEPTH_VISION_CAM_HEIGHT;//240;//240;//处理图像数据的大小设置
	const int datalong = DEPTH_VISION_CAM_WIDTH*DEPTH_VISION_CAM_HEIGHT;
	const int x320r1 = x320 - 1;//320;//处理图像数据的大小设置  x值必须和原数据相同否则出现 行错误 
	const int y240r1 = y240 - 1;//240;//处理图像数据的大小设置

	register int i, j, k, l, x, y;//seedx,seedy,,used_num

	register int    x_add, x_reduce, y_add, y_reduce, xg, yg, zg, zt, maxW, maxN;



	register int connect_area_num = 1;//记录用到的连通域号	注意:这里的编号 0无效点 1未处理过的有效点 2连通域的第一个编号 3 4 5....
	register int connect_max_area_num = 1;//最大连通域的编号	默认无效值1
	register int connect_area_w = 0;//记录连通域重量值
	register int connect_max_area_w = 0;//记录连通域最大重量值

	register int connect_area_layer_l = 0;//存储连通域 层的 点数 长度
	register int connect_area_layer_s = 0;//存储连通域 层的 首
	register int connect_area_layer_e = 0;//存储连通域 层的 尾
	register int connect_area_layer_pl = 0;//存储连通域 层的 点数 长度
	register int connect_area_layer_ps = 0;//存储连通域 上层的 首
	register int connect_area_layer_pe = 0;//存储连通域 上层的 尾



	Num_track_L = 0;//存储当前被跟踪连通体的个数清零

	////////////////////////////////////
	//黑边处理  防止出现边缘冒了//////       
	for (x = 0; x<x320; x++)
	{
		DepthBuf_O[0][x] = 0;
		DepthBuf_O[1][x] = 0;
		DepthBuf_O[y240r1][x] = 0;
		DepthBuf_O[y240r1 - 1][x] = 0;
	}
	for (y = 0; y<y240; y++)
	{
		DepthBuf_O[y][0] = 0;
		DepthBuf_O[y][1] = 0;
		DepthBuf_O[y][x320r1] = 0;
		DepthBuf_O[y][x320r1 - 1] = 0;
	}
	//黑边处理  防止出现边缘冒了//////


	unsigned int *area_grow_data_are_PO = &area_grow_data_obj[0][0];
	area_grow_data_arel = 0;//所有连通域纪录 位置置零
	//1自动设置这次生长的起点种子
	//for(i=0;i<circle_mask_long;i++)
	//i=0;while(i<datalong)


	//分号策略 如果一个号 跟踪过 一周期就要在使用累加加1 加到一个定值就不加了   如果此号没有点了此号的使用累加清零 跟踪时间越长认为越可靠
	//当前周期参考上周期的分号结果 从跟踪时间长的区域开始先查找
#ifndef WIN01_MOUSE_SET
	for (y = 0; y<DEPTH_VISION_CAM_HEIGHT; y += Y_step)
	{
		for (x = 0; x<DEPTH_VISION_CAM_WIDTH; x += X_step)
		{
#endif
#ifdef WIN01_MOUSE_SET
			for (y = win01_mouse_set_Y; y<win01_mouse_set_Y + 1; y += Y_step)//标定测试用
			{
				for (x = win01_mouse_set_X; x<win01_mouse_set_X + 1; x += X_step)//标定测试用
				{
#endif
					//y=i/DEPTH_VISION_CAM_WIDTH;
					//x=i%DEPTH_VISION_CAM_WIDTH;
					if (DepthBuf_O[y][x]>0)//没生长过 且是有效点
					{
						connect_area_num++;//记录用到的连通域号	注意:这里的编号 0无效点 1未处理过的有效点 2连通域的第一个编号 3 4 5....
						connect_area_w = 0;//记录连通域重量值	清零
						connect_area_s_e_w[connect_area_num][0] = area_grow_data_arel;//存储1000个连通域的 首 尾 标号 [首]connect_area_s_e[n][0] [尾]connect_area_s_e[n][1] [重量]connect_area_s_e[n][2]
						//●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●
						//1设置这次生长的起点种子
						//			area_grow_data_arex[area_grow_data_arel]=x;//当前次生长的一圈的纪录
						//			area_grow_data_arey[area_grow_data_arel++]=y;//当前次生长的一圈的纪录
						*area_grow_data_are_PO++ = y;
						*area_grow_data_are_PO++ = x;
						*area_grow_data_are_PO++ = DepthBuf_O[y][x];
						area_grow_data_arel++;
						connect_area_layer_l = 1;
						connect_area_layer_pe = connect_area_layer_ps + 1;
						//    		area_grow_dataAtpx[0]=x;//seedx;////手动设置种子
						//    		area_grow_dataAtpy[0]=y;//seedy;////手动设置种子
						//    		area_grow_dataAtpl=1;////手动设置种子
						connect_area_w++;//连通域重量+1

						//开始生长
						while (1)//for (i=0;i<1500;i++)//生成生长区域 的生长次数
						{
							connect_area_layer_s = area_grow_data_arel;
							j = connect_area_layer_ps; while (j<connect_area_layer_pe)
							{   //周围不是A中的有效点且符合生长规则
								//    				xg=area_grow_data_arex[j];//当前次生长的一圈的纪录
								//    				yg=area_grow_data_arey[j];//当前次生长的一圈的纪录
								zg = area_grow_data_obj[j][2];//当前次生长的一圈的纪录
								xg = area_grow_data_obj[j][1];//当前次生长的一圈的纪录
								yg = area_grow_data_obj[j][0];//当前次生长的一圈的纪录
								DepthBuf_O_T[yg][xg] = connect_area_num;//一个原有问题 没将种子去掉 会使连通域加1个点
								x_add = xg + X_step;
								x_reduce = xg - X_step;
								y_add = yg + Y_step;
								y_reduce = yg - Y_step;

								zt = DepthBuf_O[y_add][x_reduce];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_add;
									*area_grow_data_are_PO++ = x_reduce;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_add][x_reduce] = -connect_area_num;
								}
								zt = DepthBuf_O[y_add][xg];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_add;
									*area_grow_data_are_PO++ = xg;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_add][xg] = -connect_area_num;
								}
								zt = DepthBuf_O[y_add][x_add];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_add;
									*area_grow_data_are_PO++ = x_add;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_add][x_add] = -connect_area_num;
								}
								zt = DepthBuf_O[yg][x_reduce];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = yg;
									*area_grow_data_are_PO++ = x_reduce;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[yg][x_reduce] = -connect_area_num;
								}
								zt = DepthBuf_O[yg][x_add];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = yg;
									*area_grow_data_are_PO++ = x_add;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[yg][x_add] = -connect_area_num;
								}
								zt = DepthBuf_O[y_reduce][x_reduce];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_reduce;
									*area_grow_data_are_PO++ = x_reduce;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_reduce][x_reduce] = -connect_area_num;
								}
								zt = DepthBuf_O[y_reduce][xg];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_reduce;
									*area_grow_data_are_PO++ = xg;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_reduce][xg] = -connect_area_num;
								}
								zt = DepthBuf_O[y_reduce][x_add];
								if ((abs(zt - zg)<Z_min_diff))//&&(zt>0)
								{
									*area_grow_data_are_PO++ = y_reduce;
									*area_grow_data_are_PO++ = x_add;
									*area_grow_data_are_PO++ = zt;
									area_grow_data_arel++;
									DepthBuf_O[y_reduce][x_add] = -connect_area_num;////////////////////////////////////////////改区域生长 加参数
								}
								j++;
							}
							connect_area_layer_e = area_grow_data_arel;
							connect_area_layer_l = connect_area_layer_e - connect_area_layer_s;//连通域重量+当前一圈的点数
							connect_area_layer_ps = connect_area_layer_s;//存储连通域 上层的 首
							connect_area_layer_pe = connect_area_layer_e;//存储连通域 上层的 尾

							connect_area_w += connect_area_layer_l;
							if (connect_area_layer_l == 0)
								break;


						}


						//●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●
						connect_area_s_e_w[connect_area_num][1] = area_grow_data_arel;//存储1000个连通域的 首 尾 标号 [首]connect_area_s_e[n][0] [尾]connect_area_s_e[n][1] [重量]connect_area_s_e[n][2]
						connect_area_s_e_w[connect_area_num][2] = connect_area_w;//存储1000个连通域的 首 尾 标号 [首]connect_area_s_e[n][0] [尾]connect_area_s_e[n][1] [重量]connect_area_s_e[n][2]
						if (connect_max_area_w<connect_area_w)
						{
							connect_max_area_w = connect_area_w;
							connect_max_area_num = connect_area_num;
						}



					}


				}
			}

			Num_track_L = Num_track_L;//


			int t;
			unsigned char R, G, B;
#ifdef mousedebug
			int maosdisplaynumber = -DepthBuf_O[pointy][pointx];
#endif

			if (1)//所有连通域 逐个显示
			{
				register int   min_x, max_x, min_y, max_y;
				i = 1;
#ifdef showphoto_msy
					//clean(pOut01);//清理两个图
					clean(pOut02);
					//////清理右上角的黑图
					int x, y;
					for (x = 0; x < 320; x++)
					{
						for (y = 0; y < 240; y++)
						{
							dataShow[(y * 320 + x) * 3] = 0;
							dataShow[(y * 320 + x) * 3 + 1] = 0;
							dataShow[(y * 320 + x) * 3 + 2] = 0;
						}
					}
#endif
			  /////////

				for (j = 2; j <= connect_area_num; j++)//j 连通域的个数
				{
					R = (i * 90 % 200) + 40;
					G = (i * 50 % 200) + 55;
					B = (i * 30 % 200) + 55;//给联通域随机赋颜色   		
					if (1)
					{
						//	if ((connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0])> 100 && (connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0])<3300)
						//if ((connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0]) > 100 && (connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0]) < 3300)
							if ((connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0]) > 70 && (connect_area_s_e_w[j][1] - connect_area_s_e_w[j][0]) <3300)
							//联通域大小筛选   100<  <3300
						{	////////此处参数可调(联通)


							min_x = 640;
							max_x = 0;
							min_y = 480;
							max_y = 0;
							//////////////////////
							weight_msy = 0;
							AXc = 0;
							AYc = 0;
							/////////////////
							for (i = connect_area_s_e_w[j][0]; i<connect_area_s_e_w[j][1]; i++)
							{
								if (min_x>area_grow_data_obj[i][1])
									min_x = area_grow_data_obj[i][1];
								if (max_x<area_grow_data_obj[i][1])
									max_x = area_grow_data_obj[i][1];
								if (min_y>area_grow_data_obj[i][0])
									min_y = area_grow_data_obj[i][0];
								if (max_y < area_grow_data_obj[i][0])
									max_y = area_grow_data_obj[i][0];

								/////////////////////////////////////////////////////////////////////////////////////////
								AXc = AXc + area_grow_data_obj[i][1] * DepthBuf_O_msy[area_grow_data_obj[i][0]][area_grow_data_obj[i][1]];
								AYc = AYc + area_grow_data_obj[i][0] * DepthBuf_O_msy[area_grow_data_obj[i][0]][area_grow_data_obj[i][1]];
								weight_msy = weight_msy + DepthBuf_O_msy[area_grow_data_obj[i][0]][area_grow_data_obj[i][1]];
								/////////////////////////////////////////////////////////////////////////////

							}
							float P_xy, area, P_area;   //P_xy为长宽比例
							if (abs(max_x - min_x) > abs(max_y - min_y))
								P_xy = (float)abs(max_y - min_y) / (float)abs(max_x - min_x);
							else
								P_xy = (float)abs(max_x - min_x) / (float)abs(max_y - min_y);
							//测量 图形面积 与 最小矩面积 比
							area = abs((max_y - min_y)*(max_x - min_x));
							if (area != 0)
								P_area = connect_area_s_e_w[j][2] * X_step*Y_step / area;
							else
								P_area = 0;
							////////////////////////////////////////////////////////////////////////////////////////////////////////////




							Xc = AXc / weight_msy;
							Yc = AYc / weight_msy;
							/////////////////////////////////////////////////////////////////////////////////////////////////
							//if ((((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)))>90000) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)))<110000))

							//if ((fabs(P_xy - 1) < 1 * 0.15) && (fabs(P_area - 0.7854) < 0.7854*0.15) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) > 90000) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) < 110000))
							//	if ((fabs(P_xy - 1) < 1 * 0.15) && (fabs(P_area - 0.7854) < 0.7854*0.15) )//&& (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) > 90000) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) < 110000))
							//if (fabs(P_xy - 1) < 1 * 0.15) && (fabs(P_area - 0.7854) < 0.7854*0.15))//&& (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) > 90000) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) < 110000))
#ifdef Screenball
							if ((fabs(P_xy - 1) < 1 * 0.18) && (fabs(P_area - 0.7854) < 0.7854*0.18) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) > 57000) && (((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))) < 73000))
							{	//圆的长宽比为1  圆的面积与外接最小矩型面积比为 2*r*2*r/Pi*r*r =4/pi 	
#endif
								//cout << "长宽比" << P_xy << " 面积比" << P_area << " 实际面积 " << connect_area_s_e_w[j][2] * X_step*Y_step << "中心点距离" << DepthBuf_O_msy[(max_y + min_y)/2][(max_x + min_x) / 2] << endl;
								/////////////////////////////////////////////////////以上内容输出长宽比，面积比，实际面积，中心距离////////////////////////////
#ifdef mousedebug
								if (1)
								{
#endif
									for (i = connect_area_s_e_w[j][0]; i < connect_area_s_e_w[j][1]; i++)
									{
#ifdef showphoto_msy
										DrawPoint(area_grow_data_obj[i][1], area_grow_data_obj[i][0], R, G, 0, pOut02);//SetColor(y,x,cPoint);//测试代码

										if (area_grow_data_obj[i][1] % 2 == 0 && area_grow_data_obj[i][0] % 2 == 0)
										{
											//因为右上角的图只有320*240, 所以隔一个点一画图
											dataShow[((240 - area_grow_data_obj[i][0] / 2) * 320 + area_grow_data_obj[i][1] / 2) * 3] = R;
											dataShow[((240 - area_grow_data_obj[i][0] / 2) * 320 + area_grow_data_obj[i][1] / 2) * 3 + 1] = G;
											dataShow[((240 - area_grow_data_obj[i][0] / 2) * 320 + area_grow_data_obj[i][1] / 2) * 3 + 2] = B;
										}
#endif
										
										ballRV->x= Xc;
										ballRV->y = Yc;
										ballRV->z= DepthBuf_O_msy[(int)Yc][(int)Xc];
										//center = DepthBuf_O_msy[(int)Yc][(int)Xc];
									}

#ifdef mousedebug
								}
#endif
#ifdef Screenball
							}
#endif
								///////////////////////////////把坐标等标记在图上MSY///////////////////////////////////////////
#ifdef mousedebug
							if (j == maosdisplaynumber)
							{
								CvFont font;
								cvInitFont(&font, 5,0.6f, 0.6f,0, 1, CV_AA);
									
								sprintf(str, "P_a %.3f ", P_area);
								cvPutText(pOut02, str, cvPoint(min_x + 10, min_y + 10), &font, CV_RGB(255, 255, 255));
								sprintf(str, "ra  %.3f  ", P_xy);
								cvPutText(pOut02, str, cvPoint(min_x + 10, min_y + 30), &font, CV_RGB(255, 255, 255));
								sprintf(str, "  end  %f  ",  ((float)(weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step))));// DepthBuf_O_msy[(int)Yc][(int)Xc]);
								cvPutText(pOut02, str, cvPoint(min_x + 10, min_y + 50), &font, CV_RGB(255, 255, 255));
							}
#endif
								//								sprintf(str, "  end  %f  ", weight_msy / (float)((float)connect_area_s_e_w[j][2])* sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)));// DepthBuf_O_msy[(int)Yc][(int)Xc]);

/*
								//								sprintf(str, "  Figure area  %d  ", connect_area_s_e_w[j][2] * X_step*Y_step);
								//								
								//								sprintf(str, "  depth  %f  ", weight_msy / (float)connect_area_s_e_w[j][2]);// DepthBuf_O_msy[(int)Yc][(int)Xc]);
								//								cvPutText(pOut02, str, cvPoint(min_x + 10, min_y + 70), &font, CV_RGB(255, 255, 255));
								////								
								//								cvPutText(pOut02, str, cvPoint(min_x + 10, min_y + 90), &font, CV_RGB(255, 255, 255));
								//////////////////////////////////////////////////////////////MSY TEST////////////////////////////
								/*	CvPoint   centerpointxy;
									centerpointxy.x = (int)Xc;
									centerpointxy.y = (int)Yc;
									cvCircle(showxy_msy, centerpointxy, 3, CV_RGB(0, 0, 0), 1, 8, 3);

									CvPoint   centerpointxz;
									centerpointxz.x = (int)Xc;
									centerpointxz.y = DepthBuf_O_msy[(int)Yc][(int)Xc] / 12;
									cvCircle(showxz_msy, centerpointxz ,3, CV_RGB(0, 0, 0), 1, 8, 3);*/


								//DrawPoint((int)Xc, (int)Yc, 0, 255, 255, showxy_msy);
								//cout << (max_y + min_y) / 2<<"     "<<(max_x + min_x) / 2 <<"     "<<Yc<<"       "<<Xc << endl;
								//cout  << " 实际面积 " << connect_area_s_e_w[j][2] * X_step*Y_step << "中心点距离" << DepthBuf_O_msy[(int)Yc][(int)Xc] << endl;
								/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								/*if ( ((weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)) >= 80000) && ((weight_msy / (float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)) < 150000))
								{
								weight_msy / (float)((float)connect_area_s_e_w[j][2])*sqrt((connect_area_s_e_w[j][2] * X_step*Y_step)
								}*/
								}
							
						/*}*/
					
					}
				}
//				cvCvtColor(pOut02, pCannyImg1, CV_RGB2GRAY);
//				cvCanny(pCannyImg1, pCannyImg1, 50, 100, 3);////////@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@此处参数可调
				//	cvShowImage("canny1", pCannyImg1);
#ifndef WIN01_MOUSE_SET
				//cvShowImage("win01", pOut01);
				//cvShowImage("win02", pOut02);//将处理完的输出
				/*cvShowImage("msy_xy", showxy_msy);
			    cvShowImage("msy_xz", showxz_msy);*/
#endif
			}




			x = x;

			//带出黑白图象重心横坐标  obj_properties *All  All.x结果的重心的x坐标 All.y结果的重心的y坐标
			//需要处理部分的掩膜数组  int *yan_mo_x,int *yan_mo_y
			//需要处理部分的掩膜数组长度int yan_mo_long,
			register float OBJ_temp_b = 0;
			register float OBJ_temp_b_ = 0;
			register float OBJ_temp_c = 0;
			register float ball_op_x = 0;
			register float ball_op_y = 0;
			register float ball_op_weight = 0;
			if (connect_max_area_num>1)//最大连通域的编号	默认无效值1
			{
				for (i = connect_area_s_e_w[connect_max_area_num][0]; i<connect_area_s_e_w[connect_max_area_num][1]; i++)
				{//	SetColor(area_grow_data_arey[i],area_grow_data_arex[i],cPointB);
					OBJ_temp_c++;
					OBJ_temp_b += area_grow_data_obj[i][1];//x
					OBJ_temp_b_ += area_grow_data_obj[i][0];//y


				}
			}

			if (OBJ_temp_c != 0)  //保险程序  当同步不佳时  有时Capture不到图象 就会出现所有象素都为零 这时做OBJ_temp_b / OBJ_temp_c的除法就会产生分母为零的错误
			{
				ball_op_x = OBJ_temp_b / OBJ_temp_c;//(int)(OBJ_temp_b / OBJ_temp_c);  //除以1+2+3+...+320
				ball_op_y = OBJ_temp_b_ / OBJ_temp_c;//(int)(OBJ_temp_b / OBJ_temp_c);  //除以1+2+3+...+240
			}
			ball_op_weight = OBJ_temp_c;//准备把象素重量传给主逻辑函数
			ball_op_y = ball_op_y;
			ball_op_x = ball_op_x;
			ball_op_weight = ball_op_weight;

			//完成二值化图像标重心程序///////////////////////////////////////////////////////////////////

			//兼容鑫鑫 回存连同域数据
			//memset(DepthBuf_O, 0, DEPTH_VISION_CAM_HEIGHT*DEPTH_VISION_CAM_WIDTH*sizeof(int));





			
			//if (0)// display_process_image == 1)//判断 显示过程图像	//显示最大连通域
			//for (y = 0; y<DEPTH_VISION_CAM_HEIGHT; y++)
			//for (x = 0; x<DEPTH_VISION_CAM_WIDTH; x++)
			//{
			//	if (DepthBuf_O[y][x] == 1)
			//		DrawPoint(x, y, 0, 255, 255, pOut01);//SetColor(y,x,cPoint);//测试代码
			//	else
			//		DrawPoint(x, y, 0, 0, 0, pOut01);//SetColor(y,x,cPoint);//测试代码

			//}

			////			cvShowImage("win01", pOut01);
			
			return 0;
		}

		// area_grow_data_obj[当前自动分配连通域号][0 1 2]=y x z   trackobj[跟踪物体号]= 当前自动分配连通域号
		// 跟踪物体号 分配机制 一个物体分到一个号 当前还分这个号 如果一个物体新出的就新分个最后的号 如果一个物体号一个点都没有了 这个号释放并把其他后面的号前移
		// 程序告知 外面程序 每个跟踪物体 当前号是多少 上周期号是多少



		/*void CheckOpenNIError(XnStatus result, string status)
		{
			if (result != XN_STATUS_OK)
				cerr << status << " Error: " << xnGetStatusString(result) << endl;
		}*/








