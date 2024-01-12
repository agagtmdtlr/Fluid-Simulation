#pragma once

class PerFrame
{

private: // static Data for singleton pattern
	static UINT instanceCount;

	struct Desc;
	struct LightDesc;
	struct PointLightDesc;
	struct SpotLightDesc;
	struct FogDesc;
	// update decription data for shader
	static LightDesc sLightDesc;
	static PointLightDesc sPointLightDesc;
	static SpotLightDesc sSpotLightDesc;
	static FogDesc sFogDesc;
	// bind piple line buffer resource;
	static ConstantBuffer* lightBuffer;
	static ConstantBuffer* pointLightBuffer;
	static ConstantBuffer* spotLightBuffer;
	static ConstantBuffer* fogBuffer;

private: // static method for singleton pattern
	static ConstantBuffer* GetLightBuffer();
	static ConstantBuffer* GetPointLightBuffer();
	static ConstantBuffer* GetSpotLightBuffer();
	static ConstantBuffer* GetFogBuffer();

public:
	PerFrame(Shader* shader);
	~PerFrame();

	void Culling(Plane* planes) { memcpy(desc.Culling, planes, sizeof(Plane) * 4); }
	void Clipping(Plane& plane) { desc.Clipping = plane; }

public:
	void Update();
	void Render();

private:
	// shader CB_PerFrame과 크기를 맞춰주어야 한다.
	struct Desc
	{
		Matrix View;
		Matrix ViewInverse;
		Matrix Projection;
		Matrix VP;

		Plane Culling[4];
		Plane Clipping;

		float Time;
		float Padding[3];
	} desc;

	struct LightDesc
	{
		Color Ambient;
		Color Specular;
		Vector3 Direction;
		float Intensity;

		Vector3 Position;
		float Padding2;
	};
	struct PointLightDesc
	{
		UINT Count = 0;
		float Padding[3];

		PointLight Lights[MAX_POINT_LIGHTS];
	};
	struct SpotLightDesc
	{
		UINT Count = 0;
		float Padding[3];

		SpotLight Lights[MAX_SPOT_LIGHTS];
	};
	struct FogDesc
	{
		Color FogColor;
		Vector2 FogDistance;
		float FogDensity;
		UINT FogType;
	};
private:
	Shader* shader;

	ConstantBuffer* buffer;
	ID3DX11EffectConstantBuffer* sBuffer;

	//ConstantBuffer* lightBuffer;
	ID3DX11EffectConstantBuffer* sLightBuffer;

	//ConstantBuffer* pointLightBuffer;
	ID3DX11EffectConstantBuffer* sPointLightBuffer;

	//ConstantBuffer* spotLightBuffer;
	ID3DX11EffectConstantBuffer* sSpotLightBuffer;

	//ConstantBuffer* fogBuffer;
	ID3DX11EffectConstantBuffer* sFogBuffer;
};