#include "Framework.h"
#include <time.h>
#include <random>
#include "Physics/Data/FluidResourceManager.h"
#include "Physics/ISPH/DFSPHFluid.h"


#define OFFSET_UNIT (1000.0f)
#define CREATE_OFFSET (2.0f) // 생성 간격 (크기는 h )

#define CALC_PCOUNT (_particleCount * 1.0f)
#define MASS_WEIGHT (1.0f)

#define ACC_MAX (155.0f)
#define V_MAX (155.0f)

#define DELTA_TIME_MIN ( 0.001f)
#define DELTA_TIME_MAX ( 0.005f)


#define CREATE_COUNT_FLOAT (10.0f)
#define CREATE_COUNT_UINT (10)
#define RENDER_WEIGHT (  20.0f / CREATE_COUNT_FLOAT)

DFSPHFluid::DFSPHFluid()
	:
	_viscosityCoefficient(0.008f),
	_surfaceTensionCoefficeint(0.015f),
	_surfaceTensionThreshold(5.0f),

	_deltaTime(0.01f),
	_cflFactor(1.0f),
	_cflMaxSize(_deltaTime),
	_cflMinSize(_deltaTime / 10.0f),

	_idealDensity(2000.0f),
	_densityThreshold(50.0f),

	_DpDtThreshold(15.0f),

	_particleCount(CREATE_COUNT_UINT * CREATE_COUNT_UINT * CREATE_COUNT_UINT),
	_obstacleCount(0),
	_N(500.0f),
	_X(20.0f),
	// boundary
	_xmin(+0.0f),
	_xmax(+4.2f),
	_ymin(+0.0f),
	_ymax(+4.0f),
	_zmin(+0.0f),
	_zmax(+4.2f),
	_spatialHash(),
	_pi(3.1415926535897f)
	
{	
	Initialize();
}



void DFSPHFluid::Initialize()
{
	initState();
	initFluidData();
    //initBoundaryData();
	initConstantState();
	initSpatialHash();
}

void DFSPHFluid::initState()
{
	///////////////////////////////////////////////////
	// volume and particle Size init;
	///////////////////////////////////////////////////
	_renderRadius = (0.8641f / 20.0f) * RENDER_WEIGHT;
	Vector3 vol = {
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET),
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET),
					CREATE_COUNT_FLOAT * (_renderRadius * CREATE_OFFSET)
	};
	_totalVolume = vol.x * vol.y * vol.z;
	///////////////////////////////////////////////////
	// radius init;
	///////////////////////////////////////////////////
	_h = cbrt(
		(3.0f * _totalVolume * _X)
		/ (4.0f * _pi * CALC_PCOUNT));
	_cellSize = _h;
	_particleRadius = _h;
	_spatialHash.setCellSize(_cellSize);
	_invH = 1.0f / _h;
	///////////////////////////////////////////////////
	// mass init;
	///////////////////////////////////////////////////
	_mass = static_cast<float>(_idealDensity * (_totalVolume / _particleCount)) * MASS_WEIGHT;
	
}

void DFSPHFluid::initConstantState()
{
	_invH = 1.0f / _h;
	_h2 = _h * _h;	

	SetCoefficient(_h);


	//float testW[1000];
	//float testWG[1000];
	//float testWGG[1000];
	//float testDF[1000];
	//float testSp[1000];
	//
	//float offset = _h / 1000.0f;
	//float r = 0.0f;
	//for (UINT i = 0; i < 1000; i++)
	//{
	//	r = offset * static_cast<float>(i);
	//	testW[i] = W_Cubic(r , _h);
	//	//testWG[i] = WG_Cubic(r , _h);
	//	testDF[i] = W_Default((_h *_h -  r * r));
	//	testSp[i] = -WG_Spiky((_h - r));
	//	testWGG[i] = W_Viscosity(_h - r);
	//}
	//int a = 0;
}



void DFSPHFluid::InitSimulation()
{
	updateNeighbor();
	computeDensityAndKFactor();
}

