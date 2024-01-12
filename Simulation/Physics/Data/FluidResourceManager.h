
#pragma once
#include "Physics/SPHcommon.h"

// 32byte ���� ����� ���߱�
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

// �̱��� ��ü�� ����/�Ҹ��ڸ� �����Ѵ�.
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

		// [ c1, c2 ] ��� [ c3 ] ��
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
// [ size : type(Particle) ] �� �����ʹ� counting ���� read/write �� �ش��ϴ� ������ �̴�.
private: shared_ptr<StructuredBuffer> _inputParticle;
private: shared_ptr<StructuredBuffer> _outputParticle;
// [ size : int ] �� ������ counting ���� write �� �ش��ϴ� �������̴�.
private: shared_ptr<StructuredBuffer> _countOffsetEachParticle;
// [ size : type(ParticleRenderData) ] 
private: shared_ptr<StructuredBuffer> _resourceRender; // 

/*
	cell Index �� ī�����԰� ���ÿ� ���� ��ƼŬ���� �ش� ī���óѹ��� �����Ѵ�. ���� prefixSum���� ��ƼŬ�� �ڽ��� ��ġ�� ã�µ� ����Ѵ�.
	outputParticles[ countNumberEachCellIndex[ cell_index ] + countNumberEachParticles[ paritlce_id ] ] = particle_data;
*/

// [ length : cellmaxCount / size : int ] �� �����ʹ� counting + prefixSum���� write �� �ش��ϴ� �������̴�.
private: shared_ptr<StructuredBuffer> _countEachCellIndex;
private: shared_ptr<StructuredBuffer> _sumBuffer;


private: vector<ParticleGpu> _particleData;
private: UINT _particleCount;
private: UINT _cellCount;

private: shared_ptr<StructuredBuffer> _debugBuffer;



		 
///////////////////////////////////////////////////////////////////////////////////////////////////
// ��� ��ƼŬ ������
//////////////////////////////////////////////////////////////////////////////////////////////////
private: shared_ptr<StructuredBuffer> _boundaryParticle;
private: shared_ptr<StructuredBuffer> _countOffsetEachBoundaryParticle;
private: shared_ptr<StructuredBuffer> _countEachBoundaryCellIndex;
private: shared_ptr<StructuredBuffer> _boundaryResourceRender; // 


private: vector<ParticleGpu> _boundaryParticleData;
private: UINT _boundaryParticleCount;
private: UINT _boundaryCellCount;

};