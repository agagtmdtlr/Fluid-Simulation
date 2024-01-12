#include "Framework.h"
#include <time.h>
#include <random>
#include "Physics/Data/FluidResourceManager.h"
#include "Physics/ISPH/SESPHFluidThread.h"

#define OFFSET_UNIT (1000.0f)
#define CREATE_OFFSET (2.0f) // 생성 간격 (크기는 h )

#define CALC_PCOUNT (_particleCount * 1.0f)
#define MASS_WEIGHT (1.0f)
#define VOLUME_WEIGHT (1.00f)
#define VOLUME_COFFICIENT (1.0f)

#define OBSTACLE_MASS_WEIGHT (1.0f)
#define TIME_WEIGHT (1.0f)
#define TIME_MAX (0.016f)
#define TIME_MIN (0.004f)

#define THREAD_GROUP_SIZE (16)

#define ACC_MAX (175.0f)
#define V_MAX (175.0f)

#define VEL_X ( 0.0f )

#define CREATE_COUNT (25)
#define CREATE_COUNT_FLOAT (static_cast<float>(CREATE_COUNT))
#define CREATE_COUNT_UINT (static_cast<UINT>(CREATE_COUNT))
#define RENDER_WEIGHT ( CREATE_COUNT_FLOAT / 20.0f )



SESPHFluidThread::SESPHFluidThread()
	:
	//_viscosityCoefficient(0.052f),
	_viscosityCoefficient(0.03f),

	_pressureCoefficient(60000.0f),// gas stiffness(k);

	_deltaTime(0.0016f),
	_idealDensity(2000.0f),

	_particleCount(CREATE_COUNT_UINT * CREATE_COUNT_UINT * CREATE_COUNT_UINT),
	_obstacleCount(0),

	//_N(500.0f),
	_X(20.0f),
	// boundary
	_xmin(+0.0f),
	_xmax(+6.8f),
	_ymin(+0.0f),
	_ymax(+2.0f),
	_zmin(+0.0f),
	_zmax(+1.8f),
	_spatialHash(),
	_pi(3.1415926535897f)
{

	Initialize();
}

void SESPHFluidThread::Initialize()
{
	initState();
	initFluidData();
	initBoundaryData();
	initConstantState();
	initSpatialHash();
}

void SESPHFluidThread::initState()
{
	//_renderRadius = (0.4641f / CREATE_COUNT_FLOAT) * RENDER_WEIGHT;
	_renderRadius = (0.4641f / CREATE_COUNT_FLOAT) * RENDER_WEIGHT;


	Vector3 vol = {
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET),
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET),
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET)
	};

	_totalVolume = vol.x * vol.y * vol.z;
	_totalVolume *= VOLUME_WEIGHT;

	//////////////////////////////////////////////////////////////////////////////////////////
	// kernel radius 초기화 + cell list size 초기화;
	//////////////////////////////////////////////////////////////////////////////////////////
	_h = cbrt(
		(3.0f * _totalVolume * _X)
		/ (4.0f * _pi * CALC_PCOUNT));
	_invh = 1.0f / _h;
	//_h = _renderRadius * 4.0f;
	_cellSize = _h;
	_particleRadius = _h;
	_spatialHash.setCellSize(_cellSize);

	//////////////////////////////////////////////////////////////////////////////////////////
	// 질량 초기화
	//////////////////////////////////////////////////////////////////////////////////////////
	_mass = static_cast<float>(_idealDensity * (_totalVolume / _particleCount));
	_mass *= MASS_WEIGHT;

	//_renderRadius = cbrt((3.0f * _mass) / ( _pi * 4.0f * _idealDensity ));

	debugBuffer.resize(_particleCount);
}

