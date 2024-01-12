#include "Framework.h"

#include "Meshes/MeshRender.h"
#include "Meshes/MeshCube.h"

#include "Physics/SPHcommon.h"

#include "SimulationDemo.h"


SimulationDemo::SimulationDemo()
	:debugRenderer(GridInfo(),0)
{	
	//Context::Get()->GetCamera()->Position(1.5f, 1.5f,-5.0f);
	//Context::Get()->GetCamera()->RotationDegree(30.0f, 0.0f, 0.0f);
	Context::Get()->GetCamera()->RotationDegree(38.0f, -90.0f, 0.0f);

	//Context::Get()->GetCamera()->Position(1.5f, 4.5f, -6.0f);
	Context::Get()->GetCamera()->Position(21.0f, 11.0f, 3.0f);

	particleRenderer = make_unique<ParticleRenderer>(fluidModel.getParitlceCount(), fluidModel.getRadius(), ParticleRenderer::eRenderingMode::fluidMode, fluidModel.getBoundary());
}

SimulationDemo::~SimulationDemo()
{
}

void SimulationDemo::Initialize()
{
}

void SimulationDemo::Ready()
{
}

void SimulationDemo::Destroy()
{
}

void SimulationDemo::Update()
{
	fluidModel.Update();
	particleRenderer->copyToBuffer(fluidModel.getFluidParticlesRenderData());
	particleRenderer->Update();

	debugRenderer.SetBoundary(fluidModel.getBoundary());
	debugRenderer.Update();
}

void SimulationDemo::PreRender()
{
}

void SimulationDemo::Render()
{
	
	particleRenderer->Render();
	fluidModel.Render();
	debugRenderer.Render();
}

void SimulationDemo::PostRender()
{
}

void SimulationDemo::ResizeScreen()
{
}
