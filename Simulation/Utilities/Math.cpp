#include "Framework.h"
#include "Math.h"

const float Math::PI = 3.14159265f;
const float Math::EPSILON = 0.0000001f;

float Math::Modulo(float val1, float val2)
{
	while (val1 - val2 >= 0)
		val1 -= val2;

	return val1;
}

float Math::Volume(const Vector3 & v)
{
	return abs(v.x) * abs(v.y) * abs(v.z);
}

float Math::ToRadian(float degree)
{
	return degree * PI / 180.0f;
}

float Math::ToDegree(float radian)
{
	return radian * 180.0f / PI;
}

Vector3 Math::ToRadian(Vector3 vec)
{
	return Vector3(ToRadian(vec.x),ToRadian(vec.y),ToRadian(vec.z));
}

Vector3 Math::ToDegree(Vector3 vec)
{
	return Vector3(ToDegree(vec.x),ToDegree(vec.y),ToDegree(vec.z));
}

Quaternion Math::ToQuaternion(Vector3 vec)
{
	float yaw = vec.y;
	float pitch = vec.x;
	float roll = vec.z;
	// Abbreviations for the various angular functions
	double cy = cos(yaw * 0.5);
	double sy = sin(yaw * 0.5);
	double cp = cos(pitch * 0.5);
	double sp = sin(pitch * 0.5);
	double cr = cos(roll * 0.5);
	double sr = sin(roll * 0.5);

	Quaternion q;
	q.w = (FLOAT)(cr * cp * cy + sr * sp * sy);
	q.x = (FLOAT)(sr * cp * cy - cr * sp * sy);
	q.y = (FLOAT)(cr * sp * cy + sr * cp * sy);
	q.z = (FLOAT)(cr * cp * sy - sr * sp * cy);

	return q;
}

Vector3 Math::ToEuler(Quaternion q)
{
	Vector3 angles;
	// roll
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.z = (float)std::atan2(sinr_cosp, cosr_cosp);
	
	//pitch
	double sinp = 2 * (q.w * q.y - q.z * q.x);
	if (abs(sinp) >= 1)
		angles.x = (float)copysign(Math::PI / 2.0, sinp); // use 90 degrees if out of range
	else
		angles.x = (float)asin(sinp);

	// yaw
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.y = (float)atan2(siny_cosp, cosy_cosp);

	return angles;
}

bool Math::IsPrime(UINT N)
{ 
	if (N <= 1) return false;
	if (N <= 3) return true;

	if (N % 2 == 0 || N % 3 == 0) return false;

	for (UINT i = 5; i * i <= N; i = i + 6)
	{
		if (N%i == 0 || N % (i + 2) == 0)
			return false;
	}
	return true;
}

UINT Math::NextPrime(UINT N)
{
	if (N <= 1)
		return 2;

	UINT prime = N;
	bool found = false;

	while (!found)
	{
		prime++;
		if (IsPrime(prime))
		{
			found = true;
		}
	}
	return prime;
}

float Math::Random(float r1, float r2)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = r2 - r1;
	float val = random * diff;

	return r1 + val;
}

D3DXVECTOR2 Math::RandomVec2(float r1, float r2)
{
	D3DXVECTOR2 result;
	result.x = Random(r1, r2);
	result.y = Random(r1, r2);

	return result;
}

D3DXVECTOR3 Math::RandomVec3(float r1, float r2)
{
	D3DXVECTOR3 result;
	result.x = Random(r1, r2);
	result.y = Random(r1, r2);
	result.z = Random(r1, r2);

	return result;
}

D3DXVECTOR4 Math::RandomVec4(float r1, float r2)
{
	D3DXVECTOR4 result;
	result.x = Random(r1, r2);
	result.y = Random(r1, r2);
	result.z = Random(r1, r2);
	result.w = Random(r1, r2);

	return result;
}

D3DXCOLOR Math::RandomColor3()
{
	D3DXCOLOR result;
	result.r = Math::Random(0.0f, 1.0f);
	result.g = Math::Random(0.0f, 1.0f);
	result.b = Math::Random(0.0f, 1.0f);
	result.a = 1.0f;

	return result;
}

D3DXCOLOR Math::RandomColor4()
{
	D3DXCOLOR result;
	result.r = Math::Random(0.0f, 1.0f);
	result.g = Math::Random(0.0f, 1.0f);
	result.b = Math::Random(0.0f, 1.0f);
	result.a = Math::Random(0.0f, 1.0f);

	return result;
}