void SESPHFluidThread::initConstantState()
{
	_invh = 1.0f / _h;
	_h2 = _h * _h;
	float h3 = powf(_h, 3.0f);
	float h6 = powf(_h, 6.0f);
	float h9 = powf(_h, 9.0f);

	_defaultKernelCoefficient = 315.0f / (64.0f * _pi * h9);
	_defaultKernelGradientCoefficient = -945.0f / (32.0f * _pi * h9);
	_defaultKernelLaplacianCodefficient = -945.0f / (32.0f * _pi * h9);

	_pressureKernelCoefficient = -45.0f / (_pi * h6);
	_viscosityKernelCoefficient = 45.0f / (_pi * h6);

	SetCubicCoefficient(_h);

	LookUpWCubic.resize(10001);
	LookUpWCubic_Grad.resize(10001);
	//LookUpWViscosity.resize(10000);

	_lookupOffset = _h / 10000.0f;
	_invlookupOffset = 1.0f / _lookupOffset;
	float r = 0.0f;

	for (UINT i = 0; i < 10001; i++)
	{
		LookUpWCubic[i] = W_Cubic(r * _invh);
		LookUpWCubic_Grad[i] = WG_Cubic(r , _h);
		r += _lookupOffset;
	}
}


Particle * SESPHFluidThread::createParticle(Vector3 position, Vector3 velocity, float mass)
{
	Particle* newParticle = new Particle();
	newParticle->id = generateUniquePointID();
	newParticle->position = position;
	newParticle->velocity = velocity;
	newParticle->mass = mass;
	newParticle->prevCell[0] = INT_MAX;
	newParticle->prevCell[1] = INT_MAX;
	newParticle->prevCell[2] = INT_MAX;
	return newParticle;
}

PointID SESPHFluidThread::generateUniquePointID()
{
	return particleCurrentIdToGenerate++;
}

void SESPHFluidThread::initFluidData()
{
	createFluidParticle();
}

void SESPHFluidThread::initSpatialHash()
{
	///////////////////////////////////////////////////
	// SPATHIAL HASH
	///////////////////////////////////////////////////
	_particleNeighbor.resize(_particleCount);

	_allParticles.reserve(_particleCount + _obstacleCount);

	_maxCellCount = calculateCellMax();
	_spatialHash.resizeFreeCells(_maxCellCount);

	for (Particle * p : _fluidParticles)
	{
		_spatialHash.addParticle(p);
		_allParticles.push_back(p);
	}
	for (Particle * p : _obstacleParticles)
	{
		_spatialHash.addParticle(p);
		_allParticles.push_back(p);
	}

	///////////////////////////////////////////////////
	// RENDER DATA
	///////////////////////////////////////////////////
	_particleRenderDatas.resize(_particleCount);

}

void SESPHFluidThread::createFluidParticle()
{
	// 정의한 공간내에 일정 구획이 모여있는 파티클 집단 중력에 의해 떨어질 것이다.
	float fxmin = _xmin + _renderRadius  + 0.3f;
	float fxmax = _xmax;
	float fymin = _ymin + _renderRadius;
	float fymax = _ymax;
	float fzmin = _zmin + _renderRadius + 0.3f;
	float fzmax = _zmax;

	_fluidParticles.reserve(_particleCount);


	srand((unsigned int)time(NULL));

	UINT ii;
	UINT jj;
	UINT kk;
	for (ii = 0; ii < CREATE_COUNT_UINT; ii++)
	{
		for (jj = 0; jj < CREATE_COUNT_UINT; jj++)
		{
			for (kk = 0; kk < CREATE_COUNT_UINT; kk++)
			{
				Vector3 position = {
					fxmin + static_cast<float>(ii) * (_renderRadius * CREATE_OFFSET),
					fymin + static_cast<float>(jj) * (_renderRadius * CREATE_OFFSET),
					fzmin + static_cast<float>(kk) * (_renderRadius * CREATE_OFFSET)
				};
				//Vector3 velocity(Math::RandomVec3(-1.0f, 1.0f));
				Vector3 velocity(VEL_X, 0.0f, 0.0f);
				Particle* newParticle = createParticle(position, velocity, _mass);

				_fluidParticles.push_back(newParticle);
			}
		}
	}


	_fluidParticles.shrink_to_fit();
	_particleRenderDatas.shrink_to_fit();
}

void SESPHFluidThread::initBoundaryData()
{
	createBounaryParticle();
}