void DFSPHFluid::Update()
{
	ImGui::Text(to_string(FluidResourceManager::debugHashKey).c_str());

	bool b = true;
	ImGui::Begin("Timer", &b);

	float currentTimes = _deltaTime;
	_timeStep = DELTA_TIME_MIN;
	UINT iter = 0;

	Performance pf;

	float p1 = 0.0f;
	float p2 = 0.0f;
	float p3 = 0.0f;
	float p4 = 0.0f;
	float p5 = 0.0f;
	float p6 = 0.0f;


	while (currentTimes > 0.0f)
	{
		pf.Start();
		computeNonePressureAcceleration();
		_timeStep = adaptTimestepSizeByCflCondition(_timeStep);
		p1 += pf.End();


		pf.Start();
		computeConstantDensitySolver(_timeStep); // predict velocity by non-pressure force 가 포함 되어 있다.
		p2 += pf.End();

		pf.Start();
		computePosition(_timeStep);
		p3 += pf.End();

		pf.Start();
		updateNeighbor();
		p4 += pf.End();

		pf.Start();
		computeDensityAndKFactor();
		p5 += pf.End();

		pf.Start();
		computeDivergenceFreeSolver(_timeStep); // update velocity 가 포함 되어 있다.
		p6 += pf.End();
		currentTimes -= _timeStep;
		iter++;
	}	
	

	ImGui::Text("correct iter : %d", iter);
	ImGui::Text("p1 : %f ms", p1);
	ImGui::Text("p2 : %f ms", p2);
	ImGui::Text("p3 : %f ms", p3);
	ImGui::Text("p4 : %f ms", p4);
	ImGui::Text("p5 : %f ms", p5);
	ImGui::Text("p6 : %f ms", p6);
	ImGui::Text("total time : %f ms", p1 + p2 + p3 + p4 + p5 + p6);


	updateRenderParticleData();
	ImGui::End();

}

void DFSPHFluid::Render()
{
	string msg;

	ImGui::Text("fluid particle count : %d", _fluidParticles.size());
	ImGui::Separator();
	ImGui::Text("particle mass : %f", _mass);
	ImGui::Text("support radius : %f", _h);
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
	ImGui::Text("viscosityKernelCoefficient : %f" ,_viscosityKernelCoefficient);
	ImGui::Separator();


	ImGui::Text("boundary X length : %f", _xmax - _xmin);
	ImGui::Text("boundary Y length : %f", _ymax - _ymin);
	ImGui::Text("boundary Z length : %f", _zmax - _zmin);
}



void DFSPHFluid::computeDensityAndKFactor()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	Vector3 rVec;
	Vector3 rWGVec;
	float r;

	Vector3 kFactorTerm1 = { 0.0f ,0.0f, 0.0f };
	float	kFactorTerm2 = 0.0f;
	float diff;
	for (UINT i = 0; i < fsize; i++)
	{
		/////////////////////////////////////////////////////////////
		// init particle data
		/////////////////////////////////////////////////////////////
		pi = _fluidParticles[i];
		pi->density = 0.0f;
		kFactorTerm2 = 0.0f;
		kFactorTerm1 = { 0.0f ,0.0f, 0.0f };

		for (Particle* & pj : _particleNeighborDataList[i])
		{
			rVec = pi->position - pj->position;
			r = D3DXVec3Length(&rVec);
			diff = _h - r;
			if (r <= _h)
			{
				/////////////////////////////////////////////////////////////
				// density 구하기
				/////////////////////////////////////////////////////////////
				pi->density += W_Cubic(r, _h) * pj->mass;
				/////////////////////////////////////////////////////////////
				// k factor 항 구하기
				/////////////////////////////////////////////////////////////
				rWGVec = WG_Cubic(rVec, _h) * pj->mass;
				// sum( mj A* gradW);
				kFactorTerm1 += rWGVec;
				// sum( square( mj * gradW ) )
				kFactorTerm2 += D3DXVec3Dot(&rWGVec, &rWGVec);
			}
		}
		
		// max(pi , p0 )		
		pi->density = max(pi->density, _idealDensity);
		/////////////////////////////////////////////////////////////
		// k factor 두 항 값 합치기
		/////////////////////////////////////////////////////////////
		//kFactorTerm2 += D3DXVec3Dot(&kFactorTerm1, &kFactorTerm1);
		kFactorTerm2 += D3DXVec3Length(&kFactorTerm1);

		if (kFactorTerm2 > Math::EPSILON)
		{
			pi->kFactor = pi->density / kFactorTerm2;
		}
		else
		{
			pi->kFactor = 0.0f;
		}

	}
}

