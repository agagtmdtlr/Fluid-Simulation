#pragma once
#include "Framework.h"

enum eParticleType : UINT
{
	ParticleTypeFluid = 0,
	ParticleTypeSolid = ParticleTypeFluid + 1
};

#define DEFAULT_PARTICLE_TYPE (eParticleType::ParticleTypeFluid);


struct ParticleRenderData
{
	Vector3 position;
	Vector3 color;	
};

class Particle // w
{

public: Vector3 acceleration;			//  ���ӵ�
public: UINT	id;
public:	Vector3 velocity;				//2	��ƼŬ�� �ӵ�
public:	float	density;				//	��ƼŬ�� �е�
public:	Vector3 position;				//3	��ƼŬ�� ��ġ
public:	float	pressure;				//	��ƼŬ�� �з� or Volume
public: int prevCell[3];
public: float	mass;					//	����
public: UINT isSurface; // 0 or 1;
public: eParticleType pType = DEFAULT_PARTICLE_TYPE;
public: Vector3 color;
public: float kFactor;
public: 
//public: float	soundSpeed;				//	�Ҹ� �ӵ�
//public:	eParticleType type = DEFAULT_PARTICLE_TYPE;// 0 : ���� // 1 : ��ü(���)
};

struct Cell
{
	int i;
	int j;
	int k;
	vector<Particle *> particles;
	inline bool isEmpty() const { return particles.size() == 0; }
	inline UINT size() const { return particles.size(); }
	inline bool operator==(const Cell & rhs) const
	{
		return (i == rhs.i && j == rhs.j && k == rhs.k);
	}
	inline void erase(Particle * particle)
	{		
		//for (auto it = particles.begin(), endit = particles.end(); it != endit; it++)
		//{
		//	if ((*it)->id == particle->id)
		//	{
		//		particles.erase(it);
		//		return;
		//	}
		//}
		//
		for (UINT it = 0, endit = particles.size(); it < endit; it++)
		{
			if (particle == particles[it])
			{
				swap(particles[it], particles[endit - 1]);
				particles.pop_back();
				return;
			}
		}		
	}
};

struct CellThread
{
	vector<Particle *> particles;
	bool isEmpty() { return particles.size() == 0; }
	inline UINT size() { return particles.size(); }
	void erase(Particle * particle)
	{
		for (auto it = particles.begin(), endit = particles.end(); it != endit; it++)
		{
			if ((*it)->id == particle->id)
			{
				particles.erase(it);
				return;
			}
		}
	}
};

struct GridInfo
{
	UINT x;
	UINT y;
	UINT z;
	float cellSize;
};
	

struct RenderData
{
	Vector3 position;
	float pressure;
};

