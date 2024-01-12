#include "Framework.h"
#include "FluidResourceManager.h"


//#define PARTICLE_LIMIT	(500000)
#define PARTICLE_LIMIT	(1000000)
#define CELL_LIMIT		(PARTICLE_LIMIT * 2)
#define CREATE_WEIGHT	(2.0f)

FluidResourceManager* FluidResourceManager::_singleton = nullptr;
bool				  FluidResourceManager::_bResourceInitialized = false;

UINT FluidResourceManager::debugHashKey = 0;


FluidResourceManager::FluidResourceManager()
	:
	_particleCount(0),
	_cellCount(0)
{
	_sumBuffer = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(UINT), SUMBUFFER_SIZE);
}

FluidResourceManager::~FluidResourceManager()
{
}

FluidResourceManager * FluidResourceManager::Get()
{
	if (_singleton == nullptr)
	{
		_singleton = new FluidResourceManager();
	}
	return _singleton;
}

void FluidResourceManager::CopyOutputToInput()
{
	D3D11_BOX box;
	box.left = 0;
	box.right = _particleCount;
	box.top = 0;
	box.bottom = 0;
	box.front = 0;
	box.back = 0;

	D3D::GetDC()->CopySubresourceRegion(_inputParticle->GetOutput(),
		0,
		_particleCount,
		0,
		0,
		_outputParticle->GetOutput(),
		0,
		&box);
}

UINT FluidResourceManager::SetParticleResource(const Vector3 volume, const float offset)
{
	UINT xCount = static_cast<UINT>( volume.x / ( 2.0f * offset) );
	UINT yCount = static_cast<UINT>( volume.y / ( 2.0f * offset) );
	UINT zCount = static_cast<UINT>( volume.z / ( 2.0f * offset) );

	_particleCount = xCount * yCount * zCount;
	if (_particleCount > PARTICLE_LIMIT) { assert(false); }

	_particleData.resize(_particleCount);

	//float bbmin = 0.1f;
	float bbmin = offset * 2.0f;

	
	for (UINT i = 0; i < yCount; i++)
	{
		for (UINT j = 0; j < zCount; j++)
		{
			for (UINT k = 0; k < xCount; k++)
			{
				UINT index = k + (j * xCount) + (i * xCount * zCount);
				_particleData[index].position = 
				{ 
					bbmin + (float)k * offset * CREATE_WEIGHT , 
					bbmin +	(float)i * offset * CREATE_WEIGHT,
					bbmin + (float)j * offset * CREATE_WEIGHT };
				_particleData[index].state = 1;
				_particleData[index].velocity = { 0.0f, 0.0f, 0.0f };
			}
		}
	}

	CreateParticleData();
	return _particleCount;
}

void FluidResourceManager::CreateParticleData()
{
	// use to simulation and sort
	/*
	*	input <Particle>
	*	output <Particle>
	*	count offset <uint>
	*	render resource <Vector3>
	*	은 동일한 길이이다.
	*	input 은 neighbor 에서 사용된다.
	*/
	
	_inputParticle = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(ParticleGpu), _particleCount, &_particleData[0]);
	_outputParticle = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(ParticleGpu), _particleCount);

	// use to offset
	_countOffsetEachParticle = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(UINT), _particleCount);
	// render resource
	_resourceRender = make_shared<StructuredBuffer>(nullptr, sizeof(ParticleRenderData), 1, sizeof(ParticleRenderData), _particleCount);

	_debugBuffer = make_shared<StructuredBuffer>(nullptr, sizeof(float), 1, sizeof(float), _particleCount);
}

UINT FluidResourceManager::SetCellResource(const UINT pcount, const Vector3 boundaryVolume, const float offset)
{
	Vector3 temp = boundaryVolume / offset;
	_cellCount = (UINT)Math::Volume(temp);
	if (_cellCount > CELL_LIMIT) { assert(false); }
	CreateCellData();
	return _cellCount;
	   

	_cellCount = pcount * 2;
	if (!Math::IsPrime(_cellCount))
	{
		_cellCount = Math::NextPrime(_cellCount);
	}
	if (_cellCount > CELL_LIMIT) { assert(false); }
	CreateCellData();
	return _cellCount;
}

