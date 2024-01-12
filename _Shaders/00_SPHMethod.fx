#define _SPH_METHOD_FX_

#ifndef _CONSTATNT_FX_
	#include "00_Constant.fx"
#endif


///////////////////////////////////// 커널 함수 ////////////////////////////////////////////////////////////////////////////
cbuffer KernelCubicConstant
{
	
	float cubicSpline_R1 = 10 / 7 * pi;
	float cubicSpline_1R2 = 10 / 7 * pi;
	float cubicSpline_2R = 0;

	float cubicSplineConst2 = 10 / 28 * pi;
    
};


float kernelCubicSpline(in float r, in float h)
{
	float result;
	float R = r / h;
	float invH2 = 1 / (h * h);
	float c;
	if (R > 2)
	{
		result = cubicSpline_2R;
	}
	else if (R < 1)
	{
		float R2 = R * R;
		float R3 = R2 * R;
		c = cubicSpline_R1 * invH2;
		result = c * (1 - 1.5 * R2 + 0.75 * R3); // c * ( 1 - 3.2*R2 + 3/4*R3 )
	}
	else
	{
        c = cubicSpline_1R2 * invH2;
		result = c * pow(2 - R, 3);	
	}

	return result;
}

// particle-based fluid simulation reference;
float kernelGrid(in float q)
{
	if (q >= 2)
	{
		return 0;
	}
	float q2 = q * q;
	float q3 = q2 * q;

	if (0 <= q < 1)
	{
		return 2 / 3 - q2 + 1 / 2 * q3;
	}
	else // 1 <= q < 2
	{
		return 1 / 6 * pow(2 - q, 3);
	}
};


// lagrangian fluid dynamics
// point-based fluid kernel function

float W_Default(in float r, in float h)
{    
    float W;
    if( r <=  h)
    {       
        W = 315 / (64 * pi * pow(h, 9)) * pow(h * h - r * r, 3);
    }
    else
    {
        W = 0;
    }
    return W;
}

float W_Default_Gradient(in float r, in float h)
{
    float W = -945 * r / (32 * pi * pow(h, 9)) * pow(h * h - r * r, 2);
    return W;
}

float W_Default_Laplacian(in float r, in float h)
{
    float h2 = h * h;
    float r2 = r * r;
    float W = -945 / (32 * pi * pow(h, 9)) * (h2 - r2) * (3 * h2 - 7 * r2);
    return W;
}

float W_Pressure(in float r, in float h)
{
    float W;
    if (r <= h)
    {
        W = 15 / (pi * pow(h, 6)) * pow(h - r, 3);
    }
    else
    {
        W = 0;
    }
    return W;
}

float W_Pressure_Gradient(in float r, in float h)
{
    
    float W;
    W = -45 / (pi * pow(h, 6)) * sign(r) * pow(h - abs(r), 2);
    return W;
    // r -> -0 = 45/( pi * h^6 )
    // r -> +0 = 45/( pi * h^6 )
}

float W_Pressure_Laplacian(in float r, in float h)
{
    float W;
    W = -90 / (pi * pow(h, 6)) * (1 / r) * (h - r) * (h - 2 * r);
    return W;
    // r -> 0 = -infinite
}

///////////////////////////////////// 해쉬 ////////////////////////////////////////////////////////////////////////////

uint hash(in float3 p, in float d , in uint m) // p 포인트 위치 , d 셀 공간 크기 , m 해쉬 테이블 크기
{
	float invD = 1 / d;
	// 상수는 큰 소수
	float x = p.x * invD * 73856093; 
	float y = p.y * invD * 19349663; 
	float z = p.z * invD * 83492791;

	return (x ^ y ^ z) % m;
}

///////////////////////////////////// 힘 계산 함수 ////////////////////////////////////////////////////////////////////////////
// Incompressible sph 기반인거 같다.
float computeDensity_PDF(in float m, in float r, in float h) // (m 질량 , r 두 파티클 간의 거리 , h 유효 반지름 거리 )
{
    return m * W_Default(r, h);
}

float computePressure_PDF(in float r, in float h)
{
	float W = -45 * r / (pi * pow(h, 6)) * (r / r) * pow(h - r, 2);
	return W;
}

float computeViscosity_PDF(in float r, in float h)
{
	float W = 45 / (pi * pow(h, 6)) * (h - r);
	return W;
}