void DFSPHFluid::computeNonePressureAcceleration()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	Vector3 rVec;
	Vector3 viscosityForce;
	Vector3 diffJI;
	float  r;
	
	/////////////////////////////////////////////////////////////////////////
	// 비압축력 계산하기
	/////////////////////////////////////////////////////////////////////////
	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		viscosityForce = { 0.0f, 0.0f, 0.0f };

		for (Particle* & pj : _particleNeighborDataList[i])
		{
			rVec = pi->position - pj->position;
			r = D3DXVec3Length(&rVec);

			if (r < 0.000001f || r > _h)
			{
				continue;
			}
			// 점성력 계산
			diffJI = pj->velocity - pi->velocity;
			viscosityForce += (pj->mass / pj->density) * W_Viscosity(_h - r) * diffJI;
		}
		/////////////////////////////////////////////////////////////////////////
		// 정성력 적용
		/////////////////////////////////////////////////////////////////////////
		pi->acceleration = _viscosityCoefficient * viscosityForce / pi->density;
		/////////////////////////////////////////////////////////////////////////
		// 중력 적용
		/////////////////////////////////////////////////////////////////////////
		pi->acceleration += gravity;
		//pi->acceleration += computeObstacleAccleration(pi);
	}
}

float DFSPHFluid::adaptTimestepSizeByCflCondition(const float dt)
{
	float timestep;

	float diameter = _h * 2.0f;

	float vMax = 0.0f;

	UINT fsize = _fluidParticles.size();
	for (UINT i = 0; i < fsize; i++)
	{
		Particle & pi = *_fluidParticles[i];
		Vector3 v = pi.velocity + pi.acceleration * dt;
		vMax = max(vMax, D3DXVec3Dot(&v, &v));
	}

	timestep = _cflFactor * 0.8f * (diameter / sqrtf(vMax));

	timestep = min(timestep, DELTA_TIME_MAX);
	timestep = max(timestep, DELTA_TIME_MIN);

	return timestep;
}

void DFSPHFluid::computeConstantDensitySolver(const float dt)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();	

	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		pi->velocity += pi->acceleration * dt;
	}


	UINT iter = 0;

	float history[5] = { 0 };
	float maxP = 0.0f, minP = 10000.0f;


	//////////////////////////////////////////////////
	// constant density solver;
	//////////////////////////////////////////////////
	do
	{
		PredictDensityAndPressure(dt);		
		CorrectVelocityByPressure(dt);	
		// 해가 수렴해가지는 확인하기 위한 디버그 배열
		//history[iter] = _avgPredictDensity;
		//if (iter > 5) { assert(false); }
		iter++;
	} 
	while (_avgPredictDensity - _idealDensity > 30  || iter < 2);
	//while (false);
	int a = 0;

}

void DFSPHFluid::PredictDensityAndPressure(const float dt)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();
	float invTs2 = 1.0f / (dt * dt);
	float pTerm;
	float r;
	Vector3 vDiff;
	Vector3 vWG;
	float maxP = 0.0f, minP = 10000.0f;

	//////////////////////////////////////////////////
	// compute predict density
	//////////////////////////////////////////////////

	_avgPredictDensity = 0.0f;

	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];

		_predictDensity[i] = pi->density;

		pTerm = 0.0f;
		for (Particle * & pj : _particleNeighborDataList[i])
		{
			// predict density		
			vWG = pi->position - pj->position;
			r = D3DXVec3Length(&vWG);
			if (r  <= _h)
			{
				vDiff = pi->velocity - pj->velocity;
				pTerm += D3DXVec3Dot(&vDiff, &WG_Cubic(vWG, _h)) * pj->mass;
			}			
		}
		_predictDensity[i] += pTerm * dt;
		_predictDensity[i] = max(_predictDensity[i], _idealDensity);

		_avgPredictDensity += _predictDensity[i];

		pi->pressure = invTs2 * (_predictDensity[i] - _idealDensity) * pi->kFactor / pi->density;

		maxP = max(maxP, pi->pressure);
		minP = min(minP, pi->pressure);
	}

	_avgPredictDensity /= static_cast<float>(fsize);
}

void DFSPHFluid::CorrectVelocityByPressure(const float dt)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();
	float pTerm;
	float r;
	Vector3 vWG;
	//////////////////////////////////////////////////
	// compute correct velocity
	//////////////////////////////////////////////////
	for (UINT i = 0; i < fsize; i++)
	{
		Vector3 correctVector = { 0.0f, 0.0f, 0.0f };

		pi = _fluidParticles[i];
		pTerm = 0.0f;

		float piTerm = pi->pressure / pi->density;

		for (Particle * & pj : _particleNeighborDataList[i])
		{
			///////////////////////////////////////////////////////////////
			// correct velocity by predict density;
			///////////////////////////////////////////////////////////////
			vWG = pi->position - pj->position;
			r = D3DXVec3Length(&vWG);
			if ( r > _h)
			{
				continue;
			}
			/////////////////////////////////////////////////////////////////
			// compute pressure by k Factor 
			/////////////////////////////////////////////////////////////////
			vWG = WG_Cubic(pi->position - pj->position, _h);

			if (pj->pType == ParticleTypeFluid)
			{
				correctVector += pj->mass * (
					piTerm
					+
					(pj->pressure / (pj->density * pj->density))
					)
					* vWG;
			}
			else
			{
				correctVector += pi->mass * 
					piTerm
					* vWG;
			}

			if (isnan(correctVector.x) || isnan(correctVector.y) || isnan(correctVector.z))
			{
				assert(false);
			}
		}
		pi->velocity -= correctVector * dt;
	}
}

