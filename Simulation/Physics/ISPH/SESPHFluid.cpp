#include "Framework.h"
#include <time.h>
#include <random>
#include "Physics/Data/FluidResourceManager.h"
#include "Physics/ISPH/SESPHFluid.h"

#define OFFSET_UNIT (1000.0f)
#define CREATE_OFFSET (2.0f)

#define CALC_PCOUNT (_particleCount * 1.0f)
#define MASS_WEIGHT (1.0f)

#define ACC_MAX (75.0f)
#define V_MAX (75.0f)

SESPHFluid::SESPHFluid()
	:

	_viscosityCoefficient(0.018f),
	_pressureCoefficient(100000.35f),// gas stiffness(k);
	_surfaceTensionCoefficeint(0.015f),
	//_surfaceTensionCoefficeint(0.075f),
	_surfaceTensionThreshold(5.0f),

	_deltaTime(0.016f),
	_idealDensity(2000.0f),

	_particleCount(8000),
	_N(500.0f),
	_X(20.0f),
	// boundary
	_xmin(+0.0f),
	_xmax(+2.2f),
	_ymin(+0.0f),
	_ymax(+4.0f),
	_zmin(+0.0f),
	_zmax(+2.2f),
	_spatialHash(),
	_pi(3.1415926535897f)
	
{	
	Initialize();
}



void SESPHFluid::Initialize()
{
	initState();
	initFluidData();
	initBoundaryData();
	initConstantState();
}

void SESPHFluid::initState()
{
	//_renderRadius = 0.023207944168f;
	_renderRadius = 0.4641f / 20.0f;

	Vector3 vol = {
					20.0f * (_renderRadius * CREATE_OFFSET),
					20.0f * (_renderRadius * CREATE_OFFSET),
					20.0f * (_renderRadius * CREATE_OFFSET)
	};

	_totalVolume = vol.x * vol.y * vol.z;


	// radius init;
	_h = cbrt(
		(3.0f * _totalVolume * _X)
		/ (4.0f * _pi * CALC_PCOUNT));

	//_h = _renderRadius * 4.0f;
	_cellSize = _h;
	_particleRadius = _h;
	_spatialHash.setCellSize(_cellSize);

	// mass init;
	_mass = static_cast<float>(_idealDensity * (_totalVolume / _particleCount)) * MASS_WEIGHT;

	//_renderRadius = cbrt((3.0f * _mass) / ( _pi * 4.0f * _idealDensity ));

	// SPATHIAL HASH
	_maxCellCount = calculateCellMax();
	_spatialHash.resizeFreeCells(_maxCellCount);

}

void SESPHFluid::initConstantState()
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
	
}


Particle * SESPHFluid::createParticle(Vector3 position, Vector3 velocity, float mass)
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

PointID SESPHFluid::generateUniquePointID()
{
	return particleCurrentIdToGenerate++;
}

void SESPHFluid::initFluidData()
{
	createFluidParticle();
}

void SESPHFluid::createFluidParticle()
{
	// 정의한 공간내에 일정 구획이 모여있는 파티클 집단 중력에 의해 떨어질 것이다.
	float fxmin = _xmin + 0.2f ;
	float fxmax = _xmax ;
	float fymin = _ymin + 0.5f ;
	float fymax = _ymax ;
	float fzmin = _zmin + 0.2f ;
	float fzmax = _zmax ;

	_fluidParticles.reserve(_particleCount);
	_particleRenderDatas.resize(_particleCount);
	_particleNeighbor.resize(_particleCount);

	srand((unsigned int)time(NULL));

	UINT ii;
	UINT jj;
	UINT kk;
	for (ii = 0; ii < 20; ii++)
	{
		for (jj = 0; jj < 20; jj++)
		{
			for (kk = 0; kk < 20; kk++)
			{
				Vector3 position = {
					fxmin + (float)ii * (_renderRadius * CREATE_OFFSET),
					fymin + (float)jj * (_renderRadius * CREATE_OFFSET),
					fzmin + (float)kk * (_renderRadius * CREATE_OFFSET)
				};
				//Vector3 velocity(Math::RandomVec3(-1.0f, 1.0f));
				Vector3 velocity(0.0f,0.0f,0.0f);
				Particle* newParticle = createParticle(position, velocity, _mass);

				_fluidParticles.push_back(newParticle);
				_spatialHash.addParticle(newParticle);
			}
		}
	}	


	_fluidParticles.shrink_to_fit();
	_particleRenderDatas.shrink_to_fit();
}

void SESPHFluid::initBoundaryData()
{
	createBounaryParticle();
}

void SESPHFluid::createBounaryParticle()
{
	UINT x = static_cast<UINT>((_xmax - _xmin) / (_renderRadius * CREATE_OFFSET));
	UINT y = static_cast<UINT>((_ymax - _ymin) / (_renderRadius * CREATE_OFFSET));
	UINT z = static_cast<UINT>((_zmax - _zmax) / (_renderRadius * CREATE_OFFSET));

	Vector3 xdir = { 1.0f,0.0f,0.0f };
	Vector3 ydir = { 0.0f,1.0f,0.0f };
	Vector3 zdir = { 0.0f,0.0f,1.0f };


}

