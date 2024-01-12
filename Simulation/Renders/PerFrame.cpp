#include "Framework.h"
#include "PerFrame.h"

UINT PerFrame::instanceCount = 0;

PerFrame::LightDesc			PerFrame::sLightDesc;
PerFrame::PointLightDesc	PerFrame::sPointLightDesc;
PerFrame::SpotLightDesc		PerFrame::sSpotLightDesc;
PerFrame::FogDesc			PerFrame::sFogDesc;

ConstantBuffer* PerFrame::lightBuffer = NULL;
ConstantBuffer* PerFrame::pointLightBuffer = NULL;
ConstantBuffer* PerFrame::spotLightBuffer = NULL;
ConstantBuffer* PerFrame::fogBuffer = NULL;


PerFrame::PerFrame(Shader * shader)
	: shader(shader)
{
	//my_assert(L"shader NA Error", shader != nullptr);		
	instanceCount++;
	buffer = new ConstantBuffer(&desc, sizeof(Desc));
	sBuffer = shader->AsConstantBuffer("CB_PerFrame");

	lightBuffer = GetLightBuffer();
	sLightBuffer = shader->AsConstantBuffer("CB_Light");

	pointLightBuffer = GetPointLightBuffer();
	sPointLightBuffer = shader->AsConstantBuffer("CB_PointLights");

	spotLightBuffer = GetSpotLightBuffer();
	sSpotLightBuffer = shader->AsConstantBuffer("CB_SpotLights");
	
	// initialize 
	ZeroMemory(desc.Culling, sizeof(Plane) * 4); // left right top bottom
	ZeroMemory(desc.Clipping, sizeof(Plane)); // used

	fogBuffer = GetFogBuffer();
	sFogBuffer = shader->AsConstantBuffer("CB_Fog");
}

PerFrame::~PerFrame()
{
	instanceCount--;
	SafeDelete(buffer);
	if (instanceCount < 1)
	{
		SafeDelete(lightBuffer);
		SafeDelete(pointLightBuffer);
		SafeDelete(spotLightBuffer);
		SafeDelete(fogBuffer);

		lightBuffer = NULL;
		pointLightBuffer = NULL;
		spotLightBuffer = NULL;
		fogBuffer = NULL;
	}
}

void PerFrame::Update()
{
	// 현재 게임 실행시간
	desc.Time = Time::Get()->Running();

	sLightDesc.Ambient = Context::Get()->Ambient();
	sLightDesc.Specular = Context::Get()->Specular();
	sLightDesc.Direction = Context::Get()->Direction();
	sLightDesc.Position = Context::Get()->Position();
	sLightDesc.Intensity = Context::Get()->Intensity();

	sPointLightDesc.Count = Lighting::Get()->PointLights(sPointLightDesc.Lights);
	sSpotLightDesc.Count = Lighting::Get()->SpotLights(sSpotLightDesc.Lights);

	sFogDesc.FogColor = Lighting::Get()->FogColor();
	sFogDesc.FogDistance = Lighting::Get()->FogDistance();
	sFogDesc.FogDensity = Lighting::Get()->FogDensity();
	sFogDesc.FogType = Lighting::Get()->FogType();
}

void PerFrame::Render()
{
	desc.View = Context::Get()->View();
	// 역행렬을 취하면 이전의 결과를 복원이 된다.
	D3DXMatrixInverse(&desc.ViewInverse, NULL, &desc.View);

	desc.Projection = Context::Get()->Projection();
	// 각 정점에서 결합된 결과를 가지고 연산을 수행하므로 속도가 더 빠르다.
	// vp wvp 등 결합된 결과를 보내서 연산을 빠르게 하도록 하는게 좋다.
	// cpu 에서는 한번만 결합하면 되지만 gpu는 정점마다 결합해야 한다.
	desc.VP = desc.View * desc.Projection;

	// CB_Perframe : desc data
	buffer->Render();
	sBuffer->SetConstantBuffer(buffer->Buffer());

	// CB_Light : lightDesc data
	lightBuffer->Render();
	sLightBuffer->SetConstantBuffer(lightBuffer->Buffer());

	pointLightBuffer->Render();
	sPointLightBuffer->SetConstantBuffer(pointLightBuffer->Buffer());

	spotLightBuffer->Render();
	sSpotLightBuffer->SetConstantBuffer(spotLightBuffer->Buffer());

	fogBuffer->Render();
	sFogBuffer->SetConstantBuffer(fogBuffer->Buffer());
}

ConstantBuffer * PerFrame::GetLightBuffer()
{
	if (lightBuffer == NULL)
	{
		lightBuffer = new ConstantBuffer(&sLightDesc, sizeof(LightDesc));
	}
	return lightBuffer;
}

ConstantBuffer * PerFrame::GetPointLightBuffer()
{
	if (pointLightBuffer == NULL)
	{
		pointLightBuffer = new ConstantBuffer(&sPointLightDesc, sizeof(PointLightDesc));
	}
	return pointLightBuffer;
}

ConstantBuffer * PerFrame::GetSpotLightBuffer()
{
	if (spotLightBuffer == NULL)
	{
		spotLightBuffer = new ConstantBuffer(&sSpotLightDesc, sizeof(SpotLightDesc));
	}
	return spotLightBuffer;
}

ConstantBuffer * PerFrame::GetFogBuffer()
{
	if (fogBuffer == NULL)
	{
		fogBuffer = new ConstantBuffer(&sFogDesc, sizeof(FogDesc));
	}
	return fogBuffer;
}
