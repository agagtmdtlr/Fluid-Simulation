#include "Framework.h"
#include "Lighting.h"

Lighting* Lighting::instance = NULL;

Lighting * Lighting::Get()
{
	assert(instance != NULL);

	return instance;
}

void Lighting::Create()
{
	assert(instance == NULL);

	instance = new Lighting();
}

void Lighting::Delete()
{
	SafeDelete(instance);
}

Lighting::Lighting()
{

}

Lighting::~Lighting()
{

}

// point ///////////////////////////////////////////////////////////

UINT Lighting::PointLights(OUT PointLight * lights)
{
	memcpy(lights, pointLights, sizeof(PointLight) * pointLightCount);

	return pointLightCount;
}

void Lighting::AddPointLight(PointLight & light)
{
	pointLights[pointLightCount] = light;
	pointLightCount++;
}

PointLight & Lighting::GetPointLight(UINT index)
{
	return pointLights[index];
}

//spot///////////////////////////////////////////

UINT Lighting::SpotLights(OUT SpotLight * lights)
{
	memcpy(lights, spotLights, sizeof(SpotLight) * spotLightCount);

	return spotLightCount;
}

void Lighting::AddSpotLight(SpotLight & light)
{
	spotLights[spotLightCount] = light;
	spotLightCount++;
}

SpotLight & Lighting::GetSpotLight(UINT index)
{
	return spotLights[index];
}

UINT Lighting::SpotLightPBRs(OUT SpotLightPBR * lights)
{
	memcpy(lights, spotLightPBRs, sizeof(SpotLightPBR) * spotLightPBRCount);
	return spotLightPBRCount;
}

void Lighting::AddSpotLightPBR(SpotLightPBR & light)
{
	spotLightPBRs[spotLightPBRCount] = light;	
	D3DXVec3Normalize(&spotLightPBRs[spotLightPBRCount].Direction, &spotLightPBRs[spotLightPBRCount].Direction);
	spotLightPBRCount++;
}

SpotLightPBR & Lighting::GetSpotLightPBR(UINT index)
{
	return spotLightPBRs[index];
}