void SESPHFluid::createBoundaryPlane(float c1, float c2, Vector3 dir1, Vector3 dir2)
{
}

void SESPHFluid::Update()
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
		computeDensityAndPressure();
		t = pf.End();
		tot += t;
		string str = "updateNeighbor and computeDensityAndPressure : \n" + to_string(t) + " ms";
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

void SESPHFluid::Render()
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
	ImGui::Text("viscosityKernelCoefficient : %f", _viscosityKernelCoefficient);
	ImGui::Separator();


	ImGui::Text("boundary X length : %f", _xmax - _xmin);
	ImGui::Text("boundary Y length : %f", _ymax - _ymin);
	ImGui::Text("boundary Z length : %f", _zmax - _zmin);
}

void SESPHFluid::updateNeighbor()
{
	//Particle* pi = nullptr;
	//UINT fsize = _fluidParticles.size();
	//for (UINT i = 0; i < fsize; i++)
	//{
	//	pi = _fluidParticles[i];
	//	_particleNeighbor[i].clear();
	//	_spatialHash.findNeighbor(pi->position, _particleNeighbor[i]);
	//}	
}

void SESPHFluid::updateRenderParticleData()
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

void SESPHFluid::computeDensityAndPressure()
{
	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();

	Vector3 rVec;
	float r;
	float r2;
	float diff;

	UINT neighbors = 0;
	UINT neighborsIn = 0;

	_sumDensity = 0;
	_sumPressure = 0;

	SpatialHash::debugNNS = 0;

	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];

		_spatialHash.findNeighbor(*pi, _particleNeighbor[i]);

		pi->density = 0;
		for (Cell * & neighbor : _particleNeighbor[i])
		{
			for (Particle* & pj : neighbor->particles)
			{
				rVec = pi->position - pj->position;
				r = D3DXVec3Length(&rVec);
				r2 = D3DXVec3Dot(&rVec, &rVec);
				diff = _h2 - r2;
				if (r <= _h)
				{
					pi->density += W_Default(diff) * pj->mass;
					neighborsIn++;
				}

			}

			neighbors += neighbor->size();
		}


		// max(pi , p0 )		
		pi->density = max(pi->density, _idealDensity);

		//if (pi->position.y < _ymin + 0.1f){pi->density *= Math::Lerp(4.5f , 1.0f, (pi->position.y - _ymin) / 0.1f);}
		// k ( pi - p0 )
		pi->pressure = _pressureCoefficient * (pi->density / _idealDensity - 1.0f);

		// debug data
		_sumDensity += pi->density;
		_sumPressure += pi->pressure;
	}
	float avg = static_cast<float>(neighborsIn) / static_cast<float>(fsize);
	ImGui::Text("average neighbor Inner count : %f", avg);
	avg = static_cast<float>(neighbors) / static_cast<float>(fsize);
	ImGui::Text("average neighbor count : %f", avg);



	ImGui::Text("debugNNS : %d", SpatialHash::debugNNS);
}

void SESPHFluid::computeFluidAccleration()
{


	Particle* pi = nullptr;
	UINT fsize = _fluidParticles.size();
	Vector3 force;
	Vector3 rVec;
	Vector3 pressureForce;
	Vector3 viscosityForce;
	float surfaceTerm;

	Vector3 diffJI;
	float diff;
	float diff2;
	float  r;
	float r2;

	Vector3 newPosistion;

	Vector3 surfaceNormalVec = { 0.0f,0.0f,0.0f };
	float colorField = 0.0f;

	_debugSurface = 0;
	_debugPressureForce = 0.0f;

	float avgAccMag = 0;

	// 
	for (UINT i = 0; i < fsize; i++)
	{
		pi = _fluidParticles[i];

		force = { 0.0f,0.0f,0.0f };
		pressureForce = { 0.0f, 0.0f, 0.0f };
		viscosityForce = { 0.0f, 0.0f, 0.0f };
		surfaceTerm = 0.0f;

		// surface Tension 계산용 벡터;
		surfaceNormalVec = { 0.0f,0.0f,0.0f };

		for (Cell* & neighbor : _particleNeighbor[i])
		{
			for (Particle* & pj : neighbor->particles)
			{
				rVec = pi->position - pj->position;
				r = D3DXVec3Length(&rVec);

				if (r < 0.0f + 0.000001f || r > _h)
				{
					continue;
				}
				r2 = r * r;
				diff = _h - r;
				diff2 = _h2 - r2;

				// compute external force [ surface tension ]
				{
					surfaceTerm += pj->mass / pj->density * WL_Default(_h2, r2);
					surfaceNormalVec += rVec * (pj->mass / pj->density * WG_Default(diff2));
				}

				// compute internal force [ viscosity ]
				{
					diffJI = pj->velocity - pi->velocity;
					viscosityForce += pj->mass * W_Viscosity(diff) * diffJI;
				}
				// normalized r vector
				rVec /= r;
				// compute internal force [ pressure ]
				{
					//float pterm = (pi->pressure / (pi->density * pi->density)) + (pj->pressure / (pj->density * pj->density));
					float pterm = (pi->pressure + pj->pressure) / (2.0f * pi->density * pj->density);
					pterm *= pj->mass / pi->mass;
					pressureForce += pterm * W_Pressure(diff) * rVec;
				}


			}
		}


		_debugPressureForce += D3DXVec3Length(&pressureForce);


		// internal force [pressure + viscosity]
		force -= pressureForce * pi->density;
		force += _viscosityCoefficient * viscosityForce;

		// acc <= force / density;
		pi->acceleration = force / pi->density;

		// external force [gravity]
		pi->acceleration += gravity;

		// external force [surface tension ]
		float surfLen = D3DXVec3Length(&surfaceNormalVec);
		if (surfLen >= _surfaceTensionThreshold)
		{

			_debugSurface++;

			surfaceNormalVec /= surfLen;
			pi->acceleration -= _surfaceTensionCoefficeint * surfaceNormalVec * surfaceTerm;

			pi->isSurface = 1;
		}
		else
		{
			pi->isSurface = 0;
		}


		pi->acceleration += computeObstacleAccleration(pi);

		float alen = D3DXVec3Length(&pi->acceleration);
		if (alen > ACC_MAX)
		{
			pi->acceleration /= alen;
			pi->acceleration *= ACC_MAX;
		}

		avgAccMag += D3DXVec3Length(&pi->acceleration);
	}

	avgAccMag /= float(fsize);
	ImGui::Text("average acceleration : %f", avgAccMag);
	ImGui::Text("debug surface : %d", _debugSurface);
}

