#pragma once
#include "Physics/SPHcommon.h"
#include "Physics/Data/SpatialHash.h"


class DFSPHFluid
{



public:	DFSPHFluid();
public: ~DFSPHFluid();

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
private: void createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2);

private: void computeDensityAndKFactor();

public: void InitSimulation();
public: void Update();
public: void Render();

private: void updateNeighbor();
private: void updateRenderParticleData();
		// 힘을 계산하기 위한 함수
private: void computeNonePressureAcceleration();
private: float adaptTimestepSizeByCflCondition(const float dt);
////////////////////////////////////////////////////////////////////////////////
// Constant Density Solver
////////////////////////////////////////////////////////////////////////////////
private: void computeConstantDensitySolver(const float dt);
private: void PredictDensityAndPressure(const float dt);
private: void CorrectVelocityByPressure(const float dt);

////////////////////////////////////////////////////////////////////////////////
// Divergence-Free Solver
////////////////////////////////////////////////////////////////////////////////
private: void computeDivergenceFreeSolver(const float dt);
private: void PredictDFSovlerPressure(const float dt);
private: void CorrectVelocityByDFSovlerPressure(const float dt);


private: void computePosition(const float dt);
private: Vector3 computeObstacleAccleration(Particle * particle);
private: void enforceBoundaryPosition(Particle* particle ,Vector3 & position);

private: UINT _debugSurface;
private: Vector3 _debugPosition;


private: float _sumDensity = 0;
private: float _sumPressure = 0;

private: float _debugDisplacement;
private: float _debugPressureForce;


private: float _cubic_Cof;
private: float _cubic_gradient_cof;

private: void SetCoefficient(float h)
{
	//_cubic_Cof = 8.0f / (_pi * powf(h, 3.0f));
	//_cubic_Cof = 1.0f / (_pi * powf(h, 3.0f));
	_cubic_Cof = 0.25f / (_pi * h * h * h);

	float h3 = powf(_h, 3.0f);
	float h6 = powf(_h, 6.0f);
	float h9 = powf(_h, 9.0f);

	_defaultKernelCoefficient = 315.0f / (64.0f * _pi * h9);
	_defaultKernelGradientCoefficient = -945.0f / (32.0f * _pi * h9);
	_defaultKernelLaplacianCodefficient = -945.0f / (32.0f * _pi * h9);

	_pressureKernelCoefficient = -45.0f / (_pi * h6);
	_viscosityKernelCoefficient = 45.0f / (_pi * h6);
}
	
// q = r / h;
private: inline const float W_Cubic( const float & r, const float & h )
{
	const float && q = 2.0f * r / h;
	
	if (q > 2.0f) return 2.0f;
	else 
	{
		const float & a = _cubic_Cof;
		return a* ( q > 1.0f ? ( powf(2.0f - q, 3.0f) ) : ( (3.0f * q - 6.0f ) * q * q  + 4.0f));
	}
}

private: inline const Vector3 WG_Cubic( const Vector3 & r, const float & h )
{
	float && q = 2.0f * D3DXVec3Length(&r) / h;
	
	if (q > 2.0f) return { 0,0,0 };
	else 
	{
		const Vector3 && a = r / (_pi * ( q + Math::EPSILON ) * powf(h ,5.0f));
		return a * ((q > 1.0f)? ((12.0f - 3.0f * q) * q) : ((9.0f * q - 12.0f) * q));
	}
}

private: inline const float W_Default( const float & diff )
{
	return _defaultKernelCoefficient * powf(diff, 3.0f);
}

private: inline const float WG_Default( const float & diff )
{
	return _defaultKernelGradientCoefficient * diff * diff;
}

private: inline const float WL_Default( const float & h2 , const float & r2 )
{
	return _defaultKernelLaplacianCodefficient * (h2 - r2) * (3.0f * h2 - 7.0f * r2);
}

private: inline const float WG_Spiky( const float & diff )
{
	return 
		_pressureKernelCoefficient * 
		diff *  
		diff ;
}

private: inline const float W_Viscosity( const float & diff )
{
	return _viscosityKernelCoefficient * diff;
}
		
private: float _totalVolume;
private: UINT _particleCount;
private: UINT _obstacleCount;
private: float _N; // n
private: float _X;   // x			 
private: UINT particleCurrentIdToGenerate = 0;

		 /////////////////////////////////////////////////////////////////
		 // Time integration CFL condition data
		 //////////////////////////////////////////////////////////////////
private: float _deltaTime;
private: float _timeStep;
private: float _cflFactor;
private: float _cflMaxSize;
private: float _cflMinSize;

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


		 /******************************************************************
		 ***********힘 연산에 사용하기 위해 미리 정의하는 상수들***************
		 ********************************************************************/
private: float _renderRadius;

private: float _pi;
private: float _h; // kernel support radius
private: float _invH;
private: float _h2; // kernel
private: float _particleRadius; // spatialHashing
private: float _cellSize;// spartialHasing
private: float _mass;
private: float _idealDensity;
private: float _avgPredictDensity;
private: float _densityThreshold;

private: float _DpDtAvg;
private: float _DpDtThreshold;
private: Vector3 gravity = { 0.0f , -9.82f , 0.0f };

private: float _surfaceTensionThreshold;
private: float _surfaceTensionCoefficeint; 

private: float _viscosityCoefficient; // viscosity coefficient

private: float _defaultKernelCoefficient; // used to mass-density

private: float _pressureKernelCoefficient; // used to pressure force
private: float _viscosityKernelCoefficient; // used to viscosity force

private: float _defaultKernelGradientCoefficient; // used to surface normal
private: float _defaultKernelLaplacianCodefficient; // used to surface tensor force

private: UINT _maxCellCount; // |xmax - xmin| * |ymax - ymin| * |zmax - zmin| * (1/h);
// particle data
private: vector<Particle*> _fluidParticles;
private: vector<float> _predictDensity;
private: vector<float> _DFSolverValues;
private: vector<float> _DFSolverPressureValues;

private: vector<Particle*> _obstacleParticles;
private: vector<Particle*> _allParticles;

private: vector<vector<Cell*>> _particleNeighbor; // simulation data
private: vector<vector<Particle*>> _particleNeighborDataList; //테스트용;

// WG의 값의 경우 자주 사용되므로
private: struct NeighborData
{
	Vector3 rVec;
	UINT id;
	float wG;
	float w;
};

private: vector<vector<NeighborData>> _particleNeighborData;

private: vector<ParticleRenderData> _particleRenderDatas; // rendering data

private: SpatialHash _spatialHash;

private: float maxFluidXPos = 0.0f;
private: float maxFluidYPos = 0.0f;
private: float maxFluidZPos = 0.0f;

};
