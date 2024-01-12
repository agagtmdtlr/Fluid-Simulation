#pragma once
#include "Camera.h"

class Spin : public Camera
{
public:
	Spin();
	~Spin();

	void Update() override;

	void Center(const Vector3 &  center)
	{
		this->center = center; 
	};

	void Distance(const float & distance)
	{
		this->distance = distance;
	};

	void Speed(const float & move, const float & rotation)
	{
		this->move = move;
		this->rotation = rotation;
	}

private:
	Vector3 center = { 0,0,0 };
	float distance = 15.0f;
	float rotation = 1.0f;
	float move = 20.0f;
};