Vector3 SESPHFluid::computeObstacleAccleration(Particle * particle)
{
	const float weight = 1.0f;
	Vector3 acc = { 0.0f , 0.0f ,0.0f };
	float r = _h * 2.0f ;// boundary radius
	float minf = 0.0f; // boundary min force
	float maxf = 1.82f; // boundary max force

	const Vector3 & p = particle->position;
	const Vector3 & a = particle->acceleration;

	if (p.x < _xmin + r && a.x < 0)
	{
		float dist = fmax(0.0f, p.x - _xmin);
		//double force = Math::Lerp(maxf, minf, dist / r);
		float force = Math::Lerp(particle->acceleration.x, minf, dist / r);
		acc += Vector3(force, 0.0f, 0.0f) * weight;

	}
	else if (p.x > _xmax - r && a.x > 0)
	{
		float dist = fmax(0.0f, _xmax - p.x);
		//double force = Math::Lerp(maxf, minf, dist / r);
		float force = Math::Lerp(particle->acceleration.x, minf, dist / r);
		acc += Vector3(-force, 0.0f, 0.0f) * weight;

	}

	if (p.y < _ymin + r && a.y < 0)
	{
		float dist = fmax(0.0f, p.y - _ymin);
		float force = Math::Lerp(maxf, minf, dist / r);
		acc += Vector3(0.0f ,force, 0.0f) * weight;
	}
	else if (p.y > _ymax - r && a.y > 0)
	{
		float dist = fmax(0.0f, _ymax - p.y);
		float force = Math::Lerp(maxf, minf, dist / r);
		acc += Vector3(0.0f, -force, 0.0f) * weight;
	}

	if (p.z < _zmin + r && a.z < 0)
	{
		float dist = fmax(0.0f, p.z - _zmin);
		//double force = Math::Lerp(maxf, minf, dist / r);
		float force = Math::Lerp(particle->acceleration.z, minf, dist / r);
		acc += Vector3(0.0f, 0.0f, force) * weight;
	}
	else if (p.z > _zmax - r && a.z > 0)
	{
		float dist = fmax(0.0f, _zmax - p.z);
		//double force = Math::Lerp(maxf, minf, dist / r);
		float force = Math::Lerp(particle->acceleration.z, minf, dist / r);
		acc += Vector3(0.0f, 0.0f, -force) * weight;
	}

	return acc;
}

void SESPHFluid::computeVelocityAndPosition()
{
	
	/*
		time integration
		implicit 오일러 방정식
	*/

	Particle * particle = nullptr;
	Vector3 newPosition;
	Vector3 t;

	float avgvelMag = 0;

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

		t = particle->velocity * _deltaTime;
		_debugDisplacement += D3DXVec3Length(&t);

		newPosition = particle->position + particle->velocity * _deltaTime;

		enforceBoundaryPosition(particle, newPosition);
		_spatialHash.moveParticleInHash(particle, newPosition);

	}

	ImGui::Text("average velocity : %f", avgvelMag / fsize);

}

void SESPHFluid::enforceBoundaryPosition(Particle* particle ,Vector3 & position)
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

SESPHFluid::~SESPHFluid()
{
	for (Particle*p : _allParticles) SafeDelete(p);
}

UINT SESPHFluid::calculateCellMax()
{
	return _particleCount * 3;
}