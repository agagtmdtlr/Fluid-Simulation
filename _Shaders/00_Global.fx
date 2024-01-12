cbuffer CB_PerFrame
{
    matrix View; // View 행렬 
    matrix ViewInverse; // View 역행렬
    matrix Projection; // Projection 행렬
    matrix VP; // View Projection 결합 행렬    
    
    float4 Culling[4];
    float4 Clipping;
    
    float Time; // 게임 시간
};

cbuffer CB_World
{
    matrix World;
};

cbuffer CB_Reflection
{
    matrix Reflection;
};


// PS_ALLLight -> PS->Fog
// 면 안개 [fog] 수업에서 구현하는 fog 거리를 통해 계산하는 선형 안개이다.
// 공간 안개 [ volume fog ]
cbuffer CB_Fog
{
    float4 FogColor;
    float2 FogDistance; //Linear Fog
    float FogDensity; //Exp Fog [안개의 강도]
    uint FogType; // 안개의 종류
};



///////////////////////////////////////////////////////////////////////////////

static const float PI = 3.14159265f;
// 중력 : 빛의 회절에 영향값
static const float G = -0.980f;
static const float G2 = -0.980f * -0.980f;


///////////////////////////////////////////////////////////////////////////////
// WVP 변환 함수
float4 WorldPosition(float4 position)
{
    return mul(position, World);
}

float4 ViewChange(float4 position)
{
    return mul(position, View);
}

float4 ProjectionPosition(float4 position)
{
    return mul(position, Projection);
}

float4 ViewProjection(float4 position)
{
    return mul(position, VP);

    //position = mul(position, View);
    //return mul(position, Projection);
}

float3 WorldNormal(float3 normal)
{
    return mul(normal, (float3x3) World);
}

float3 WorldTangent(float3 tangent)
{
    return mul(tangent, (float3x3) World);
}

// 카메라의 위치를 구해오는 함수
float3 ViewPosition()
{
    return ViewInverse._41_42_43;
}

// PS_AllLight color와 합치기 
float4 LinearFogBlend(float4 color, float3 wPosition)
{
    float dist = distance(wPosition, ViewPosition()); // 거리
    // sat(dist - fogdist.x) 최소  sat ( y + x ) 최대 
    // 연산 결과는 비율이 낭노다.
    float factor = saturate((dist - FogDistance.x) / (FogDistance.y + FogDistance.x));
    // 멀리 떨어져 있을수록 안개가 껴있는다.
    return float4(lerp(color.rgb, FogColor.rgb, factor), 1);
}

float4 ExpFogBlend(float4 color, float3 wPosition)
{
    float dist = distance(wPosition, ViewPosition());
    dist = dist / FogDistance.y * FogDistance.x;
    
    float factor = exp(-dist * FogDensity);
    // 역수이니깐 fog ~ color로 보간처리한다 위의 선형 ㄹfog는 달라진다.
    return float4(lerp(FogColor.rgb, color.rgb, factor), 1);
}

float4 Exp2FogBlend(float4 color, float3 wPosition)
{
    float dist = distance(wPosition, ViewPosition());
    dist = dist / FogDistance.y * FogDistance.x;
    
    float factor = exp(-(dist * FogDensity) * (dist * FogDensity));
    return float4(lerp(FogColor.rgb, color.rgb, factor), 1);
}

float4 CalculateFogColor(float4 color, float3 wPosition)
{
    if (FogType == 1)
        color = LinearFogBlend(color, wPosition);
    else if (FogType == 2)
        color = ExpFogBlend(color, wPosition);
    else if (FogType == 3)
        color = Exp2FogBlend(color, wPosition);
    
    return color;
}

///////////////////////////////////////////////////////////////////////////////
// 자수 사용하는 정점 자료형
struct Vertex
{
    float4 Position : Position;
};

struct VertexNormal
{
    float4 Position : Position;
    float3 Normal : Normal;
};

struct VertexColor
{
    float4 Position : Position;
    float4 Color : Color;
};

struct VertexColorNormal
{
    float4 Position : Position;
    float4 Color : Color;
    float3 Normal : Normal;
};

struct VertexTexture
{
    float4 Position : Position;
    float2 Uv : Uv;
};

