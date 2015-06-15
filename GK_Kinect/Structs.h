#if !defined(HOME_STRUCTS_INCLUDED)
#define HOME_STRUCTS_INCLUDED

#include <vector>
using namespace std;

#define DEPTH_WIDTH		512
#define DEPTH_HEIGHT	424

#define COLOR_WIDTH		1920
#define COLOR_HEIGHT	1080

typedef struct st_2DPoint
{
	long x;
	long y;
}tdPoint;

typedef struct st_trPoint
{
	float x;
	float y;
	float z;
}trPoint;

typedef struct st_trVector
{
	double x;
	double y;
	double z;
}trVector;

//自定义3D数据格式
typedef struct stWP_K_3D_Object
{
	unsigned char srcRGB[COLOR_WIDTH*COLOR_HEIGHT * 3];
	unsigned short depthD16[DEPTH_WIDTH*DEPTH_HEIGHT];
	
	int DepthToColorX[DEPTH_WIDTH*DEPTH_HEIGHT];
	int DepthToColorY[DEPTH_WIDTH*DEPTH_HEIGHT];
	tdPoint ColorToDepth[COLOR_WIDTH*COLOR_HEIGHT];

	bool en[DEPTH_WIDTH][DEPTH_HEIGHT];
	float x[DEPTH_WIDTH][DEPTH_HEIGHT];
	float y[DEPTH_WIDTH][DEPTH_HEIGHT];
	float z[DEPTH_WIDTH][DEPTH_HEIGHT];
	unsigned char R[DEPTH_WIDTH][DEPTH_HEIGHT];
	unsigned char G[DEPTH_WIDTH][DEPTH_HEIGHT];
	unsigned char B[DEPTH_WIDTH][DEPTH_HEIGHT];
}WP_K_3D_Object;

//物体描述
typedef struct trObject
{
	float nCenterX;	//中心点坐标
	float nCenterY;
	float nCenterZ;

	float nWidthX;	//尺寸
	float nWidthY;
	float nWidthZ;

	int nBlobIndex;	//在Blob数组中对应的index
	int nPointNum;	//点数

	float nMaxX;	//点云X最大值
	float nMinX;	//点云X最小值
	float nMaxY;	//点云Y最大值
	float nMinY;	//点云Y最小值
	float nMaxZ;	//点云Z最大值
	float nMinZ;	//点云Z最小值

	tdPoint pntDepthLeft;
	tdPoint pntDepthRight;
	tdPoint pntDepthTop;
	tdPoint pntDepthBottom;

	bool bEn;	//标记

	string strObjectName;
}trObject;

#endif // !defined(HOME_STRUCTS_INCLUDED)