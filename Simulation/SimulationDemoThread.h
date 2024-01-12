#pragma once
#include "Physics/SimulationSystem.h"
#include "Physics/ISPH/SESPHFluidThread.h"
#include "Physics/ParticleRenderer.h"
#include "Physics/DebugRenderer.h"


class SimulationDemoThread : public IExecute
{


public: SimulationDemoThread();
public: ~SimulationDemoThread();

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
private: unique_ptr<class MeshRender> _meshRender = nullptr;

private: SESPHFluidThread fluidModel;
private: unique_ptr<ParticleRenderer> particleRenderer;
private: DebugRenderer	  debugRenderer;
};