struct VertexTextureNormal
{
    float4 Position : Position;
    float2 Uv : Uv;
    float3 Normal : Normal;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct MeshOutput
{
    float4 Position : SV_Position0; // Rasterizing Position;
    float3 oPosition : Position1; // Original Position;
    float3 wPosition : Position2; // World Position 
    float4 wvpPosition : Position3; // WVP 기본 
    float4 wvpPosition_Sub : Position4; // WVP Projector용 [Projector를 사용하지 않는다면 Position3과 같다]
    float4 sPosition : Position5; // Light dir - WVP [ Shadow VP position]
    float4 gPosition : Position6; // Geometry Position
    
    
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float2 Uv : Uv;
    
    float4 Culling : SV_CullDistance; //x - left, y - right, z - near, w - far
    float4 Clipping : SV_ClipDistance; //x - used
};

// dynamic cube map을 만들때 사용할수 있다.
struct MeshGeometryOutput
{
    float4 Position : SV_Position0; // Rasterizing Position;
    float4 wvpPosition : Position1; //WVP Position
    float4 wvpPosition_Sub : Position2; //WVP Sub Position
    float3 oPosition : Position3; //Original
    float3 wPosition : Position4; //World Position
    float4 sPosition : Position5; //Shaodw VP Position
    float4 gPosition : Position6; //Geometry Position;
    
    
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float2 Uv : Uv;
    
    // array index symentic
    // 현재 지오메트리 세이더에서 처리하는삼각형이 렌더 타깃의 Array중에 몇 변에 
    // 렌더링 할지를 결정!
    // 우리가 시스템에 전달해 주는 값
    uint TargetIndex : SV_RenderTargetArrayIndex;
    // 큐브맵도 array로 구성되어 있어서 rendertarget각 큐브면으로 지정되어있다.
};

/// use mesh tessellation
struct MeshTessVertexOutput
{
    float4 Position : Position; // Rasterizing Position;
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float2 Uv : Uv;   
    matrix Transform : Transform;
};

struct CHullOutput_Mesh
{
    float Edge[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
};

struct HullOutput_Mesh
{
    float4 Position : Position;    
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float2 Uv : Uv;    
    matrix Transform : Transform;
};


struct SSR_MeshOutput
{
    float4 Position : SV_Position; // Rasterizing Position;
    float4 ViewPosition : Position1;
    float3 ViewNormal : Normal;
    float3 ViewTangent : Tangent;
    float3 csPos : Position2;
};

MeshOutput ConvertMeshOutput(MeshGeometryOutput input)
{
    MeshOutput output;
    
    output.Position = input.Position;            
    output.wvpPosition = input.wvpPosition;
    output.wvpPosition_Sub = input.wvpPosition_Sub;
    output.oPosition = input.oPosition;
    output.wPosition = input.wPosition;
    output.sPosition = input.sPosition;
    output.gPosition = input.gPosition;
    
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    output.Uv = input.Uv;
    //output.Color = input.Color;
    
    return output;
}
///////////////////////////////////////////////////////////////////////////////

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState MipLinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;

    AddressU = Wrap;
    AddressV = Wrap;
    MinLod = 0;
    MaxLod = 5;
};

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;

    AddressU = Wrap;
    AddressV = Wrap;
};
// 반시계  방향 skysphere cubemap 할때 사용했음
RasterizerState FrontCounterClockwise_True
{
    FrontCounterClockwise = true;
};

RasterizerState FillMode_WireFrame
{
    FillMode = WireFrame;
};

// 잘라내지 마라
RasterizerState CullMode_None
{
    CullMode = None;
};

// 잘라내지 마라
RasterizerState FillMode_WireFrame_CullMode_None
{
    FillMode = WireFrame;
    CullMode = None;
};


DepthStencilState DepthEnable_False
{
    DepthEnable = false;
};

DepthStencilState DepthStencilEnable_False
{
    DepthEnable = false;
    StencilEnable = false;
};

DepthStencilState DepthRead_Particle
{
    DepthEnable = true;
    DepthFunc = Less_Equal;
    DepthWriteMask = 0;
    //DepthWriteMask = 0;
};

BlendState OpaqueBlend
{
    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = Zero;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;
    
    RenderTargetWriteMask[0] = 15; //Ox0F
};


