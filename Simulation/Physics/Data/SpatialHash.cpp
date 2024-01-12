#include "Framework.h"
#include "Physics/Data/SpatialHash.h"

UINT SpatialHash::debugNNS = 0;

SpatialHash::SpatialHash()
{}

SpatialHash::~SpatialHash()
{}

UINT SpatialHash::getInnerParticleCount()
{
	UINT sum = 0;
	for (Cell& c : _table)
		sum += c.size();
	return sum;
}

void SpatialHash::positionToIJK(const Vector3 & p, int & i, int & j, int & k)
{
	i = static_cast<int>(floor(p.x / _cellSize));
	j = static_cast<int>(floor(p.y / _cellSize));
	k = static_cast<int>(floor(p.z / _cellSize));
}

void SpatialHash::IJKtoPosition(Vector3 & p, const int & i, const int & j, const int & k)
{
	p = Vector3(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k)) * _cellSize;
}

void SpatialHash::addParticle(Particle * particle)
{
	int i, j, k;
	positionToIJK(particle->position, i, j, k);
	Cell & cell = _table[hashing(i, j, k)];
	cell.particles.push_back(particle);
}

void SpatialHash::findNeighbor(Particle & particle, vector<Cell*> & neighbor)
{

	const Vector3 & position = particle.position;
	int i, j, k;
	positionToIJK(position, i, j, k);

	if (particle.prevCell[0] == i && particle.prevCell[1] == j && particle.prevCell[2] == k)
	{
		particle.prevCell[0] = i;
		particle.prevCell[1] = j;
		particle.prevCell[2] = k;
		debugNNS++;
		return; // 
	}

	particle.prevCell[0] = i;
	particle.prevCell[1] = j;
	particle.prevCell[2] = k;

	neighbor.clear();

	int bbMin[3] = { i - 1, j - 1, k - 1};
	int bbMax[3] = { i + 1, j + 1, k + 1};

	Vector3 detect;
	//int di, dj, dk;
	int ci, cj, ck;


	HashKey hsSet[27];
	UINT curr = 0; 
	HashKey key;
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			for (int dz = -1; dz < 2; dz++)
			{
				ci = i + dx;
				cj = j + dy;
				ck = k + dz;
				//
				//detect.x = position.x + _cellSize * dx;
				//detect.y = position.y + _cellSize * dy;
				//detect.z = position.z + _cellSize * dz;
				//positionToIJK(detect, di, dj, dk);
				//
				//if (ci == di && cj == dj && ck == dk)
				UINT ii;
				for (ii = 0 , key = hashing(ci, cj, ck); ii < curr && hsSet[ii] != key; ii++)	{}
				if(ii == curr)
				{
					Cell & cell = _table[key];
					neighbor.push_back(&cell);
				}
				hsSet[curr++] = key;
			}
		}
	}
}

void SpatialHash::findNeighborSet(Particle & particle, set<Cell*>& neighbor)
{
	const Vector3 & position = particle.position;
	int i, j, k;
	positionToIJK(position, i, j, k);

	if (particle.prevCell[0] == i && particle.prevCell[1] == j && particle.prevCell[2] == k)
	{
		particle.prevCell[0] = i;
		particle.prevCell[1] = j;
		particle.prevCell[2] = k;
		debugNNS++;
		return; // 
	}
	particle.prevCell[0] = i;
	particle.prevCell[1] = j;
	particle.prevCell[2] = k;

	neighbor.clear();

	int bbMin[3] = { i - 1, j - 1, k - 1 };
	int bbMax[3] = { i + 1, j + 1, k + 1 };

	Vector3 detect;
	//int di, dj, dk;
	int ci, cj, ck;


	bool checked = true;
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			for (int dz = -1; dz < 2; dz++)
			{
				ci = i + dx;
				cj = j + dy;
				ck = k + dz;
				{
					neighbor.insert(&_table[hashing(ci, cj, ck)]);
				}
			}
		}
	}
}

void SpatialHash::findNeighborList(Particle & particle, vector<Particle*> & neighbor, const float & radius)
{
	const Vector3 & position = particle.position;
	int i, j, k;
	positionToIJK(position, i, j, k);
	particle.prevCell[0] = i;
	particle.prevCell[1] = j;
	particle.prevCell[2] = k;

	neighbor.clear();

	Vector3 diff;
	HashKey hsSet[27];
	UINT curr = 0;
	HashKey key;
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			for (int dz = -1; dz < 2; dz++)
			{
				UINT ii;
				for (ii = 0, key = hashing(i + dx, j + dy, k + dz); ii < curr && hsSet[ii] != key; ii++) {}
				if (ii >= curr)
				{
					Cell & cell = _table[key];
					for (Particle* p : cell.particles)
					{
						diff = position - p->position;
						if (D3DXVec3Length(&diff) <= radius)
						{
							neighbor.push_back(p);
						}
					}
				}
				hsSet[curr++] = key;
			}
		}
	}
	int a = 0;
}

void SpatialHash::moveParticleInHash(Particle* particle, Vector3 & position)
{
	
	int	prevI, prevJ, prevK;
	int i, j, k;
	positionToIJK(particle->position, prevI, prevJ, prevK);
	positionToIJK(position, i, j, k);
	
	// 변화가 없음;
	if (i == prevI && j == prevJ && k == prevK)
	{
		return;
	}
	// pop
	Cell & prevCell = _table[hashing(prevI, prevJ, prevK)];
	prevCell.erase(particle);

	// push
	Cell & cell = _table[hashing(i, j, k)];
	cell.particles.push_back(particle);

	particle->position = position;

}

void SpatialHash::resizeFreeCells(UINT freeCellCount)
{
	_totalCells = freeCellCount;
	_freeCellCount = freeCellCount;
	_table.resize(freeCellCount);
}

inline const HashKey SpatialHash::hashing(const int i, const int j, const int k)
{
	return
		((i * 73856093 ^
			j * 19349663 ^
			k * 83492791) % _totalCells);
}

