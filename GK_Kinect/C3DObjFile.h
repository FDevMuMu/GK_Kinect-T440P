#pragma once
#include "Structs.h"

class C3DObjFile
{
public:
	C3DObjFile(void);
	~C3DObjFile(void);
	void Save3DObjFile(CString inFileName, WP_K_3D_Object* inp3DObj,int inNum);
	int Load3DObjFile(CString inFileName, WP_K_3D_Object* inArObjects, int inMax);
};

