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
	// ���� ���� ����ð�
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
	// ������� ���ϸ� ������ ����� ������ �ȴ�.
	D3DXMatrixInverse(&desc.ViewInverse, NULL, &desc.View);

	desc.Projection = Context::Get()->Projection();
	// �� �������� ���յ� ����� ������ ������ �����ϹǷ� �ӵ��� �� ������.
	// vp wvp �� ���յ� ����� ������ ������ ������ �ϵ��� �ϴ°� ����.
	// cpu ������ �ѹ��� �����ϸ� ������ gpu�� �������� �����ؾ� �Ѵ�.
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
