#pragma once
#include "Physics/SimulationSystem.h"
#include "Physics/ISPH/SESPHFluidGpu.h"
#include "Physics/ParticleRenderer.h"
#include "Physics/DebugRenderer.h"


class SimulationGPUDemo : public IExecute
{


public: SimulationGPUDemo();
public: ~SimulationGPUDemo();

	// Inherited via IExecute
public:	virtual void Initialize() override;
public:	virtual void Ready() override;
public:	virtual void Destroy() override;
public:	virtual void Update() override;
public:	virtual void PreRender() override;
public:	virtual void Render() override;
public:	virtual void PostRender() override;
public:	virtual void ResizeScreen() override;

private: Shader* _shader = nullptr;

private: SESPHFluidGpu fluidModel;
private: unique_ptr<ParticleRenderer> particleRenderer;
private: unique_ptr<ParticleRenderer> boundaryParticleRenderer;
private: DebugRenderer	  debugRenderer;


};