void SESPHFluidThread::createBounaryParticle()
{
	UINT x = static_cast<UINT>((_xmax - _xmin) / (_renderRadius * CREATE_OFFSET));
	UINT y = static_cast<UINT>((_ymax - _ymin) / (_renderRadius * CREATE_OFFSET));
	UINT z = static_cast<UINT>((_zmax - _zmax) / (_renderRadius * CREATE_OFFSET));

	Vector3 xdir = { 1.0f,0.0f,0.0f };
	Vector3 ydir = { 0.0f,1.0f,0.0f };
	Vector3 zdir = { 0.0f,0.0f,1.0f };

	/////////////////////////////////////////////////////////////
	// 바닥
	/////////////////////////////////////////////////////////////
	Vector3 offset = { 0,0,0 };
	createBoundaryPlane(_xmax, _zmax, xdir, zdir, offset);
	/////////////////////////////////////////////////////////////
	// 벽면
	/////////////////////////////////////////////////////////////
	offset = { 0,0,0 };
	createBoundaryPlane(2.0f, _zmax, ydir, zdir, offset);
	offset = { _xmax,0,0 };
	createBoundaryPlane(2.0f, _zmax, ydir, zdir, offset);
	offset = { 0,0,0 };
	createBoundaryPlane(2.0f, _xmax, ydir, xdir, offset);
	offset = { 0,0,_zmax };
	createBoundaryPlane(2.0f, _xmax, ydir, xdir, offset);



}

void SESPHFluidThread::createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2, Vector3 offset)
{
	float r2 = _renderRadius * 2.0f;
	Vector3 velocity = { 0 ,0 ,0 };


	for (float i = 0; i < c1; i += r2)
	{
		for (float j = 0; j < c2; j += r2)
		{
			Vector3 position = dir1 * i + dir2 * j;
			position += offset;
			Particle* newParticle = createParticle(position, velocity, OBSTACLE_MASS_WEIGHT * _mass);
			newParticle->density = _idealDensity;
			newParticle->pressure = 0;
			newParticle->pType = ParticleTypeSolid;
			_obstacleParticles.push_back(newParticle);
			_obstacleCount++;
		}
	}
	int a = 0;
}

void SESPHFluidThread::Update()
{
	ImGui::Text(to_string(FluidResourceManager::debugHashKey).c_str());

	maxFluidXPos = 0.0f;
	maxFluidYPos = 0.0f;
	maxFluidZPos = 0.0f;

	bool b = true;
	ImGui::Begin("Timer", &b);
	float t;
	float tot = 0;
	Performance pf;

	{
		pf.Start();
		updateNeighbor();
		t = pf.End();
		tot += t;
		string str = "updateNeighbor : \n" + to_string(t) + " ms";
		ImGui::Text(str.c_str());
	}
	{
		pf.Start();
		computeDensityAndPressure();
		t = pf.End();
		tot += t;
		string str = "computeDensityAndPressure : \n" + to_string(t) + " ms";
		ImGui::Text(str.c_str());
	}
	{
		pf.Start();
		computeFluidAccleration();
		t = pf.End();
		tot += t;
		string str = "computeFluidAcceleration : \n" + to_string(t) + " ms";
		ImGui::Text(str.c_str());
	}
	{
		pf.Start();
		computeVelocityAndPosition();
		t = pf.End();
		tot += t;
		string str = "computeVelocityAndPosition : \n" + to_string(t) + " ms";
		ImGui::Text(str.c_str());
	}
	{
		pf.Start();
		updateRenderParticleData();
		t = pf.End();
		tot += t;
		string str = "updateRenderParticleData : \n" + to_string(t) + " ms";
		ImGui::Text(str.c_str());
	}
	ImGui::Text("total time : \n%f ms", tot);

	ImGui::End();

}