void DFSPHFluid::computePosition(const float dt)
{
	float debugLen = 0.0f;


	Particle * particle = nullptr;
	Vector3 newPosition;

	UINT fsize = _fluidParticles.size();
	for (UINT i = 0; i < fsize; i++)
	{
		particle = _fluidParticles[i];
		float vlen = D3DXVec3Length(&particle->velocity);
		if (vlen > V_MAX)
		{
			particle->velocity /= vlen;
			particle->velocity *= V_MAX;
		}

		newPosition = particle->velocity * dt;
		debugLen += D3DXVec3Length(&newPosition);
		newPosition += particle->position;
		enforceBoundaryPosition(particle, newPosition);	
		_spatialHash.moveParticleInHash(particle, newPosition);

	}

	//ImGui::Text("p avg len : %f ", debugLen / fsize);
}

void DFSPHFluid::computeDivergenceFreeSolver(const float dt)
{
	UINT iter = 0;
	float history[5] = { 0 };
	do 
	{
		PredictDFSovlerPressure(dt);
		CorrectVelocityByDFSovlerPressure(dt);
		//history[iter] = _DpDtAvg;
		
		//if (iter > 5) { assert(false); }
		iter++;
	} 
	while (_DpDtAvg > 50.0f || iter < 1);
	//while (false);
	//while (DpDtAvg > _DpDtThreshold || iter < 1);
}

void DFSPHFluid::PredictDFSovlerPressure(const float dt)
{
	_DpDtAvg = 0.0f;

	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	float invts = 1.0f / dt;
	float invts2 = 1.0f / (dt*dt);

	Vector3 rVec;
	Vector3 vdiff;
	float  r;

	/////////////////////////////////////////////////////////////
	// the time derivative of the density
	/////////////////////////////////////////////////////////////
	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		_DFSolverValues[i] = 0.0f;

		for (Particle* & pj : _particleNeighborDataList[i])
		{

			rVec = pi->position - pj->position;
			r = D3DXVec3Length(&rVec);
			if (r < Math::EPSILON || r > _h)
			{
				continue;
			}
			vdiff = pi->velocity - pj->velocity;
			_DFSolverValues[i] += pj->mass * D3DXVec3Dot(&vdiff, &WG_Cubic(rVec, _h));

		} // for pj
		if (isnan(_DFSolverValues[i]))
		{
			assert(false);
		}
		_DFSolverPressureValues[i] = invts * _DFSolverValues[i] * pi->kFactor * pi->density;
		if (isnan(_DFSolverPressureValues[i]))
		{
			assert(false);
		}
		_DpDtAvg += _DFSolverValues[i];
	}
	_DpDtAvg = _DpDtAvg / static_cast<float>(fsize);
}

void DFSPHFluid::CorrectVelocityByDFSovlerPressure(const float dt)
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	float invts = 1.0f / dt;
	float invts2 = 1.0f / (dt*dt);

	Vector3 rVec;
	Vector3 vdiff;
	float  r;

	Vector3 correctVector;

	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		correctVector = { 0.0f, 0.0f , 0.0f };

		for (Particle* & pj : _particleNeighborDataList[i])
		{
			rVec = pi->position - pj->position;
			r = D3DXVec3Length(&rVec);

			if (r < Math::EPSILON || r > _h)
			{
				continue;
			}
			rVec = WG_Cubic(rVec, _h);

			if (pj->pType == ParticleTypeFluid)
			{
				correctVector += pj->mass *
					((_DFSolverPressureValues[i] / (pi->density * pi->density))
						+
						(_DFSolverPressureValues[pj->id] / (pj->density * pj->density))
						)
					* rVec;
			}
			else if (pj->pType == ParticleTypeSolid)
			{
				correctVector += pj->mass *
					((_DFSolverPressureValues[i] / (pi->density * pi->density))
						+
						(_DFSolverPressureValues[i] / (_idealDensity * _idealDensity))
						)
					* rVec;
			}

			if (isnan(correctVector.x) || isnan(correctVector.y) || isnan(correctVector.z))
			{
				assert(false);
			}
		} // for pj

		pi->velocity -= correctVector * dt;
	}
}



