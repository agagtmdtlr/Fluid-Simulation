#include "Framework.h"
#include "Perspective.h"

Perspective::Perspective(float width, float height, float zn, float zf, float fov)
	: Projection(width, height, zn, zf, fov)
{
	Set(width, height, zn, zf, fov);
}

Perspective::~Perspective()
{
}

void Perspective::Set(float width, float height, float zn, float zf, float fov)
{
	Super::Set(width, height, zn, zf, fov);
	//Aspect : 비율...
	// 1대1이 되면 시야각이 90도가 되서 거의 정투영이 된다.
	aspect = width / height;
	D3DXMatrixPerspectiveFovLH(&matrix, fov, aspect, zn, zf);
}