void SESPHFluidThread::Render()
{
	string msg;


	ImGui::Text("delta time : %f ms", _deltaTime);
	ImGui::Separator();
	ImGui::Text("fluid particle count : %d", _fluidParticles.size());
	ImGui::Text("boundary particle count : %d", _obstacleCount);
	ImGui::Separator();
	ImGui::Text("particle mass : %f", _mass);
	ImGui::Text("support radius : %f", _h);
	ImGui::Text("particle size : %f", _renderRadius);
	ImGui::Separator();
	ImGui::Text("cell size : %f ", _cellSize);
	ImGui::Text("max cell counts : %d", _maxCellCount);
	ImGui::Separator();
	ImGui::Text("average density: %f", _sumDensity / _particleCount);
	ImGui::Text("average Pressure: %f", _sumPressure / _particleCount);
	ImGui::Text("average DisplaceMentLength: %f", _debugDisplacement / _particleCount);
	ImGui::Text("average PressureForceLength: %f", _debugPressureForce / _particleCount);

	ImGui::Separator();

	ImGui::Text("defaultKernelCoefficient : %f", _defaultKernelCoefficient);
	ImGui::Text("pressureKernelCoefficient : %f", _pressureKernelCoefficient);
	ImGui::Text("viscosityKernelCoefficient : %f", _viscosityKernelCoefficient);
	ImGui::Separator();


	ImGui::Text("boundary X length : %f", _xmax - _xmin);
	ImGui::Text("boundary Y length : %f", _ymax - _ymin);
	ImGui::Text("boundary Z length : %f", _zmax - _zmin);
}

void SESPHFluidThread::updateNeighbor()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	vector<std::thread> simThreads;

	auto func = std::bind(&SESPHFluidThread::executeNeighborThreadFunction, this, std::placeholders::_1, std::placeholders::_2);

	UINT group = THREAD_GROUP_SIZE;
	UINT threadSize = fsize / group;
	if (fsize % threadSize > 0)
	{
		group++;
	}
	for (UINT i = 0; i < group; i++)
	{
		UINT it = i * threadSize;
		UINT endit = (i + 1) * threadSize;

		simThreads.push_back(thread(func, it, endit));
	}
	for (UINT i = 0; i < group; i++)
	{
		simThreads[i].join();
	}

}

void SESPHFluidThread::executeNeighborThreadFunction(UINT it, UINT endit)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	for (it; it < endit && it < fsize; it++)
	{
		pi = _fluidParticles[it];
		_spatialHash.findNeighbor(*pi, _particleNeighbor[it]);
	}
}


void SESPHFluidThread::computeDensityAndPressure()
{
	UINT fsize = _fluidParticles.size();

	vector<std::thread> simThreads;

	auto func = std::bind(&SESPHFluidThread::executeDensityAndPressureThreadFunction, this, std::placeholders::_1, std::placeholders::_2);
	UINT group = THREAD_GROUP_SIZE;
	UINT threadSize = fsize / group;
	if (fsize % threadSize > 0)
	{
		group++;
	}
	for (UINT i = 0; i < group; i++)
	{
		UINT it = i * threadSize;
		UINT endit = (i + 1) * threadSize;
		simThreads.push_back(thread(func, it, endit));
	}
	for (UINT i = 0; i < group; i++)
	{
		simThreads[i].join();
	}

}

void SESPHFluidThread::executeDensityAndPressureThreadFunction(UINT it, UINT endit)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	Vector3 rVec;

	float r;


	for (it; it < endit && it < fsize; it++)
	{

		float debug = 0;

		pi = _fluidParticles[it];
		pi->density = 0;
		Vector3 numerator = { 0,0,0 };
		float denominator = 0;


		for (Cell * neighbor : _particleNeighbor[it])
		{
			for (Particle* & pj : neighbor->particles)
			{
				rVec = pi->position - pj->position;
				r = D3DXVec3Length(&rVec);				
				if (r <= _h)
				{
					////////////////////////////////////////////////////////////////////////////////////////////////
					// 범위 내 fluid 와 boundary 를 구분하여 밀도 누적값을 저장한다.
					////////////////////////////////////////////////////////////////////////////////////////////////
					if (pj->pType == ParticleTypeFluid)
					{
						pi->density += LookUpWCubic[lookupCubicKey(r)] * pj->mass;
					}
					else
					{
						pi->density += VOLUME_COFFICIENT * LookUpWCubic[lookupCubicKey(r)] * pj->mass;
						
					}
					numerator += pj->mass / _idealDensity * LookUpWCubic_Grad[lookupCubicKey(r)] * rVec;
					denominator += pj->mass / _idealDensity * LookUpWCubic[lookupCubicKey(r)];

				}
				
					debug++;
			}
		}	
		//////////////////////////////////////////////////////////////////////////
		// 상태 방정식의 gas stiffnesses 고려하여 압력을 도출한다.
		//////////////////////////////////////////////////////////////////////////
		// k ( pi - p0 )
		//pi->pressure = _pressureCoefficient * ( powf( pi->density / _idealDensity , 5.0f)- 1.0f);
		pi->pressure = _pressureCoefficient * (powf(pi->density / _idealDensity, 2.0f) - 1.0f);


		if (pi->pressure < 0.0f) pi->pressure = 0.0f;

		pi->color = numerator / fmaxf(Math::EPSILON, denominator);

		debugBuffer[it] = debug;
	}
}