float Math::Clamp(float value, float min, float max)
{
	value = value > max ? max : value;
	value = value < min ? min : value;

	return value;
}

float Math::Lerp(float value1, float value2, float t)
{
	//return (1- t) * value1 + t * value2;
	return value1 + (value2 - value1 ) * t;
}

double Math::Lerp(double value1, double value2, double t)
{
	//return (1- t) * value1 + t * value2;
	return value1 + (value2 - value1) * t;
}

void Math::LerpMatrix(OUT D3DXMATRIX & out, const D3DXMATRIX & m1, const D3DXMATRIX & m2, float amount)
{
	out._11 = m1._11 + (m2._11 - m1._11) * amount;
	out._12 = m1._12 + (m2._12 - m1._12) * amount;
	out._13 = m1._13 + (m2._13 - m1._13) * amount;
	out._14 = m1._14 + (m2._14 - m1._14) * amount;

	out._21 = m1._21 + (m2._21 - m1._21) * amount;
	out._22 = m1._22 + (m2._22 - m1._22) * amount;
	out._23 = m1._23 + (m2._23 - m1._23) * amount;
	out._24 = m1._24 + (m2._24 - m1._24) * amount;

	out._31 = m1._31 + (m2._31 - m1._31) * amount;
	out._32 = m1._32 + (m2._32 - m1._32) * amount;
	out._33 = m1._33 + (m2._33 - m1._33) * amount;
	out._34 = m1._34 + (m2._34 - m1._34) * amount;

	out._41 = m1._41 + (m2._41 - m1._41) * amount;
	out._42 = m1._42 + (m2._42 - m1._42) * amount;
	out._43 = m1._43 + (m2._43 - m1._43) * amount;
	out._44 = m1._44 + (m2._44 - m1._44) * amount;
}

D3DXQUATERNION Math::LookAt(const D3DXVECTOR3 & origin, const D3DXVECTOR3 & target, const D3DXVECTOR3 & up)
{
	D3DXVECTOR3 f = (origin - target);
	D3DXVec3Normalize(&f, &f);

	D3DXVECTOR3 s;
	D3DXVec3Cross(&s, &up, &f);
	D3DXVec3Normalize(&s, &s);

	D3DXVECTOR3 u;
	D3DXVec3Cross(&u, &f, &s);

	float z = 1.0f + s.x + u.y + f.z;
	float fd = 2.0f * sqrtf(z);

	D3DXQUATERNION result;

	if (z > Math::EPSILON)
	{
		result.w = 0.25f * fd;
		result.x = (f.y - u.z) / fd;
		result.y = (s.z - f.x) / fd;
		result.z = (u.x - s.y) / fd;
	}
	else if (s.x > u.y && s.x > f.z)
	{
		fd = 2.0f * sqrtf(1.0f + s.x - u.y - f.z);
		result.w = (f.y - u.z) / fd;
		result.x = 0.25f * fd;
		result.y = (u.x + s.y) / fd;
		result.z = (s.z + f.x) / fd;
	}
	else if (u.y > f.z)
	{
		fd = 2.0f * sqrtf(1.0f + u.y - s.x - f.z);
		result.w = (s.z - f.x) / fd;
		result.x = (u.x - s.y) / fd;
		result.y = 0.25f * fd;
		result.z = (f.y + u.z) / fd;
	}
	else
	{
		fd = 2.0f * sqrtf(1.0f + f.z - s.x - u.y);
		result.w = (u.x - s.y) / fd;
		result.x = (s.z + f.x) / fd;
		result.y = (f.y + u.z) / fd;
		result.z = 0.25f * fd;
	}

	return result;
}

int Math::Random(int r1, int r2)
{
	return (int)(rand() % (r2 - r1 + 1)) + r1;
}

float Math::Gaussian(float val, UINT blurCount)
{
	float a = 1.0f / sqrtf(2 * PI * (float)blurCount * (float)blurCount);
	float c = 2.0f * (float)blurCount * (float)blurCount;
	float b = exp(-(val * val) / c);

	return a * b;
}

void Math::MatrixDecompose(const D3DXMATRIX & m, OUT Vector3 & S, OUT Vector3 & R, OUT Vector3 & T)
{
	D3DXQUATERNION rotation;
	D3DXMatrixDecompose(&S, &rotation, &T, &m);

	D3DXMATRIX temp;
	D3DXMatrixRotationQuaternion(&temp, &rotation);

	R.x = asin(-temp._32);
	R.y = atan2(temp._31, temp._33);
	R.z = atan2(temp._12, temp._22);
}