BlendState AlphaBlend
{
    AlphaToCoverageEnable = false;

    // 렌더타겟을 MRT 할수 있다.
    // 주 렌더링 0번 렌더타켓;
    // blend는 연산량이 많으므로 필요한곳에서만 불투명을 킨다.
    BlendEnable[0] = true;
    SrcBlend[0] = SRC_ALPHA;
    DestBlend[0] = INV_SRC_ALPHA;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F 모든 색을 섞겟다. 일반벚ㄱ인ㅌ세팅
};

BlendState AlphaBlend_AlphaToCoverageEnable
{
    // 외관선을 좀더 깔끔하게
    AlphaToCoverageEnable = true;

    // 렌더타겟을 MRT 할수 있다.
    // 주 렌더링 0번 렌더타켓;
    // blend는 연산량이 많으므로 필요한곳에서만 불투명을 킨다.
    BlendEnable[0] = true;
    SrcBlend[0] = SRC_ALPHA; // 그릴색에 부여할 알파값 비중 본인껄 쓰겟다. src_alpha
    DestBlend[0] = INV_SRC_ALPHA; // 그려진 색에 부여할 알파값 비중  1 -src_alpha
    BlendOp[0] = ADD;
    
    
    SrcBlendAlpha[0] = One; // 들어오는 알파값의 수치를 얼마나 쓸지
    DestBlendAlpha[0] = Zero; // 그려져 있던 알파값의 수치를 얼마나 쓸지(이건 나중에 다시 살펴보기)
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F 모든 색을 섞겟다. 일반벚ㄱ인ㅌ세팅
};

BlendState AdditiveBlend // 두색을 섞어버리는 더 색을 진하게 만든다.
{
    AlphaToCoverageEnable = false;

    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = One;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F 모든 색을 섞겟다. 일반벚ㄱ인ㅌ세팅
};

BlendState Additive
{
    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = One;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = One;
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F 모든 색을 섞겟다. 일반벚ㄱ인ㅌ세팅
};

BlendState AdditiveBlend_Particle
{
    AlphaToCoverageEnable = false;
    
    BlendEnable[0] = true;
    SrcBlend[0] = SRC_ALPHA;
    DestBlend[0] = One;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;
    
    RenderTargetWriteMask[0] = 15; //Ox0F
};


BlendState AdditiveBlend_AlphaToCoverageEnable
{
    AlphaToCoverageEnable = true;

    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = One;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One; // 들어오는 알파값의 수치를 얼마나 쓸지
    DestBlendAlpha[0] = Zero; // 그려져 있던 알파값의 수치를 얼마나 쓸지(이건 나중에 다시 살펴보기)
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F 모든 색을 섞겟다. 일반벚ㄱ인ㅌ세팅
};



///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Vertex / Pixel
///////////////////////////////////////////////////////////////////////////////

#define P_VP(name, vs, ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_VP(name, rs, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_BS_VP(name, bs, vs, ps) \
pass name \
{ \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_VP(name, dss, vs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_VP(name, rs, dss, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_BS_VP(name, rs, bs, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_BS_VP(name, dss, bs, vs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_BS_VP(name,rs, dss, bs, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}


///////////////////////////////////////////////////////////////////////////////
// Vertex / Geometry / Pixel
///////////////////////////////////////////////////////////////////////////////
#define P_VGP(name, vs, gs, ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_VGP(name, rs, vs, gs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_BS_VGP(name, bs, vs, gs, ps) \
pass name \
{ \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_VGP(name, dss, vs, gs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_VGP(name, rs, dss, vs, gs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_BS_VGP(name, rs, bs, vs, gs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_BS_VGP(name, dss, bs, vs, gs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}


///////////////////////////////////////////////////////////////////////////////
// Vertex / Tessellation / Pixel
///////////////////////////////////////////////////////////////////////////////
#define P_VTP(name, vs, hs, ds, ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_VTP(name, rs, vs, hs, ds, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_BS_VTP(name, bs, vs, hs, ds, ps) \
pass name \
{ \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_VTP(name, dss, vs, hs, ds, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_VTP(name, rs, dss, vs, hs, ds, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_BS_VTP(name, rs, bs, vs, hs, ds, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_DSS_BS_VTP(name, dss, bs, vs, hs, ds, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_BS_VTP(name, rs, dss, bs, vs, hs, ds, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetHullShader(CompileShader(hs_5_0, hs())); \
    SetDomainShader(CompileShader(ds_5_0, ds())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

