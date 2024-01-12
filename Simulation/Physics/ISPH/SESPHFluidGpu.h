#pragma once
#include "Physics/SPHcommon.h"
#include "Physics/Data/SpatialHash.h"




class SESPHFluidGpu
{
private:
	enum class eDispatchMode
	{
		////////////////////////////////////////////////////////////////
		// 유체 파티클 정렬을 위한 함수
		////////////////////////////////////////////////////////////////
		insertCount = 0,
		localScan = insertCount + 1,
		sumBufferScan = localScan + 1,
		globaclIncrement = sumBufferScan + 1,
		insertSortedOutput = globaclIncrement + 1,

		////////////////////////////////////////////////////////////////
		// 유체 파티클 시뮬레이션을 위한 함수
		////////////////////////////////////////////////////////////////
		computeDensityAndPressure = insertSortedOutput + 1,
		computeAcceleration = computeDensityAndPressure + 1,
		computeVelocityAndPosition = computeAcceleration + 1,
		copyToRenderResource = computeVelocityAndPosition + 1,
		copyToInputFromOutput = copyToRenderResource + 1,
		copyToOutputFromInput = copyToInputFromOutput + 1,
		clearSortData = copyToOutputFromInput + 1,
		computeForceAndUpdatePosition = clearSortData + 1,

		////////////////////////////////////////////////////////////////
		// 경계 파티클 정렬을 위한 함수
		////////////////////////////////////////////////////////////////
		insertToCountingBoundary = computeForceAndUpdatePosition + 1,
		localScanBoundary = insertToCountingBoundary + 1,
		sumBufferScanBoundary = localScanBoundary + 1,
		globaclIncrementBoundary = sumBufferScanBoundary + 1,
		insertSortedOutputBoundary = globaclIncrementBoundary + 1,

		///////////////// enum 크기 //////////////////////////////////////////
		modeSize = insertSortedOutputBoundary + 1
	};
	

public:	SESPHFluidGpu(Vector3 boundaryVolumeSize, float particleSize, float countForAxis, float kernelCount = 20.0f);
public: ~SESPHFluidGpu();

public: UINT getParticleCount() { return _particleInfo.paritlceCount; }
public: float getRadius() { return _particleSize; }

public: Vector3 getBoundary() { return _boundaryVolumeSize; }
public: UINT getBoundaryParticleCount() { return _boundaryInfo.boundaryCount; }

private: UINT _fluidParticleCount;
private: UINT _boundaryParticleCount;

private: Vector3 _fluidVolumeSize;
private: Vector3 _fluidOffset;
private: Vector3 _boundaryVolumeSize;

private: float _particleSize;
private: float _kernelCount;

private: float _soundOfSpeed = 10.0f;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private: struct cbParticleInfo
{
	UINT paritlceCount;
	float mass;
	float deltaTime;
	float particleSize;
} _particleInfo;
private: unique_ptr<ConstantBuffer> _particleInfoCBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:struct cbKernelInfo
{
	float h;
	float idealDensity;
	float gasStiffness;
	float viscosity_cof = 0.01f;
	//
	float coubic_spline_cof;
	float invLookupOffset;
	float poly6cof;
	float poly6cofGrad;
	//
	float h2;
	float padding[3];
} _kernelInfo;
private: unique_ptr<ConstantBuffer> _kernelInfoCBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private: struct cbBoundaryInfo
{
	float xmin;
	float xmax;
	float ymin;
	float ymax; 
	//
	float zmin;
	float zmax;
	UINT boundaryCount;
private:
	float padding[1];
} _boundaryInfo;
private: unique_ptr<ConstantBuffer> _boundaryInfoCBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private: struct cbHashGridInfo
{
	UINT maxCellCount;
	float cellSize;
	UINT maxBoundaryCellCount;
private:
	float padding[1];
public:
	Vector3 gridResolution;
	float padding2;
} _hashGridInfo;
private: unique_ptr<ConstantBuffer> _hashGridInfoCBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private: struct cbDebugInfo
{
	float debugScalar = 5.8f;
	Vector3 debugVector;
} _debugInfo;
private: unique_ptr<ConstantBuffer> _debugInfoCBuffer;


private: unique_ptr<Shader> _shader;

public: void initialize(); // 나머지 상태 및 계수 초기화
private: void initFluid(); // 유체 파티클 초기화
private: void initBoundary(); // 경계 파티클 초기화
private: void initializeShader(); // 시뮬레이션 쉐이더 리소스 초기화

private: void sortBoundary(); 

public: void update();
private: void updateHashGrid();
private: void updateSimulation();
private: void updateRenderResource();

public: void Render();
public: void RenderState();

private: vector<UINT> debugCountData;
private: vector<UINT> debugoffsetData;


private: vector<float> debugNeighborCountData;

private: enum class eTimerCatergory : UINT
{
	neighborSearchEnd = 0,
	simulationEnd = neighborSearchEnd + 1,
	categorySize = simulationEnd + 1
};

private: bool isTimerInit = false;
private: vector<UINT> _timers;

private: bool _boundaryParticleSorted = false;
private: float _limitCycle = 1.0f;

////////////////////////////////////////////////////////////////////////////
// LOOK UP TABLE RESOURCE
////////////////////////////////////////////////////////////////////////////
private: unique_ptr<StructuredBuffer> _lookupTableCubicSplineKernel;
private: unique_ptr<StructuredBuffer> _lookupTableCubicSplineKernelGradient;

private: vector<float> _cubicSplineKernelData;
private: vector<float> _cubicSplineKernelGradinetData;



private: bool isPlay = false;


//private: float wavePivot;
//private: float waveTime = 0.0f;
//private: float waveRange = 1.0f;

};