void SESPHFluidThread::computeFluidAccleration()
{
	UINT fsize = _fluidParticles.size();

	vector<std::thread> simThreads;

	auto func = std::bind(&SESPHFluidThread::executeFluidAcclerationThreadFunction, this, std::placeholders::_1, std::placeholders::_2);
	UINT group = THREAD_GROUP_SIZE;
	UINT threadSize = fsize / group;
	if (fsize % threadSize > 0)
	{
		group++;
	}
	for (UINT i = 0; i < group; i++)
	{
		UINT it = i * threadSize;
		UINT endit = (i + 1) * threadSize;
		simThreads.push_back(thread(func, it, endit));
	}
	for (UINT i = 0; i < group; i++)
	{
		simThreads[i].join();
	}
}

void SESPHFluidThread::executeFluidAcclerationThreadFunction(UINT it, UINT endit)
{
	Particle* pi = nullptr;

	UINT fsize = _fluidParticles.size();	
	Vector3 rVec;

	Vector3 pressureForce;
	Vector3 viscosityForce;
	Vector3 surfaceForce;

	Vector3 diffJI;
	float  r;


	for (it; it < fsize && it < endit; it++)
	{
		pi = _fluidParticles[it];

		pi->acceleration = { 0,0,0 };
		pressureForce = { 0.0f, 0.0f, 0.0f };
		viscosityForce = { 0.0f, 0.0f, 0.0f };
		surfaceForce = { 0,0,0 };

		for (Cell* neighbor : _particleNeighbor[it])
		{
			for (Particle* & pj : neighbor->particles)
			{
				rVec = pi->position - pj->position;
				r = D3DXVec3Length(&rVec);

				if (r > _h) continue;

				const float && color_cof = 0.001f;
				const float && air_pressure_cof = 0.001f;
				const float & cubic_gradient_kernel_scalar = LookUpWCubic_Grad[lookupCubicKey(r)];

				if (pj->pType == ParticleTypeFluid)
				{
					/////////////////////////////////////////////////////////////////////
					// surface tension
					/////////////////////////////////////////////////////////////////////
					surfaceForce -= 0.25f * pj->mass / (_idealDensity * _idealDensity) * color_cof
						* (D3DXVec3Dot(&pi->color, &pi->color) + D3DXVec3Dot(&pj->color, &pj->color))
						* WG_Surface(rVec, _h);
					/////////////////////////////////////////////////////////////////////
					// air pressure
					/////////////////////////////////////////////////////////////////////
					surfaceForce += air_pressure_cof * pj->mass / (_idealDensity * _idealDensity)
						*  cubic_gradient_kernel_scalar * rVec
						* D3DXVec3Length(&pi->color) / fmaxf(Math::EPSILON, D3DXVec3Length(&pi->color));
					/////////////////////////////////////////////////////////////////////
					// compute internal force [ viscosity ]
					/////////////////////////////////////////////////////////////////////
					diffJI = pj->velocity - pi->velocity;
					viscosityForce += pj->mass * WL_Viscosity(r,_h) * diffJI;
					/////////////////////////////////////////////////////////////////////
					// compute internal force [ pressure ]
					/////////////////////////////////////////////////////////////////////
					float pterm = pj->mass * (pi->pressure / fmaxf(Math::EPSILON, pi->density * pi->density)) + (pj->pressure / fmaxf(Math::EPSILON, pj->density * pj->density));
					pressureForce += pterm * cubic_gradient_kernel_scalar * rVec;
				}
				else if (pj->pType == ParticleTypeSolid)
				{
					float pterm = pj->mass * (pi->pressure / fmaxf(Math::EPSILON, pi->density * pi->density));
					pressureForce += pterm * cubic_gradient_kernel_scalar * rVec;
				}
			}
		}

		// internal force [pressure + viscosity]
		pi->acceleration -= pressureForce ;
		pi->acceleration += _viscosityCoefficient * viscosityForce / fmaxf(Math::EPSILON,pi->density);

		// external force [gravity]
		pi->acceleration += gravity;
		// external force [surface tension ]
		pi->acceleration += surfaceForce;
		
		pi->color = surfaceForce;

		float alen = D3DXVec3Length(&pi->acceleration);
		if (alen > ACC_MAX)
		{
			pi->acceleration /= alen;
			pi->acceleration *= ACC_MAX;
		}
	}
}

