#include "Framework.h"

#include "Meshes/MeshRender.h"
#include "Meshes/MeshCube.h"

#include "Physics/SPHcommon.h"

#include "SimulationDemoThread.h"


SimulationDemoThread::SimulationDemoThread()
	:debugRenderer(GridInfo(),0)
{	
	//Context::Get()->GetCamera()->Position(1.5f, 1.5f,-5.0f);

	//Context::Get()->GetCamera()->RotationDegree(30.0f, 0.0f, 0.0f);
	//Context::Get()->GetCamera()->Position(1.5f, 4.5f, -6.0f);

	//Context::Get()->GetCamera()->RotationDegree(38.0f, -90.0f, 0.0f);
	//Context::Get()->GetCamera()->Position(21.0f, 11.0f, 3.0f);

	Context::Get()->GetCamera()->RotationDegree(29.0f, -2.0f, 0.0f);
	Context::Get()->GetCamera()->Position(8.0f, 15.0f, -25.0f);

	particleRenderer = make_unique<ParticleRenderer>(fluidModel.getParitlceCount(), fluidModel.getRadius(), ParticleRenderer::eRenderingMode::fluidMode, fluidModel.getBoundary());
}

SimulationDemoThread::~SimulationDemoThread()
{
}

void SimulationDemoThread::Initialize()
{
}

void SimulationDemoThread::Ready()
{
}

void SimulationDemoThread::Destroy()
{
}

void SimulationDemoThread::Update()
{
	if (Keyboard::Get()->Press(VK_SPACE))
	{
		Context::Get()->GetCamera()->RotationDegree(30.0f, -90.0f, 0.0f);
		Context::Get()->GetCamera()->Position(21.0f, 11.0f, 3.0f);
	}

	fluidModel.Update();
	particleRenderer->copyToBuffer(fluidModel.getFluidParticlesRenderData());
	particleRenderer->Update();

	debugRenderer.SetBoundary(fluidModel.getBoundary());
	debugRenderer.Update();
}

void SimulationDemoThread::PreRender()
{
}

void SimulationDemoThread::Render()
{
	
	particleRenderer->Render();
	fluidModel.Render();
	debugRenderer.Render();
}

void SimulationDemoThread::PostRender()
{
}

void SimulationDemoThread::ResizeScreen()
{
}
