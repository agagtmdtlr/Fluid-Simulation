#define _GLOBAL_RENDER_

#define SWEIGHT (1.2f)
#define RELATIVE_POSITION (3.0 )

static const float R0 = 0.15f;
static const float PI = 3.14159265f;
static const float epsilon = 0.000001f;

static const float Water_RefractIndex = 0.75f;
static const float3 Water_Absorption = { 1, 0.1, 0.01 };

static const float intensity = 1.0f;
static const float specPower = 100.0f;

static const float thickness_blend_aplha = 0.005f;
static const float relative_scale = 3.0f;

float3 light = float3(1, -2, -1);

static const float2 NDC[4] = { float2(-1, +1), float2(+1, +1), float2(-1, -1), float2(+1, -1) };
static const float2 ScreenUV[4] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 1) };

Texture2D debugTexture;
Texture2D debugNormalTexture;

Texture2D<float> frontTargetTexture;
Texture2D<float> backTargetTexture;

Texture2D<float2> smoothTexture;
Texture2D<float4> backGroundTexture;

Texture2D<float> thicknessTexture;

TextureCube<float4> backGroundCube;
TextureCube<float4> debugCube;

DepthStencilState screenDSS;
DepthStencilState backGroundDSS;
DepthStencilState backFaceDSS;
DepthStencilState thicknessDSS;

BlendState thicknessBLS;
BlendState userBlendState;


SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;

    AddressU = Wrap;
    AddressV = Wrap;
};


RasterizerState CullMode_Front
{
    CullMode = front;
};

RasterizerState FrontCounterClockwise_True
{
    FrontCounterClockwise = true;
};

cbuffer cbInfo_Render
{
    matrix view;
    matrix projection;
    float3 boundary;
    float radius;
    matrix invView;
    matrix InvViewProjection;
    float far;
    float near;
    float width;
    float height;
    float deviation_texel;
    float deviation_depth;
    float filterRadius;
    float transparency;
    float3 diffuseColor;
    float distance_range;
    matrix shadowView;
    matrix shadowProjection;
    matrix invShadowView;
};

struct VertexInput
{
    float4 Position : Position;
    float2 Uv : UV;
    float3 iPosition : Inst1_Position;
    float3 iColor : Inst1_Color;
};


struct VertexOutput
{
    float4 Position : SV_Position;
    float2 Uv : UV;
    float3 wPosition : Position2;
    float3 colorData : Position3;
};

struct VertexInput_BG
{
    float4 Position : Position;
    float2 Uv : UV;
};
struct VertexOutput_BG
{
    float4 Position : SV_Position;
    float3 vPosition : Position1;
};

struct VertexOutput_ScreenSpace
{
    float4 Position : SV_Position;
    float2 Screen : Position1;
    float2 Uv : Uv0;
};


float schlick(float costheta, float F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - costheta, 5.0f);
}
