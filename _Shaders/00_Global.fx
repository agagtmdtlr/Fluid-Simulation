cbuffer CB_PerFrame
{
    matrix View; // View ��� 
    matrix ViewInverse; // View �����
    matrix Projection; // Projection ���
    matrix VP; // View Projection ���� ���    
    
    float4 Culling[4];
    float4 Clipping;
    
    float Time; // ���� �ð�
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
// �� �Ȱ� [fog] �������� �����ϴ� fog �Ÿ��� ���� ����ϴ� ���� �Ȱ��̴�.
// ���� �Ȱ� [ volume fog ]
cbuffer CB_Fog
{
    float4 FogColor;
    float2 FogDistance; //Linear Fog
    float FogDensity; //Exp Fog [�Ȱ��� ����]
    uint FogType; // �Ȱ��� ����
};



///////////////////////////////////////////////////////////////////////////////

static const float PI = 3.14159265f;
// �߷� : ���� ȸ���� ���Ⱚ
static const float G = -0.980f;
static const float G2 = -0.980f * -0.980f;


///////////////////////////////////////////////////////////////////////////////
// WVP ��ȯ �Լ�
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

// ī�޶��� ��ġ�� ���ؿ��� �Լ�
float3 ViewPosition()
{
    return ViewInverse._41_42_43;
}

// PS_AllLight color�� ��ġ�� 
float4 LinearFogBlend(float4 color, float3 wPosition)
{
    float dist = distance(wPosition, ViewPosition()); // �Ÿ�
    // sat(dist - fogdist.x) �ּ�  sat ( y + x ) �ִ� 
    // ���� ����� ������ �����.
    float factor = saturate((dist - FogDistance.x) / (FogDistance.y + FogDistance.x));
    // �ָ� ������ �������� �Ȱ��� ���ִ´�.
    return float4(lerp(color.rgb, FogColor.rgb, factor), 1);
}

float4 ExpFogBlend(float4 color, float3 wPosition)
{
    float dist = distance(wPosition, ViewPosition());
    dist = dist / FogDistance.y * FogDistance.x;
    
    float factor = exp(-dist * FogDensity);
    // �����̴ϱ� fog ~ color�� ����ó���Ѵ� ���� ���� ��fog�� �޶�����.
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
// �ڼ� ����ϴ� ���� �ڷ���
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
    float4 wvpPosition : Position3; // WVP �⺻ 
    float4 wvpPosition_Sub : Position4; // WVP Projector�� [Projector�� ������� �ʴ´ٸ� Position3�� ����]
    float4 sPosition : Position5; // Light dir - WVP [ Shadow VP position]
    float4 gPosition : Position6; // Geometry Position
    
    
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float2 Uv : Uv;
    
    float4 Culling : SV_CullDistance; //x - left, y - right, z - near, w - far
    float4 Clipping : SV_ClipDistance; //x - used
};

// dynamic cube map�� ���鶧 ����Ҽ� �ִ�.
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
    // ���� ������Ʈ�� ���̴����� ó���ϴ»ﰢ���� ���� Ÿ���� Array�߿� �� ���� 
    // ������ ������ ����!
    // �츮�� �ý��ۿ� ������ �ִ� ��
    uint TargetIndex : SV_RenderTargetArrayIndex;
    // ť��ʵ� array�� �����Ǿ� �־ rendertarget�� ť������� �����Ǿ��ִ�.
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
// �ݽð�  ���� skysphere cubemap �Ҷ� �������
RasterizerState FrontCounterClockwise_True
{
    FrontCounterClockwise = true;
};

RasterizerState FillMode_WireFrame
{
    FillMode = WireFrame;
};

// �߶��� ����
RasterizerState CullMode_None
{
    CullMode = None;
};

// �߶��� ����
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

    // ����Ÿ���� MRT �Ҽ� �ִ�.
    // �� ������ 0�� ����Ÿ��;
    // blend�� ���귮�� �����Ƿ� �ʿ��Ѱ������� �������� Ų��.
    BlendEnable[0] = true;
    SrcBlend[0] = SRC_ALPHA;
    DestBlend[0] = INV_SRC_ALPHA;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F ��� ���� ���ٴ�. �Ϲݺ����Τ�����
};

BlendState AlphaBlend_AlphaToCoverageEnable
{
    // �ܰ����� ���� ����ϰ�
    AlphaToCoverageEnable = true;

    // ����Ÿ���� MRT �Ҽ� �ִ�.
    // �� ������ 0�� ����Ÿ��;
    // blend�� ���귮�� �����Ƿ� �ʿ��Ѱ������� �������� Ų��.
    BlendEnable[0] = true;
    SrcBlend[0] = SRC_ALPHA; // �׸����� �ο��� ���İ� ���� ���β� ���ٴ�. src_alpha
    DestBlend[0] = INV_SRC_ALPHA; // �׷��� ���� �ο��� ���İ� ����  1 -src_alpha
    BlendOp[0] = ADD;
    
    
    SrcBlendAlpha[0] = One; // ������ ���İ��� ��ġ�� �󸶳� ����
    DestBlendAlpha[0] = Zero; // �׷��� �ִ� ���İ��� ��ġ�� �󸶳� ����(�̰� ���߿� �ٽ� ���캸��)
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F ��� ���� ���ٴ�. �Ϲݺ����Τ�����
};

BlendState AdditiveBlend // �λ��� ��������� �� ���� ���ϰ� �����.
{
    AlphaToCoverageEnable = false;

    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = One;
    BlendOp[0] = ADD;
    
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F ��� ���� ���ٴ�. �Ϲݺ����Τ�����
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

    RenderTargetWriteMask[0] = 15; // 0x0F ��� ���� ���ٴ�. �Ϲݺ����Τ�����
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
    
    SrcBlendAlpha[0] = One; // ������ ���İ��� ��ġ�� �󸶳� ����
    DestBlendAlpha[0] = Zero; // �׷��� �ִ� ���İ��� ��ġ�� �󸶳� ����(�̰� ���߿� �ٽ� ���캸��)
    BlendOpAlpha[0] = Add;

    RenderTargetWriteMask[0] = 15; // 0x0F ��� ���� ���ٴ�. �Ϲݺ����Τ�����
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

