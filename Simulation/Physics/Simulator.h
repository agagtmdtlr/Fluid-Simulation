#pragma once
#include "Physics/SPHcommon.h"


class Simulator	
{
public:	Simulator();
public: ~Simulator();

//private: unique_ptr<SimulationSystem> simulationModel;
//private: unique_ptr<RenderingSystem> renderingModel;

// 0-2;
private: unique_ptr<StructuredBuffer> _particles[2];
	
private: vector<Cell> grid;

};
