#pragma once
/////////////////////////////////////////////////////////////////////////////
#define MAX_POINT_LIGHTS 256
struct PointLight
{
	// 포인트 라이팅도 물체의 색에 영향을 주므로 포인트 라이팅도 매터리얼을 가지고 있다.
	Color Ambient;
	Color Diffuse;
	Color Specular;
	Color Emissive;

	Vector3 Position; //

	float Range;

	float Intensity;
	Vector3 Padding;
};

#define MAX_SPOT_LIGHTS 256
struct SpotLight
{
	Color Ambient;
	Color Diffuse;
	Color Specular;
	Color Emissive;

	Vector3 Position;
	float Range;

	Vector3 Direction;
	float Angle;

	float Intensity;
	float Padding[3];
};

struct SpotLightPBR
{
	Vector3 Position;
	float Range;

	Vector3 Direction;
	float OuterAngle;

	Vector3 color;
	float InnerAngle;

	float Intensity;
	float pad[3];

	
};

// 싱글톤
// 프로그램 전역에서 객체를 n개 이하로 제한하여 생성해서 사용할 때 유용합니다.
class Lighting
{
public:
	static Lighting* Get();
	static void Create();
	static void Delete();

private:
	Lighting();
	~Lighting();

public:
	UINT PointLightCount() { return pointLightCount; }
	UINT PointLights(OUT PointLight* lights);
	void AddPointLight(PointLight& light);
	PointLight& GetPointLight(UINT index);

public:
	UINT SpotLightCount() { return spotLightCount; }
	UINT SpotLights(OUT SpotLight* lights);
	void AddSpotLight(SpotLight& light);
	SpotLight& GetSpotLight(UINT index);

public:
	UINT SpotLightPBRCount() { return spotLightPBRCount; }
	UINT SpotLightPBRs(OUT SpotLightPBR* lights);
	void AddSpotLightPBR(SpotLightPBR& light);
	SpotLightPBR& GetSpotLightPBR(UINT index);

	Color& FogColor() { return fogColor; }
	Vector2& FogDistance() { return fogDistance; }
	float& FogDensity() { return fogDensity; }
	UINT& FogType() { return fogType; }
	

private:
	static Lighting* instance;

private:
	UINT pointLightCount = 0;
	PointLight pointLights[MAX_POINT_LIGHTS];

private:
	UINT spotLightCount = 0;
	SpotLight spotLights[MAX_SPOT_LIGHTS];

private:
	UINT spotLightPBRCount = 0;
	SpotLightPBR spotLightPBRs[MAX_SPOT_LIGHTS];

	Color fogColor = Color(1, 1, 1, 1);
	Vector2 fogDistance = Vector2(1, 105);
	float fogDensity = 0.5f;
	UINT fogType = 3;
};