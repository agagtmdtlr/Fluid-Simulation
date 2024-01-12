#include "Framework.h"

#include "Meshes/MeshRender.h"
#include "Meshes/MeshCube.h"

#include "Physics/SPHcommon.h"

#include "SimulationISPHDemo.h"


SimulationISPHDemo::SimulationISPHDemo()
	:debugRenderer(GridInfo(),0)
{	
	//Context::Get()->GetCamera()->Position(1.5f, 1.5f,-5.0f);

	//Context::Get()->GetCamera()->RotationDegree(30.0f, 0.0f, 0.0f);
	//Context::Get()->GetCamera()->Position(1.5f, 4.5f, -6.0f);

	//Context::Get()->GetCamera()->RotationDegree(38.0f, -90.0f, 0.0f);
	//Context::Get()->GetCamera()->Position(18.0f, 13.0f, 1.0f);

	Context::Get()->GetCamera()->RotationDegree(30.0f, -90.0f, 0.0f);
	Context::Get()->GetCamera()->Position(9.5f * 2, 6.5f, 1.0f);

	//Context::Get()->GetCamera()->RotationDegree(0.0f, -109.0f, 0.0f);
	//Context::Get()->GetCamera()->Position(30.0f, 0.0f, 10.0f);

	particleRenderer = make_unique<ParticleRenderer>(fluidModel.getParitlceCount(), fluidModel.getRadius(), ParticleRenderer::eRenderingMode::fluidMode, fluidModel.getBoundary());
}

SimulationISPHDemo::~SimulationISPHDemo()
{
}

void SimulationISPHDemo::Initialize()
{
	fluidModel.InitSimulation();
}

void SimulationISPHDemo::Ready()
{
}

void SimulationISPHDemo::Destroy()
{
}

void SimulationISPHDemo::Update()
{
	if (Keyboard::Get()->Press(VK_SPACE))
	{
		Context::Get()->GetCamera()->RotationDegree(30.0f, -90.0f, 0.0f);
		Context::Get()->GetCamera()->Position(9.5f * 2, 6.5f, 1.0f);
	}

	fluidModel.Update();
	particleRenderer->copyToBuffer(fluidModel.getFluidParticlesRenderData());
	particleRenderer->Update();

	debugRenderer.SetBoundary(fluidModel.getBoundary());
	debugRenderer.Update();
}

void SimulationISPHDemo::PreRender()
{
}

void SimulationISPHDemo::Render()
{
	
	particleRenderer->Render();
	fluidModel.Render();
	debugRenderer.Render();
}

void SimulationISPHDemo::PostRender()
{
}

void SimulationISPHDemo::ResizeScreen()
{
}