void FluidResourceManager::CreateCellData()
{
	_countEachCellIndex = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(UINT), _cellCount);
}

UINT FluidResourceManager::SetBoundaryParticleResource(const Vector3 volume, const float offset)
{
	Vector3 xdir = { 1.0f,0.0f,0.0f };
	Vector3 ydir = { 0.0f,1.0f,0.0f };
	Vector3 zdir = { 0.0f,0.0f,1.0f };

	float xmax = volume.x;
	float ymax = volume.y;
	float zmax = volume.z;

	Vector3 pivot;

	// 아랫면 윗면 
	pivot = { -6.0f * offset , 0, -6.0f * offset };
	createBoundaryPlane(xmax + 6.0f * offset, zmax + 6.0f * offset , 6.0f * offset, xdir, zdir, -ydir, offset, pivot);
	pivot = { 0,ymax,0 };
	createBoundaryPlane(xmax, zmax, 6.0f * offset, xdir, zdir, ydir, offset, pivot);
	// 옆면 [ y , z ]
	pivot = { 0,0,0 };
	createBoundaryPlane(ymax, zmax, 6.0f * offset, ydir, zdir, -xdir, offset, pivot);
	pivot = { xmax,0,0 };
	createBoundaryPlane(ymax, zmax, 6.0f * offset, ydir, zdir, xdir, offset, pivot);
	// 옆면 [ x , y ]
	pivot = { 0,0,0 };
	createBoundaryPlane(xmax, ymax, 6.0f * offset, xdir, ydir, -zdir, offset, pivot);
	pivot = { 0,0,zmax };
	createBoundaryPlane(xmax, ymax, 6.0f * offset, xdir, ydir, zdir, offset, pivot);

	_boundaryParticleCount = _boundaryParticleData.size();
	//CreateBoundaryParticleData();
	return _boundaryParticleCount;
}

void FluidResourceManager::createBoundaryPlane(
	const float c1, const float c2, const float c3,
	const Vector3 dir1, const Vector3 dir2, const Vector3 dir3,
	const float offset, const Vector3 pivot)
{	
	Vector3 xdir = { 1.0f,0.0f,0.0f };
	Vector3 ydir = { 0.0f,1.0f,0.0f };
	Vector3 zdir = { 0.0f,0.0f,1.0f };

	float r2 = offset * 2.0f;
	for (float i = 0; i < c3; i += r2)
	{
		for (float j = 0; j < c1; j += r2)
		{
			for (float k = 0; k < c2; k += r2)
			{
				ParticleGpu p;
				p.position =
					dir1 * j + dir2 * k + dir3 * i;
				p.position += pivot;
				p.velocity = { 0, 0, 0 };
				p.state = 0;
				_boundaryParticleData.push_back(p);
			}
		}
	}
}

void FluidResourceManager::CreateBoundaryParticleData()
{
	_boundaryParticle = make_shared<StructuredBuffer>(&_boundaryParticleData[0], 
		sizeof(ParticleGpu), _boundaryParticleCount, /// input
		sizeof(ParticleGpu), _boundaryParticleCount);/// output
	// use to offset
	_countOffsetEachBoundaryParticle = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(UINT), _boundaryParticleCount);

	_boundaryResourceRender = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(ParticleRenderData), _boundaryParticleCount);
}

UINT FluidResourceManager::SetBoundaryCellResource(const UINT pcount)
{
	_boundaryCellCount = pcount * 2;
	if (!Math::IsPrime(_boundaryCellCount))
	{
		_boundaryCellCount = Math::NextPrime(_boundaryCellCount);
	}
	if (_boundaryCellCount > CELL_LIMIT) { assert(false); }

	CreateBoundaryCellData();
	return _boundaryCellCount;
}

void FluidResourceManager::CreateBoundaryCellData()
{
	_countEachBoundaryCellIndex = make_shared<StructuredBuffer>(nullptr, sizeof(UINT), 1, sizeof(UINT), _boundaryCellCount);
}
