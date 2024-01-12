#include "Framework.h"
#include "Viewport.h"

Viewport::Viewport(float width, float height, float x, float y, float minDepth, float maxDepth)
{
	Set(width, height, x, y, minDepth, maxDepth);
}

Viewport::~Viewport()
{
	
}

void Viewport::RSSetViewport()
{
	D3D::GetDC()->RSSetViewports(1, &viewport);
}

void Viewport::Set(float width, float height, float x, float y, float minDepth, float maxDepth)
{
	viewport.TopLeftX = this->x = x;
	viewport.TopLeftY = this->y = y;
	viewport.Width = this->width = width;
	viewport.Height = this->height = height;
	viewport.MinDepth = this->minDepth = minDepth;
	viewport.MaxDepth = this->maxDepth = maxDepth;

	RSSetViewport();
}

void Viewport::Project(Vector3 * pOut, Vector3 & source, Matrix & W, Matrix & V, Matrix & P)
{
	// W -> V -> P -> Vp
	Vector3 position = source;

	Matrix wvp = W * V * P;
	// x -1 ~ 1
	// y -1 ~ 1
	// z 0 ~ 1
	D3DXVec3TransformCoord(pOut, &position, &wvp);


	// 0 ~ 1280
	// 0 ~ 720 �� ȭ��������� �ٲ��ֱ�	
	// ~1 ~ 1 (+1.0f)-> 0 ~ 2 ( * 0.5) -> 0 ~ 1 (*width)
	pOut->x = ((pOut->x + 1.0f) * 0.5f) * width + x;
	pOut->y = ((-pOut->y + 1.0f) * 0.5f) * height + y;
	// depth
	// 0 ~ 1 ( * ���̰Ÿ� ) -> (���̰Ÿ�) (*������ ���� �ּҰŸ�) = �� ���� )
	pOut->z = (pOut->z * (maxDepth - minDepth)) + minDepth;

}

void Viewport::Unproject(Vector3 * pOut, Vector3 & source, Matrix & W, Matrix & V, Matrix & P)
{
	// Vp -> P -> V -> W

	// ȭ�� ��ǥ�� ���� ��ǥ�� ��ȯ
	Vector3 position = source;

	// 0 ~ 1280 -> -1 ~ 1
	// 0 ~ 720 -> -1 ~ 1
	pOut->x = ((position.x - x) / width) * 2.0f - 1.0f; // -1 ~ 1
	pOut->y = (((position.y - y) / height) * 2.0f - 1.0f) * -1.0f; // -1 ~ 1
	pOut->z = ((position.z - minDepth) / (maxDepth - minDepth)); // -1 ~ 1

	Matrix wvp = W * V * P;
	D3DXMatrixInverse(&wvp, NULL, &wvp);

	D3DXVec3TransformCoord(pOut, pOut, &wvp);
}
