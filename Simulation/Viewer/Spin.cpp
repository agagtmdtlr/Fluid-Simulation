#include "Framework.h"
#include "Spin.h"

Spin::Spin()
	: Camera()
{

}

Spin::~Spin()
{

}

void Spin::Update()
{
	Vector3 f = Forward();
	Vector3 u = Up();
	Vector3 r = Right();

	//Move
	{
		Vector3 P;
		Position(&P);

		Matrix Yaw;
		D3DXMatrixRotationY(&Yaw, Math::ToRadian(10.0f * Time::Delta()));

		Matrix Pitch;
		D3DXMatrixIdentity(&Pitch);
		Vector3 mouse = Mouse::Get()->GetMoveValue();
		if (Mouse::Get()->Press(1) == true)
		{
			D3DXMatrixRotationX(&Pitch, -mouse.y * rotation * Time::Delta());
		}

		Matrix Rot = Yaw * Pitch;

		Vector3 invF = -f;
		Vector4 newInvF;
		D3DXVec3Transform(&newInvF, &invF, &Rot);
		f.x = -newInvF.x;
		f.y = -newInvF.y;
		f.z = -newInvF.z;

		bool upcheck = (f.y > 1 - 1e-6f || f.y < -1 + 1e-6f);
		u = upcheck ? Vector3(0, 0, f.y) : Vector3(0, 1, 0);
		D3DXVec3Cross(&r, &u, &f);
		D3DXVec3Normalize(&r, &r);
		D3DXVec3Cross(&u, &f, &r);
		D3DXVec3Normalize(&u, &u);

		Forward(f);
		Up(u);
		Right(r);

		distance += -mouse.z * 2.0f * Time::Delta();
		Math::Clamp(distance, 5.0f, 50.0f);
		P = center + (-f * distance);
		Position(P); // update poisiton and view matrix by eye and look;
	}
}