void SESPHFluidThread::computeVelocityAndPosition()
{
	Particle * particle = nullptr;
	Vector3 newPosition;
	Vector3 t;

	float avgvelMag = 0;
	float avgPre = 0.0f;

	_debugDisplacement = 0.0f;

	UINT fsize = _fluidParticles.size();
	for (UINT i = 0; i < fsize; i++)
	{
		particle = _fluidParticles[i];
		particle->velocity = particle->velocity + particle->acceleration * _deltaTime;

		float vlen = D3DXVec3Length(&particle->velocity);
		if (vlen > V_MAX)
		{
			particle->velocity /= vlen;
			particle->velocity *= V_MAX;
		}
		avgvelMag += vlen;
		avgPre += particle->pressure;

		t = particle->velocity * _deltaTime;
		_debugDisplacement += D3DXVec3Length(&t);

		newPosition = particle->position + particle->velocity * _deltaTime;

		enforceBoundaryPosition(particle, newPosition);
		_spatialHash.moveParticleInHash(particle, newPosition);
	}

	ImGui::Text("average velocity : %f", avgvelMag / fsize);
	ImGui::Text("average Pressure : %f ", avgPre / fsize);
}

void SESPHFluidThread::enforceBoundaryPosition(Particle* particle, Vector3 & position)
{
	float dampWeight = 0.99f;
	if (position.x < _xmin)
	{
		position.x = _xmin + Math::EPSILON;
		particle->velocity.x = -particle->velocity.x * dampWeight;
	}
	else if (position.x > _xmax)
	{
		position.x = _xmax - Math::EPSILON;
		particle->velocity.x = -particle->velocity.x * dampWeight;
	}

	if (position.y < _ymin)
	{
		position.y = _ymin + Math::EPSILON;
		particle->velocity.y = -particle->velocity.y * dampWeight;
	}
	else if (position.y > _ymax)
	{
		position.y = _ymax - Math::EPSILON;
		particle->velocity.y = -particle->velocity.y * dampWeight;
	}

	if (position.z < _zmin)
	{
		position.z = _zmin + Math::EPSILON;
		particle->velocity.z = -particle->velocity.z * dampWeight;
	}
	else if (position.z > _zmax)
	{
		position.z = _zmax - Math::EPSILON;
		particle->velocity.z = -particle->velocity.z * dampWeight;
	}
}

SESPHFluidThread::~SESPHFluidThread()
{
	for (Particle*p : _allParticles) SafeDelete(p);
}

UINT SESPHFluidThread::calculateCellMax()
{
	return (_particleCount + _obstacleCount) * 3;
}


void SESPHFluidThread::updateRenderParticleData()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();
	float vmax = 0.0f;
	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		_particleRenderDatas[i].position = pi->position;
		D3DXVec3Normalize(&_particleRenderDatas[i].color, &pi->color);
		//_particleRenderDatas[i].color.x = pi->density / _idealDensity;
		vmax = max(vmax, D3DXVec3Length(&pi->velocity));
	}

	float c_time = TIME_WEIGHT * _h * 2.0f / vmax;
	c_time = min(c_time, TIME_MAX);
	c_time = max(c_time, TIME_MIN);
	_deltaTime = c_time;
	//_deltaTime = 0.0001f;


	UINT ps = _spatialHash.getInnerParticleCount();
	string st = "inner particle count : " + to_string(ps);
	ImGui::Text(st.c_str());


}