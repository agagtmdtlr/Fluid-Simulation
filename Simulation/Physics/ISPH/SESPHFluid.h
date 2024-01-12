#pragma once
#include "Physics/SPHcommon.h"
#include "Physics/Data/SpatialHash.h"

#define FLUID_OFFSET (1.0)


class SESPHFluid
{

public:	SESPHFluid();
public: ~SESPHFluid();

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
private: Particle * createParticle(Vector3 position, Vector3 velocity, float mass);
private: PointID generateUniquePointID();

private: void createFluidParticle();
private: void initBoundaryData();
private: void createBounaryParticle();
private: void createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2);

public: void Update();
public: void Render();

private: void updateNeighbor();
private: void updateRenderParticleData();
		// 힘을 계산하기 위한 함수
private: void computeDensityAndPressure();
private: void computeFluidAccleration();
private: Vector3 computeObstacleAccleration(Particle * particle);
private: void computeVelocityAndPosition();
private: void enforceBoundaryPosition(Particle* particle ,Vector3 & position);


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
	if (q > 1.0f)
	{
		return 0;
	}
	else if (q > 0.5f)
	{
		return _cubic_Cof * (2.0f * powf( 1.0f - q , 3.0f ));
	}
	else
	{
		return _cubic_Cof * (6.0f *( q * q * q - q * q) + 1.0f) ;
	}
}

private: inline const float W_Default(const float & diff)
{
	return _defaultKernelCoefficient * powf(diff, 3.0f);
}

private: inline const float WG_Default(const float & diff)
{
	return _defaultKernelGradientCoefficient * diff * diff;
}

private: inline const float WL_Default(const float & h2 , const float & r2)
{
	return _defaultKernelLaplacianCodefficient * (h2 - r2) * (3.0f * h2 - 7.0f * r2);
}

private: inline const float W_Pressure(
	const float & diff	)
{
	return 
		_pressureKernelCoefficient * 
		diff *  
		diff ;
}\

private: inline const float W_Viscosity(
	const float & diff)
{
	return _viscosityKernelCoefficient * diff;
}
		
private: float _totalVolume;
private: UINT _particleCount;
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
			ni : surface normal
			i : particle			
			ci : smoothed value of the color field evaluated at particle i
			(c=1 : exactly at particle location
			(c=0 : everywhere
			o : tension coefficient <- depend on fluid surface form
		 */

private: float _surfaceTensionThreshold;
private: float _surfaceTensionCoefficeint;
private: float _pressureCoefficient; // gas stiffness
private: float _viscosityCoefficient; // viscosity cof

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
private: vector<ParticleRenderData> _particleRenderDatas; // rendering data

private: SpatialHash _spatialHash;

private: float maxFluidXPos = 0.0f;
private: float maxFluidYPos = 0.0f;
private: float maxFluidZPos = 0.0f;

};
