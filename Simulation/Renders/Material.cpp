#include "Framework.h"
#include "Material.h"

Material::Material()
{
	Initialize();
}

Material::Material(Shader * shader)
{
	Initialize();

	SetShader(shader);
}

void Material::Initialize()
{
	mName = L"";


}

Material::~Material()
{
}

void Material::SetShader(Shader * shader)
{
	this->shader = shader;
	mAlbedo.EffectSRV = shader->AsSRV("AlbedoMap");
	mMetallic.EffectSRV = shader->AsSRV("MetallicMap");
	mRoughness.EffectSRV = shader->AsSRV("RoughnessMap");
	mNormal.EffectSRV = shader->AsSRV("NormalMap");
	mAO.EffectSRV = shader->AsSRV("AOMap");
	mHeight.EffectSRV = shader->AsSRV("HeightMap");
}

void Material::CopyFrom(Material * material)
{
	mName = material->mName;
	mResourceDesc = material->mResourceDesc;

	if (material->shader != NULL)
		SetShader(material->shader);
}


void Material::Render()
{
	if (mAlbedo.Texture != nullptr)
	{
		mAlbedo.EffectSRV->SetResource(mAlbedo.Texture->SRV());
	}
	if (mMetallic.Texture != nullptr)
	{
		mMetallic.EffectSRV->SetResource(mMetallic.Texture->SRV());
	}
	if (mRoughness.Texture != nullptr)
	{
		mRoughness.EffectSRV->SetResource(mRoughness.Texture->SRV());
	}
	if (mNormal.Texture != nullptr)
	{
		mNormal.EffectSRV->SetResource(mNormal.Texture->SRV());
	}
	if (mAO.Texture != nullptr)
	{
		mAO.EffectSRV->SetResource(mAO.Texture->SRV());
	}
	if (mNormal.Texture != nullptr)
	{
		mHeight.EffectSRV->SetResource(mHeight.Texture->SRV());
	}


}