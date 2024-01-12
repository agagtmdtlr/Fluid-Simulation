#pragma once

class MeshHemiSphere : public Mesh
{
public:
	MeshHemiSphere(UINT stackCount = 20, UINT sliceCount = 20);
	~MeshHemiSphere();
private:
	void Create();

	UINT stackCount;
	UINT sliceCount;
};