void DFSPHFluid::enforceBoundaryPosition(Particle* particle ,Vector3 & position)
{
	float dampWeight = 0.91f;
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

DFSPHFluid::~DFSPHFluid()
{
	for (Particle*p : _allParticles) SafeDelete(p);
}

UINT DFSPHFluid::calculateCellMax()
{
	return ( _particleCount + _obstacleCount ) * 3;
}

void DFSPHFluid::updateNeighbor()
{
	Particle* pi = nullptr;
	UINT psize = _fluidParticles.size();
	for (UINT i = 0; i < psize; i++)
	{
		pi = _fluidParticles[i];		
		_spatialHash.findNeighborList(*pi, _particleNeighborDataList[i], _h);
	}
}

void DFSPHFluid::updateRenderParticleData()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();
	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];
		_particleRenderDatas[i].position = pi->position;
		_particleRenderDatas[i].color = pi->color;
	}

	UINT ps = _spatialHash.getInnerParticleCount();
	string st = "inner particle count : " + to_string(ps);
	ImGui::Text(st.c_str());
}


Particle * DFSPHFluid::createParticle(Vector3 position, Vector3 velocity, float mass)
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

PointID DFSPHFluid::generateUniquePointID()
{
	return particleCurrentIdToGenerate++;
}

void DFSPHFluid::initFluidData()
{
	createFluidParticle();
}

void DFSPHFluid::initSpatialHash()
{
	///////////////////////////////////////////////////////////////
	// Simulation Data
	///////////////////////////////////////////////////////////////
	_predictDensity.resize(_particleCount );
	_DFSolverValues.resize(_particleCount );

	_DFSolverPressureValues.resize(_particleCount);

	///////////////////////////////////////////////////
	// SPATHIAL HASH
	///////////////////////////////////////////////////
	_particleNeighbor.resize(_particleCount);
	_particleNeighborDataList.resize(_particleCount);

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

void DFSPHFluid::createFluidParticle()
{
	// 정의한 공간내에 일정 구획이 모여있는 파티클 집단 중력에 의해 떨어질 것이다.
	float fxmin = _xmin + _renderRadius;
	float fxmax = _xmax;
	float fymin = _ymin + _renderRadius;
	float fymax = _ymax;
	float fzmin = _zmin + _renderRadius ;
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
					fxmin + (float)ii * (_renderRadius * CREATE_OFFSET),
					fymin + (float)jj * (_renderRadius * CREATE_OFFSET),
					fzmin + (float)kk * (_renderRadius * CREATE_OFFSET)
				};
				//Vector3 velocity(Math::RandomVec3(-1.0f, 1.0f));
				Vector3 velocity(0.0f, 0.0f, 0.0f);
				Particle* newParticle = createParticle(position, velocity, _mass);

				_fluidParticles.push_back(newParticle);
			}
		}
	}


	_fluidParticles.shrink_to_fit();
	_particleRenderDatas.shrink_to_fit();
}

void DFSPHFluid::initBoundaryData()
{
	createBounaryParticle();
}

void DFSPHFluid::createBounaryParticle()
{
	UINT x = static_cast<UINT>((_xmax - _xmin) / (_renderRadius * CREATE_OFFSET));
	UINT y = static_cast<UINT>((_ymax - _ymin) / (_renderRadius * CREATE_OFFSET));
	UINT z = static_cast<UINT>((_zmax - _zmax) / (_renderRadius * CREATE_OFFSET));

	Vector3 xdir = { 1.0f,0.0f,0.0f };
	Vector3 ydir = { 0.0f,1.0f,0.0f };
	Vector3 zdir = { 0.0f,0.0f,1.0f };

	createBoundaryPlane(_xmax, _zmax, xdir, zdir);
}

void DFSPHFluid::createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2)
{
	float r2 = _renderRadius * 2.0f;
	Vector3 velocity = { 0 ,0 ,0 };

	
	UINT count = 0;
	for (float x = 0; x < _xmax; x += r2)
	{
		for (float z = 0; z < _zmax; z += r2)
		{
			Vector3 position = { x, 0 , z };
			Particle* newParticle = createParticle(position, velocity, _mass);
			newParticle->density = _idealDensity;
			newParticle->pressure = 0;
			newParticle->pType = ParticleTypeSolid;
			_obstacleParticles.push_back(newParticle);
			count++;
		}
	}
	_obstacleCount = count;
	int a = 0;
}
