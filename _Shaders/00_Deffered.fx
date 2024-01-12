#include "00_Light.fx"

RasterizerState Deffered_Rasterizer_State;
DepthStencilState Deffered_DepthStencil_State;

/////////////////////////////////////////////////////////////////////////////////////////////
// Pack GBuffer
/////////////////////////////////////////////////////////////////////////////////////////////

struct PixelOutuput_GBuffer
{
    float4 Diffuse : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Emissive : SV_Target2;
    float4 Normal : SV_Target3;
    float4 Tangent : SV_Target4;
};
// Gbuffer : MRT
PixelOutuput_GBuffer PS_PackGBuffer(MeshOutput input)
{    
    NormalMapping(input.Uv, input.Normal, input.Tangent);
    
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    Texture(Material.Specular, SpecularMap, input.Uv);
    
    PixelOutuput_GBuffer output;
    output.Diffuse = float4(Material.Diffuse.rgb, 1);
    output.Specular = Material.Specular; // a = power
    output.Emissive = Material.Emissive;
    
    // float -1 ~ 1 �״�� ����ص� �ȴ�. ���е��� ����. packing�� �� �ʿ䰡 ��������.
    output.Normal = float4(input.Normal, 1);
    output.Tangent = float4(input.Tangent, 1);
    
    // Linear Depth    
    //float2 xy = 1.0f / float2(projection._11, projection._22);
    // float z = projection._43;
    // float w = -projection._33;    
    // float depth = input.wvpPosition.z;
    // float d = z / (depth + w) * xy;
    // output.Depth = float4(d,d,d,1);
    
    
    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Unpack GBuffer
/////////////////////////////////////////////////////////////////////////////////////////////
Texture2D GBufferMaps[6];
// get world position material color , normal, tangent
void UnpackGBuffer(inout float4 position, in float2 screen, out MaterialDesc material, out float3 normal, out float3 tangent)
{
    material.Ambient = float4(0, 0, 0, 1);
    // SV_Position�� �̿��ؼ� load �ߴ�. // w�� �������� [ ���� ������ �������� ]
    // screen�� w�� �� ������ ��
    material.Diffuse = GBufferMaps[1].Load(int3(position.xy, 0));
    material.Specular = GBufferMaps[2].Load(int3(position.xy, 0));
    material.Emissive = GBufferMaps[3].Load(int3(position.xy, 0));
    
    normal = GBufferMaps[4].Load(int3(position.xy, 0)).rgb;
    tangent = GBufferMaps[5].Load(int3(position.xy, 0)).rgb;
    
    // ���ø��� ���� ���� �������� ��ȯ
    // world �������� ��ġ�� ����Ŵ�.
    // �ش� pixel �� ���� �������� ��ġ [ ���̸� �̿��ؼ� ]
    float2 xy = 1.0f / float2(Projection._11, Projection._22);
    float z = Projection._43;
    float w = -Projection._33;
    
    // R24G8_TYPELESS []
    float depth = GBufferMaps[0].Load(int3(position.xy, 0)).r;
    // ����
    float linearDepth = z / (depth + w);
    
    // xy ȭ�� ����
    // VP -1, + 1 -> P
    position.xy = screen.xy * xy * linearDepth;
    position.z = linearDepth;
    position.w = 1.0f;
    // ���� ���ۿ� ����� ���� ���� ���� ��ġ ���� ����� ����° ���� ������Ų �����.
    // ���� ���۸� ���� ���̷� �ٽ� ��ȯ�Ϸ��� ���ݻ� ������ ����ߴ� ����� �Ųٷ� �����ϱ⸸ �ϸ� �ȴ�.
    
    position = mul(position, ViewInverse);

}


/////////////////////////////////////////////////////////////////////////////////////////////
// DirectionalLight
/////////////////////////////////////////////////////////////////////////////////////////////
// cpu draw call index�� �ƴ� ������ ������ ��ο����ϴµ� 
//�̶� ������ ���̵� 0,1,2,3�������� ����Ȱ���Ѵ�.
// vertex Id�� ������ ���Ȱ���Ѵ�.
static const float2 NDC[4] = { float2(-1, +1), float2(+1, +1), float2(-1, -1), float2(+1, -1) };

struct VertexOutput_Directional
{
    float4 Position : SV_Position;
    float2 Screen : Position1;
};

// deffered vertex shader :: �ܼ��� id�� ��� vertexbuffer�� ������ ������� �ʴ´�.
VertexOutput_Directional VS_Directional(uint id : SV_VertexID)
{
    //Point List   
    VertexOutput_Directional output;
    
    output.Position = float4(NDC[id], 0, 1);
    output.Screen = output.Position.xy;
    
    return output;
}


void ComputeLight_Deffered(out MaterialDesc output,MaterialDesc material, float3 normal, float3 wPosition)
{
    //output = (MaterialDesc)0 ���� �ʱ�ȭ ����̴�.
    output = MakeMaterial();
    
    float3 direction = -GlobalLight.Direction;
    float NdotL = dot(direction, normalize(normal));
    
    // ambient ���� �ڱⲨ / �ֱ����� ���°�
    output.Ambient = GlobalLight.Ambient * material.Ambient;
    
    float3 E = normalize(ViewPosition() - wPosition);
    
    [flatten]
    if (NdotL > 0.0f)
    {
        //�̹� diffuse color�� texture�� ������ �Ϸ�� ���¿��� ndotl���길 �����ϸ��
        output.Diffuse = material.Diffuse * NdotL;
        // ���� �������� specular ���
        [flatten]
        if (any(material.Specular.rgb))            
        //if (material.Specular.a > 0.0f)
        {
            // �ݻ�� /����ȣ(���⺤��)
            float3 R = normalize(reflect(-direction, normal));
            // saturate ( 0 ~ 1) ����
            float RdotE = saturate(dot(R, E));
            // pow ( RdotE, shininess)
            float specular = pow(RdotE, material.Specular.a);
            // �ݻ�Ǵ� ���� �ٲ� �� �ֵ��� �¾簰���� ���� ���̸� �װ� �°�
            output.Specular = material.Specular * specular * GlobalLight.Specular;
            
            // Blinn specular :: �ڻ� ���͸� ������� �ʰ� specular���� ���ϴ� ���
            //float3 HalfWay = normalize(E + direction);
            //float NDotH = saturate(dot(HalfWay, normal));
            //output.Specular = pow(NDotH, material.Specular.a) *
            //GlobalLight.Specular * DiffuseMap.Sample();

        }
    }
    
    [flatten]
    if (any(material.Emissive.rgb))        
    //if (material.Emissive.a > 0.0f)
    {
        float NdotE = dot(E, normalize(normal));
        // ( 1-em) ~ 1 : �����ϴ� Ndote��
        // lerp�� ���� ���� ����
        // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
        //float emissive = lerp(1.0f - material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = material.Emissive * emissive;
    }
}

float4 PS_Directional(VertexOutput_Directional input) : SV_Target
{
    float4 position = input.Position; // ndc
    
    float3 normal = 0, tangent = 0;
    MaterialDesc material = MakeMaterial();
    
    UnpackGBuffer(position, input.Screen, material, normal, tangent);
    
    MaterialDesc result = MakeMaterial();
    ComputeLight_Deffered(result, material, normal, tangent);
    
    float3 color = MaterialToColor(result);
    color = pow(color, float3(1 / 2.2, 1 / 2.2, 1 / 2.2));
    return float4(color, 1);
    
    return float4(MaterialToColor(result), 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// PointLighting
/////////////////////////////////////////////////////////////////////////////////////////////
cbuffer CB_Deffered_PointLight
{
    float PointLight_TessFactor;
    float3 CB_Deffered_PointLight_Padding;
    
    matrix PointLight_Projection[MAX_POINT_LIGHTS];
    PointLight PointLight_Deffered[MAX_POINT_LIGHTS];
};

float4 VS_PointLights() : Poistion
{
    return float4(0,0,0,1); // �����ϴ� �� ���� ���̴��� �Ѱ��ֱ� ���� �뵵
}

struct CHullOutput_PointLights
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

// ��ġ�� �����ϱ� ���ϴ� �׼����̼� ���� ���� ����ü ���
CHullOutput_PointLights CHS_PointLights()
{
    CHullOutput_PointLights output;
    // ����Ʈ ����Ʈ �� ������ ���е�
    output.Edges[0] = output.Edges[1] = output.Edges[2] = output.Edges[3] = PointLight_TessFactor;
    output.Inside[0] = output.Inside[1] = PointLight_TessFactor;
    
    return output;
}

struct HullOutput_PointLights
{
    float4 Direction : Position;
};


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CHS_PointLights")]
HullOutput_PointLights HS_PointLights(uint id : SV_PrimitiveID)
{
    // ���� ���� ������ ����
    // �ݱ� ��翡 �����ϴ� ���� ��Ÿ�� ��� ��
    float4 direction[2] = { float4(1, 1, 1, 1), float4(-1, 1, -1, 1) };
    
    HullOutput_PointLights output;
    output.Direction = direction[id % 2];
    

    // �װ��� ���� ������ ���� ��ġ�� �����Ѵ�.
    return output;
}


struct DomainOutput_PointLights
{
    float4 Position : SV_Position;
    float2 Screen : Uv;
    uint PrimitiveID : Id;
};

[domain("quad")]
DomainOutput_PointLights DS_PointLights(CHullOutput_PointLights input, float2 uv : SV_DomainLocation,
    const OutputPatch<HullOutput_PointLights, 4> quad, uint id : SV_PrimitiveID)
{
    // uv -> [-1,1]�� NDC�������� ��ǥ�� ��ȥ��
    float2 clipSpace = uv.xy * 2.0f - 1.0f;
    
    // �߽ɿ��� ���� �� ������ ���� �Ÿ� ���
    float2 clipSpaceAbs = abs(clipSpace.xy);
    // �� ���� ū�� ���� ���̷� ����ϰڴ�.
    float maxLength = max(clipSpaceAbs.x, clipSpaceAbs.y);
    
    // (maxLength - 1.0f) ������� ���� ũ�� -1
    float3 direction = normalize(float3(clipSpace.xy, (maxLength - 1.0f)) * quad[0].Direction.xyz);
    float4 position = float4(direction, 1.0f);

    DomainOutput_PointLights output;
    output.Position = mul(position, PointLight_Projection[id / 2]);
    output.Screen = output.Position.xy / output.Position.w;
    output.PrimitiveID = id / 2;
    
    return output;
}

// point light mesh check
float4 PS_PointLights_Debug(DomainOutput_PointLights input) : SV_Target
{
    return float4(0, 1, 0, 1);
}

void ComputePointLight_Deffered(inout MaterialDesc output, uint id,MaterialDesc material, float3 normal, float3 wPosition)
{
    output = MakeMaterial();

    PointLight desc = PointLight_Deffered[id];

    
    float3 light = desc.Position - wPosition;
    float dist = length(light);
        
    // ������ ����Ʈ ����Ʈ�� �߽����� ���� ������ �Ÿ����� ũ�ٸ� ���� ����� �ʿ䰡 ����
    [flatten]
    if (dist > desc.Range)
        return;
        
    light /= dist; // normalize;
        
    output.Ambient = material.Ambient * desc.Ambient;
    
    float NdotL = dot(light, normalize(normal));
    float3 E = normalize(ViewPosition() - wPosition);
        
    [flatten]
    if (NdotL > 0.0f)
    {
        float3 R = normalize(reflect(-light, normal));
        float RdotE = saturate(dot(R, E));            
        float specular = pow(RdotE, material.Specular.a);
        
        output.Diffuse = material.Diffuse * NdotL * desc.Diffuse;
        output.Specular = material.Specular * specular * desc.Specular;
    }
    
    [flatten]
    if (Material.Emissive.a > 0.0f)
    {
        float NdotE = dot(E, normalize(normal));
    // ( 1-em) ~ 1 : �����ϴ� Ndote��
    // lerp�� ���� ���� ����
    // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
    //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = material.Emissive * emissive * desc.Emissive;
        
        
    // ����� �����ϰ� ���ֱ� ���� ���� �ʹ� Ŀ���°� ���� �ϱ� ���� ������ �����ش�.
    // 1 /  ���� �Ÿ� / ���� 
        float temp = 1.0f / saturate(dist / desc.Range);
    // ������ �����༭ �������� ������ ��� �հ��� ������ ���ϰ�
    // ��(r^2)�� �ؼ� ������ �ٿ� ������ ���� -> ���� ������ ���̴� ���
    // Intencity�� ������ ������� ���� ������ ����ؼ� ���� ����ȴ�. 
    // ���� ���� ������ ������ ���� ����̱� �����̴�.
    // 0 ~ 1 ( 1 - Int) :
        float att = temp * temp * (1.0f / max(1.0f - desc.Intensity, 1e-8f));
        
    // ��濡�� ������ ������ ������ ��
        output.Ambient = output.Ambient * temp;
        output.Diffuse = output.Diffuse * att;
        output.Specular = output.Specular * att;
        output.Emissive = output.Emissive * att;
    }

}

float4 PS_PointLights(DomainOutput_PointLights input) : SV_Target
{
    float4 position = input.Position; // ndc
    
    float3 normal = 0, tangent = 0;
    MaterialDesc material = MakeMaterial();
    
    UnpackGBuffer(position, input.Screen, material, normal, tangent);
    
    MaterialDesc result = MakeMaterial();
    ComputePointLight_Deffered(result,input.PrimitiveID, material, normal, position.xyz);
    
    float3 color = MaterialToColor(result);
    color = pow(color, float3(1 / 2.2, 1 / 2.2, 1 / 2.2));
    return float4(color, 1);
    return float4(MaterialToColor(result), 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SpotLighting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer CB_Deffered_SpotLight
{
    float SpotLight_TessFactor;
    float3 CB_Deffered_SpotLight_Padding;
    
    float4 SpotLight_Angle[MAX_SPOT_LIGHTS];
    matrix SpotLight_Projection[MAX_SPOT_LIGHTS];    
    
    SpotLight SpotLight_Deffered[MAX_SPOT_LIGHTS];
};

float4 VS_SpotLights() : Position
{
    return float4(0, 0, 0, 1);
}

struct ConstantHullOutput_SpotLights
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

ConstantHullOutput_SpotLights ConstantHS_SpotLights()
{
    ConstantHullOutput_SpotLights output;
    
    output.Edges[0] = output.Edges[1] = output.Edges[2] = output.Edges[3] = SpotLight_TessFactor;
    output.Inside[0] = output.Inside[1] = SpotLight_TessFactor;
   
    return output;
}

struct HullOutput_SpotLights
{
    float4 Position : Position;
};

// horn �ϳ��� �׸���.
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS_SpotLights")]
HullOutput_SpotLights HS_SpotLights()
{
    HullOutput_SpotLights output;

    output.Position = float4(0, 0, 0, 1);

    return output;
}

struct DomainOutput_SpotLights
{
    float4 Position : SV_Position;
    float2 Screen : Uv;
    uint PrimitiveID : Id;
};

[domain("quad")]
DomainOutput_SpotLights DS_SpotLights(ConstantHullOutput_SpotLights input, float2 uv : SV_DomainLocation,
    const OutputPatch<HullOutput_SpotLights, 4> quad, uint id : SV_PrimitiveID)
{
    float c = SpotLight_Angle[id].x;
    float s = SpotLight_Angle[id].y;
    
    // uv�� Ŭ�� �������� ��ȯ
    float2 clipSpace = uv.xy * float2(2, -2) + float2(-1, 1);
    
    // uv�� ���� ���ؽ� ������ ��� : �ؿ� �ѷ�
    float2 clipSpaceAbs = abs(clipSpace.xy);
    float maxLength = max(clipSpaceAbs.x, clipSpaceAbs.y);
    
    // ����Ʈ����Ʈ �޽��� �ʿ��� �����
    float cylinder = 0.2f;
    float expentAmount = (1.0f + cylinder); // ����
    
    // ���� ���ؽ��� �޽� ������ ���� ��ȯ
    float2 clipSpaceCylAbs = saturate(clipSpaceAbs * expentAmount);
    float maxLengthCapsule = max(clipSpaceCylAbs.x, clipSpaceCylAbs.y);
    float2 clipSpaceCyl = sign(clipSpace.xy) * clipSpaceCylAbs; // ���� �����
    
    // �ݱ� ��ġ�� �����ڸ��� ���� ���ؽ��� ��ȭ
    float3 halfSpherePosition = normalize(float3(clipSpaceCyl.xy, 1.0f - maxLengthCapsule));
    // ���� ���� �ظ� ũ��� ������ ����
    halfSpherePosition = normalize(float3(halfSpherePosition.xy * s, c));
    
    // ���� ���ؽ��� ������ ��� ���� �ظ��� 0
    float cylOffsetZ = saturate((maxLength * expentAmount - 1.0f) / cylinder);
    
    // ���� ���ؽ��� ���� ��ġ�� ������
    float4 position = 0;
    position.xy = halfSpherePosition.xy * (1.0f - cylOffsetZ);
    position.z = halfSpherePosition.z - cylOffsetZ * c;
    position.w = 1.0f;
    
    // ���� ��ȯ �� uv��ǥ�� ����
    DomainOutput_SpotLights output;
    output.Position = mul(position, SpotLight_Projection[id]);
    output.Screen = output.Position.xy / output.Position.w;
    output.PrimitiveID = id;
    
    return output;
}

void ComputeSpotLight_Deffered(inout MaterialDesc output, uint id, MaterialDesc material, float3 normal, float3 wPosition)
{
    output = MakeMaterial();
    
    SpotLight desc = SpotLight_Deffered[id];
    
   
    float3 light = desc.Position - wPosition;
    float dist = length(light);
        
    // ������ ����Ʈ ����Ʈ�� �߽����� ���� ������ �Ÿ����� ũ�ٸ� ���� ����� �ʿ䰡 ����
    [flatten]
    if (dist > desc.Range)
        return;
        
    light /= dist; // normalize;
        
    output.Ambient = desc.Ambient * material.Ambient;
    
    float NdotL = dot(light, normalize(normal));
    float3 E = normalize(ViewPosition() - wPosition);
        
    [flatten]
    if (NdotL > 0.0f)
    {
    //�̹� diffuse color�� texture�� ������ �Ϸ�� ���¿��� ndotl���길 �����ϸ��
        output.Diffuse = material.Diffuse * NdotL * desc.Diffuse;
    // ���� �������� specular ���
    [flatten]
        if (material.Specular.a > 0.0f)
        {
        // �ݻ�� /����ȣ(���⺤��)
            float3 R = normalize(reflect(-light, normal));
        // saturate ( 0 ~ 1) ����
            float RdotE = saturate(dot(R, E));
            
            float specular = pow(RdotE, material.Specular.a);
            output.Specular = material.Specular * specular * desc.Specular;
        }
    }
    
    [flatten]
    if (material.Emissive.a > 0.0f)
    {
        float NdotE = dot(E, normalize(normal));
    // ( 1-em) ~ 1 : �����ϴ� Ndote��
    // lerp�� ���� ���� ����
    // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
    //float emissive = lerp(1.0f - material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = material.Emissive * emissive * desc.Emissive;
    }
        
    // Pow :: ���� �����ϱ� ���� ����Ѵ�. 
    float temp = pow(saturate(dot(-light, desc.Direction)), (90 - desc.Angle));
    float att = temp * (1.0f / max(1.0f - desc.Intensity, 1e-8f));
        
    // ��濡�� ������ ������ ������ ��
    output.Ambient = output.Ambient * temp;
    output.Diffuse = output.Diffuse * att;
    output.Specular = output.Specular * att;
    output.Emissive = output.Emissive * att;
}

float4 PS_SpotLights_Debug(DomainOutput_SpotLights input) : SV_Target
{
    return float4(0, 1, 0, 1);
}
float4 PS_SpotLights(DomainOutput_SpotLights input) : SV_Target
{
    float4 position = input.Position; // ndc
    
    float3 normal = 0, tangent = 0;
    MaterialDesc material = MakeMaterial();
    
    UnpackGBuffer(position, input.Screen, material, normal, tangent);
    
    MaterialDesc result = MakeMaterial();
    ComputeSpotLight_Deffered(result, input.PrimitiveID, material, normal, position.xyz);
    
    float3 color = MaterialToColor(result);
    color = pow(color, float3(1 / 2.2, 1 / 2.2, 1 / 2.2));
    return float4(color, 1);
    return float4(MaterialToColor(result), 1);
}
