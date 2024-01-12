#include "Framework.h"
#include "Camera.h"

Camera::Camera()
{
	D3DXMatrixIdentity(&matRotation);
	D3DXMatrixIdentity(&matView);

	Rotation();
	Move();
}

Camera::~Camera()
{

}

void Camera::Position(float x, float y, float z)
{
	Vector3 vec(x, y, z);
	Position(vec);
}

void Camera::Position(Vector3 & vec)
{
	position = vec;

	Move();
}

void Camera::Position(Vector3 * vec)
{
	*vec = position;
}

void Camera::Rotation(float x, float y, float z)
{
	Vector3 vec(x, y, z);
	Rotation(vec);
}

void Camera::Rotation(Vector3 & vec)
{
	rotation = vec;

	Rotation();
}

void Camera::Rotation(Vector3 * vec)
{
	*vec = rotation;
}

void Camera::RotationDegree(float x, float y, float z)
{
	Vector3 vec(x, y, z);
	RotationDegree(vec);
}

void Camera::RotationDegree(Vector3 & vec)
{
	//rotation = vec * Math::PI / 180.0f;
	rotation = vec * 0.01745328f;

	Rotation();
}

void Camera::RotationDegree(Vector3 * vec)
{
	//*vec = rotation * 180.0f / Math::PI;
	*vec = rotation * 57.29577957f; // constant : 180.0f / Math::PI;
}

void Camera::GetMatrix(Matrix * matrix)
{
	//*matrix = matView;
	memcpy(matrix, &matView, sizeof(Matrix));
}

void Camera::Rotation()
{
	Matrix X, Y, Z;
	D3DXMatrixRotationX(&X, rotation.x);
	D3DXMatrixRotationY(&Y, rotation.y);
	D3DXMatrixRotationZ(&Z, rotation.z);

	matRotation = X * Y * Z;

	Vector3 f(0, 0, 1);
	Vector3 u(0, 1, 0);
	Vector3 l(1, 0, 0);

	D3DXVec3TransformNormal(&forward, &f, &matRotation);
	D3DXVec3TransformNormal(&up, &u, &matRotation);
	D3DXVec3TransformNormal(&right, &l, &matRotation);
}

void Camera::Move()
{
	View();
}

void Camera::View()
{
	const Vector3&& lookat = position + forward;
	D3DXMatrixLookAtLH(&matView, &position, &lookat, &up);
}
