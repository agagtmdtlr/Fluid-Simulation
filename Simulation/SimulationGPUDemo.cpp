#include "Framework.h"
#include "Physics/SPHcommon.h"
#include "SimulationGPUDemo.h"
#define COUNT_FOR_AXIS (60.f)

SimulationGPUDemo::SimulationGPUDemo()
	:debugRenderer(GridInfo(),0)
	,fluidModel(Vector3(6.0f, 2.5f,4.0f), (0.4641f / 30.0f) , COUNT_FOR_AXIS , 20.0f)
{	
	Context::Get()->GetCamera()->RotationDegree(19.0f, -41.0f, 0.0f);
	Context::Get()->GetCamera()->Position(17.0f , 8.0f, -10.0f);

	particleRenderer = make_unique<ParticleRenderer>(fluidModel.getParticleCount(), fluidModel.getRadius() , ParticleRenderer::eRenderingMode::fluidMode, fluidModel.getBoundary());
	boundaryParticleRenderer = make_unique<ParticleRenderer>(fluidModel.getBoundaryParticleCount(), fluidModel.getRadius(), ParticleRenderer::eRenderingMode::boundaryMode, fluidModel.getBoundary());
}

SimulationGPUDemo::~SimulationGPUDemo()
{
}

void SimulationGPUDemo::Initialize()
{
}

void SimulationGPUDemo::Ready()
{
}

void SimulationGPUDemo::Destroy()
{
}

void SimulationGPUDemo::Update()
{
	if (Keyboard::Get()->Press(VK_SPACE))
	{
		Context::Get()->GetCamera()->RotationDegree(30.0f, -90.0f, 0.0f);
		Context::Get()->GetCamera()->Position(9.5f * 2, 6.5f, 1.0f);
	}

	fluidModel.update();
	particleRenderer->copyToBufferFromResourceManager();
	particleRenderer->Update();
	debugRenderer.SetBoundary(fluidModel.getBoundary());
	debugRenderer.Update();
	//boundaryParticleRenderer->Update();
}

void SimulationGPUDemo::PreRender()
{
}

void SimulationGPUDemo::Render()
{
	fluidModel.Render();
	particleRenderer->Render();
	debugRenderer.Render();
	//boundaryParticleRenderer->Render();
}

void SimulationGPUDemo::PostRender()
{
}

void SimulationGPUDemo::ResizeScreen()
{
}
