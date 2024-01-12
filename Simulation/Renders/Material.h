#pragma once

class Material
{
public:
	struct MaterialResource
	{
		unique_ptr<Texture> Texture;
		ID3DX11EffectShaderResourceVariable* EffectSRV;
	};
	struct MaterialResourceDesc
	{
		bool bAlbedo = false;
		bool bMetallic = false;
		bool bRoughness = false;
		bool bNormal = false;
		bool bAO = false;
		bool bHeight = false;
	};
public:
	Material();
	Material(Shader* shader);
	~Material();

	Shader* GetShader() { return shader; }
	void SetShader(Shader* shader);

	void CopyFrom(Material* material);

	void Name(wstring val) { mName = val; }
	wstring Name() { return mName; }

	void Render();

private:
	void Initialize();

private:
	Shader* shader = NULL;
	wstring mName;
	MaterialResourceDesc mResourceDesc;
	unique_ptr<ConstantBuffer> resourceDescCBuffer;

	MaterialResource mAlbedo;
	MaterialResource mMetallic;
	MaterialResource mRoughness;
	MaterialResource mNormal;
	MaterialResource mAO;
	MaterialResource mHeight;
private:
};