#pragma once
#include "Physics/SPHcommon.h"
using HashKey = UINT;
using PointID = UINT;

class SpatialHash
{
public: static UINT debugNNS;

public: SpatialHash();
public: ~SpatialHash();

public: UINT getInnerParticleCount(); // 디버그 용
public: void setCellSize(float cellSize) { _cellSize = cellSize; }

public: void addParticle(Particle * particle);
public: void findNeighbor(Particle & particle, vector<Cell*> & neighbor);
public: void findNeighborSet(Particle & particle, set<Cell*> & neighbor);

public: void findNeighborList(Particle & particle, vector<Particle*> & neighbor, const float & radius);


public: void moveParticleInHash(Particle* particle , Vector3 & position);

public: void resizeFreeCells(UINT freeCellCount);

private: inline void positionToIJK(const Vector3 & p, int & i, int & j, int & k);
private: inline void IJKtoPosition(Vector3& p, const int & i, const int & j, const int & k);
private: inline const HashKey hashing(const int i, const int j, const int k);

private: float _cellSize;
private: UINT _totalCells;
private: UINT _freeCellCount;

private: vector<Cell> _table;
private: vector<Cell*> _freeCells;
};