#pragma once
#include "Physics/SPHcommon.h"
#include "Physics/Data/SpatialHash.h"

class SESPHFluidThread
{

public:	SESPHFluidThread();
public: ~SESPHFluidThread();

public: UINT calculateCellMax();

public: Vector3 getBoundary() { return { _xmax, _ymax, _zmax }; }
public: void Initialize();
		// getter
public: const vector<ParticleRenderData> & getFluidParticlesRenderData() { return _particleRenderDatas; }
public: inline UINT getParitlceCount() { return _fluidParticles.size(); }
public: inline float getRadius() { return _renderRadius; }
		// initialize
private: void initState();
private: void initConstantState();
private: void initFluidData();
private: void initSpatialHash();
private: Particle * createParticle(Vector3 position, Vector3 velocity, float mass);
private: PointID generateUniquePointID();

private: void createFluidParticle();
private: void initBoundaryData();
private: void createBounaryParticle();
private: void createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2, Vector3 offset);

public: void Update();
public: void Render();

private: void updateNeighbor();
private: void executeNeighborThreadFunction(UINT it, UINT endit);

private: void updateRenderParticleData();
		 // 힘을 계산하기 위한 함수
private: void computeDensityAndPressure();
private: void executeDensityAndPressureThreadFunction(UINT it, UINT endit);

private: void computeFluidAccleration();
private: void executeFluidAcclerationThreadFunction(UINT it, UINT endit);

private: Vector3 computeObstacleAccleration(Particle * particle);
private: void computeVelocityAndPosition();
private: void enforceBoundaryPosition(Particle* particle, Vector3 & position);


private: UINT _debugSurface;
private: float _sumDensity = 0;
private: float _sumPressure = 0;

private: float _debugDisplacement;
private: float _debugPressureForce;

private: float _invh;
private: float _cubic_Cof;

private: void SetCubicCoefficient(float h)
{
	_cubic_Cof = 8.0f / (_pi * powf(h, 3.0f));
}

// q = r / h;
private: inline const float W_Cubic(const float & q)
{
	float w = 0;
	if (q > 1.0f)
	{
		w = 0;
	}
	else if (q > 0.5f)
	{
		w = 2.0f * powf(1.0f - q, 3.0f);
	}
	else if (q >= 0.0f)
	{
		float q2 = q * q;
		w = (6.0f * q2  * (q - 1.0f) + 1.0f);
	}
	return w * _cubic_Cof;
}
private: inline const float W_Cubic(const float & r, const float & h)
{
	float w = 0;
	const float && q = r / h;
	if (q > 1.0f)
	{
		w = 0;
	}
	else if (q > 0.5f)
	{
		w = 2.0f * powf(1.0f - q, 3.0f);
	}
	else if (q >= 0.0f)
	{
		w = 6.0f * ( q * q * q - q * q ) + 1.0f;
	}
	
	return w * _cubic_Cof;
}
private: inline const float WG_Cubic(const float & q)
{
	float w = 0;
	if (q > 1.0f)
	{
		w = 0;
	}
	else if (q > 0.5f)
	{
		w = -6.0f * powf(1.0f - q, 2.0f);
	}
	else if (q >= 0.0f)
	{
		w = q * (18.0f * q - 12.0f);
	}
	return w * _cubic_Cof;
}
private: inline const float WG_Cubic(const float & r, const float & h)
{
	const float && q = r / h;
	const float && gw = 1.0f / (r + Math::EPSILON);
	float w = 0;
	if (q > 1.0f)
	{
		w = 0;
	}
	else if (q > 0.5f)
	{
		w = -6.0f * powf(1.0f - q, 2.0f);
	}
	else if (q >= 0.0f)
	{
		w = q * (18.0f * q - 12.0f);
	}
	return w * _cubic_Cof * gw;
}

private: inline const float WL_Viscosity(
	const float & r, const float & h)
{
	return (r <= h) ? (45.0f * (h - r) / (_pi * powf(h, 6.0f))) : 0.0f;
}

private: inline const Vector3 WG_Surface(const Vector3 & r, const float & h)
{
	const float x = D3DXVec3Length(&r);
	if (x > h || x < Math::EPSILON) return { 0,0,0 };
	else
	{
		auto cube = [](float x) { return x * x * x; };
		const Vector3 && a = 136.0241f * -r / (_pi * cube(h) * cube(h) * cube(h) * x);
		return a * (( 2.0f * x <= h) ?
			(2.0f * cube(h - x) * cube(x) - 0.0156f * cube(h) * cube(h)) :
			(cube(h - x) * cube(x)));
	}

}
private: float _totalVolume;
private: UINT _particleCount;
private: UINT _obstacleCount;
private: float _N; // n
private: float _X;   // x

private: UINT particleCurrentIdToGenerate = 0;
private: float _deltaTime;
		 // boundary plane data
private: float _boundaryRadius;
private: float _boundaryMinForce;
private: float _boundaryMaxForce;

private: float _xmin;
private: float _xmax;

private: float _ymin;
private: float _ymax;

private: float _zmin;
private: float _zmax;

private: bool _bInitialized = false;
private: bool _bOptionChanged = false;


		 /*
		 힘 연산에 사용하기 위해 미리 정의하는 상수들
		 */
private: float _renderRadius;

private: float _pi;
private: float _h; // kernel support radius
private: float _h2; // kernel
private: float _particleRadius; // spatialHashing
private: float _cellSize;// spartialHasing
private: float _mass;
private: float _idealDensity;
private: Vector3 gravity = { 0.0f , -9.82f , 0.0f };

		 /*
			n_i : surface normal
			i : particle
			c_i : smoothed value of the color field evaluated at particle i
			(c=1 : exactly at particle location
			(c=0 : everywhere
			o : tension coefficient <- depend on fluid surface form
		 */

private: float _lookupOffset;
private: float _invlookupOffset;
private: inline const UINT lookupCubicKey(const float & r)
{
	return (UINT)(r * _invlookupOffset);
}

private: vector<float> LookUpWCubic;
private: vector<float> LookUpWCubic_Grad;

private: vector<float> LookUpWDefault_Grad;
private: vector<float> LookUpWDefault_Lap;
private: vector<float> LookUpWDefault;
private: vector<float> LookUpWPressure;
private: vector<float> LookUpWViscosity;




private: float _surfaceTensionThreshold;
private: float _surfaceTensionCoefficeint;
private: float _pressureCoefficient; // gas stiffness
private: float _viscosityCoefficient; // viscosity coefficient

private: float _defaultKernelCoefficient; // mass-density

private: float _pressureKernelCoefficient; // pressure force
private: float _viscosityKernelCoefficient; // viscosity force

private: float _defaultKernelGradientCoefficient; // surface normal
private: float _defaultKernelLaplacianCodefficient; // surface tensor force

private: UINT _maxCellCount; // |xmax - xmin| * |ymax - ymin| * |zmax - zmin| * (1/h);
// particle data
private: vector<Particle*> _fluidParticles;
private: vector<Particle*> _obstacleParticles;
private: vector<Particle*> _allParticles;

private: vector<vector<Cell*>> _particleNeighbor; // simulation data
private: vector<vector<Particle*>> _particleNeighborDataList; //테스트용;

private: vector<ParticleRenderData> _particleRenderDatas; // rendering data

private: SpatialHash _spatialHash;

private: float maxFluidXPos = 0.0f;
private: float maxFluidYPos = 0.0f;
private: float maxFluidZPos = 0.0f;


private: vector<float> debugBuffer;
};
