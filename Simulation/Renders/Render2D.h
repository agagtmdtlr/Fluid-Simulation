#pragma once

class Render2D : public Renderer
{
public:
	Render2D();
	~Render2D();

	virtual void Update() override;
	virtual void Render() override;

	void SRV(ID3D11ShaderResourceView* srv);
	void SRV2(ID3D11ShaderResourceView* srv);

private:
	struct Desc
	{
		Matrix View;
		Matrix Projection;
	} desc;

private:
	ConstantBuffer* buffer;
	ID3DX11EffectShaderResourceVariable* sDiffuseMap;
	ID3DX11EffectShaderResourceVariable* sDiffuseMap2;
};

class Render2DMipMap : public Render2D
{
public:
	Render2DMipMap(float level);	
	~Render2DMipMap();

	virtual void Update() override;
	virtual void Render() override;

private:
	struct MipMapDesc
	{
		float MipLevel = 0.0f;
		float padding[3];
	} mMipMapDesc;
private:
	float mCurLevel;
	float mMaxLevel;
	unique_ptr<ConstantBuffer> mCBuffer;
	ComPtr<ID3D11SamplerState> mSamplerState;
};