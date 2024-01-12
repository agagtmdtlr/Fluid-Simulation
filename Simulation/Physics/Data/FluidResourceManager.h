
#pragma once
#include "Physics/SPHcommon.h"

// 32byte 패팅 사이즈에 맞추기
struct ParticleGpu 
{
	Vector3 position;
	Vector3 velocity;
	float density;
	float pressure;
	float state;
};

class FluidResourceManager
{	
public: static UINT debugHashKey;

private: enum { SUMBUFFER_SIZE = 1024 };

private: static FluidResourceManager * _singleton;
private: static bool _bResourceInitialized; 

// 싱글톤 객체라서 생성/소멸자를 은닉한다.
private: FluidResourceManager();
private: ~FluidResourceManager();

public: static FluidResourceManager* Get();

public: void CopyOutputToInput();

//public: shared_ptr<StructuredBuffer> GetParticleBuffer() { return _particles; }
public: shared_ptr<StructuredBuffer> GetInputParticleBuffer() { return _inputParticle; }
public: shared_ptr<StructuredBuffer> GetOutputParticleBuffer() { return _outputParticle; }

public: shared_ptr<StructuredBuffer> GetCellIndexBuffer() { return _countEachCellIndex; }
public: shared_ptr<StructuredBuffer> GetSumBuffer() { return _sumBuffer; }
public: shared_ptr<StructuredBuffer> GetOffsetBuffer() { return _countOffsetEachParticle; }
public: shared_ptr<StructuredBuffer> GetRenderResourceBuffer() { return _resourceRender; }

public: shared_ptr<StructuredBuffer> GetDebugBuffer() { return _debugBuffer; }

public: shared_ptr<StructuredBuffer> GetBoundaryParticleBuffer() { return _boundaryParticle; }
public: shared_ptr<StructuredBuffer> GetBoundaryCellIndexBuffer() { return _countEachBoundaryCellIndex; }
public:	shared_ptr<StructuredBuffer> GetBoundaryOffsetBuffer() { return _countOffsetEachBoundaryParticle; }
		


public: UINT SetParticleResource( const Vector3 volume, const float offset );
private: void CreateParticleData();

public: UINT SetCellResource( const UINT pcount , const Vector3 boundaryVolume , const float offset);
private: void CreateCellData();


public: UINT SetBoundaryParticleResource( const Vector3 volume, const float offset );

		// [ c1, c2 ] 평면 [ c3 ] 층
private: void createBoundaryPlane(
	const float c1, const float c2, const float c3, 
	const Vector3 dir1, const Vector3 dir2 ,const Vector3 dir3, 
	const float offset, const Vector3 pivot);
private: void CreateBoundaryParticleData();


public: UINT SetBoundaryCellResource(
	const UINT pcount
);
private: void CreateBoundaryCellData();
		/*
			N * sizeof(Vector3) : position;
			+ N * sizeof(Vector3) : accl
			+ N * sizeof(Vector3) : velocity;
		*/
///////////////////////////////////////////////////////////////////////
// [length:particleCount]
///////////////////////////////////////////////////////////////////////
// [ size : type(Particle) ] 이 데이터는 counting 에서 read/write 에 해당하는 데이터 이다.
private: shared_ptr<StructuredBuffer> _inputParticle;
private: shared_ptr<StructuredBuffer> _outputParticle;
// [ size : int ] 이 데이터 counting 에서 write 에 해당하는 데이터이다.
private: shared_ptr<StructuredBuffer> _countOffsetEachParticle;
// [ size : type(ParticleRenderData) ] 
private: shared_ptr<StructuredBuffer> _resourceRender; // 

/*
	cell Index 를 카운팅함과 동시에 현재 파티클에게 해당 카운팅넘버를 저장한다. 이후 prefixSum이후 파티클이 자신의 위치를 찾는데 사용한다.
	outputParticles[ countNumberEachCellIndex[ cell_index ] + countNumberEachParticles[ paritlce_id ] ] = particle_data;
*/

// [ length : cellmaxCount / size : int ] 이 데이터는 counting + prefixSum에서 write 에 해당하는 데이터이다.
private: shared_ptr<StructuredBuffer> _countEachCellIndex;
private: shared_ptr<StructuredBuffer> _sumBuffer;


private: vector<ParticleGpu> _particleData;
private: UINT _particleCount;
private: UINT _cellCount;

private: shared_ptr<StructuredBuffer> _debugBuffer;



		 
///////////////////////////////////////////////////////////////////////////////////////////////////
// 경계 파티클 데이터
//////////////////////////////////////////////////////////////////////////////////////////////////
private: shared_ptr<StructuredBuffer> _boundaryParticle;
private: shared_ptr<StructuredBuffer> _countOffsetEachBoundaryParticle;
private: shared_ptr<StructuredBuffer> _countEachBoundaryCellIndex;
private: shared_ptr<StructuredBuffer> _boundaryResourceRender; // 


private: vector<ParticleGpu> _boundaryParticleData;
private: UINT _boundaryParticleCount;
private: UINT _boundaryCellCount;

};