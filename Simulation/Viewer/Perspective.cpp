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
	//Aspect : ����...
	// 1��1�� �Ǹ� �þ߰��� 90���� �Ǽ� ���� �������� �ȴ�.
	aspect = width / height;
	D3DXMatrixPerspectiveFovLH(&matrix, fov, aspect, zn, zf);
}
