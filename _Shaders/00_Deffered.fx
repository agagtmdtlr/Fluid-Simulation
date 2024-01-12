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
    
    // float -1 ~ 1 그대로 사용해도 된다. 정밀도도 좋다. packing을 할 필요가 없어진다.
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
    // SV_Position을 이용해서 load 했다. // w로 나눠진값 [ 원본 비율로 나눠진값 ]
    // screen은 w가 안 나눠진 값
    material.Diffuse = GBufferMaps[1].Load(int3(position.xy, 0));
    material.Specular = GBufferMaps[2].Load(int3(position.xy, 0));
    material.Emissive = GBufferMaps[3].Load(int3(position.xy, 0));
    
    normal = GBufferMaps[4].Load(int3(position.xy, 0)).rgb;
    tangent = GBufferMaps[5].Load(int3(position.xy, 0)).rgb;
    
    // 샘플링된 깊이 값을 선형으로 변환
    // world 공간상의 위치를 만들거다.
    // 해당 pixel 의 월드 공간상의 위치 [ 깊이를 이용해서 ]
    float2 xy = 1.0f / float2(Projection._11, Projection._22);
    float z = Projection._43;
    float w = -Projection._33;
    
    // R24G8_TYPELESS []
    float depth = GBufferMaps[0].Load(int3(position.xy, 0)).r;
    // 깊이
    float linearDepth = z / (depth + w);
    
    // xy 화면 비율
    // VP -1, + 1 -> P
    position.xy = screen.xy * xy * linearDepth;
    position.z = linearDepth;
    position.w = 1.0f;
    // 깊이 버퍼에 저장된 값은 월드 공간 위치 값을 행렬의 세번째 열과 내적시킨 결과다.
    // 깊이 버퍼를 선형 깊이로 다시 변환하려면 전반사 정도를 계산했던 방법을 거꾸로 수행하기만 하면 된다.
    
    position = mul(position, ViewInverse);

}


/////////////////////////////////////////////////////////////////////////////////////////////
// DirectionalLight
/////////////////////////////////////////////////////////////////////////////////////////////
// cpu draw call index가 아닌 정점을 가지고 드로우콜하는데 
//이때 정점의 아이디 0,1,2,3을가지고 접근활용한다.
// vertex Id를 가지고 사용활용한다.
static const float2 NDC[4] = { float2(-1, +1), float2(+1, +1), float2(-1, -1), float2(+1, -1) };

struct VertexOutput_Directional
{
    float4 Position : SV_Position;
    float2 Screen : Position1;
};

// deffered vertex shader :: 단순한 id로 사용 vertexbuffer를 실제로 사용하지 않는다.
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
    //output = (MaterialDesc)0 같은 초기화 방식이다.
    output = MakeMaterial();
    
    float3 direction = -GlobalLight.Direction;
    float NdotL = dot(direction, normalize(normal));
    
    // ambient 종류 자기꺼 / 주광에서 오는거
    output.Ambient = GlobalLight.Ambient * material.Ambient;
    
    float3 E = normalize(ViewPosition() - wPosition);
    
    [flatten]
    if (NdotL > 0.0f)
    {
        //이미 diffuse color와 texture의 연산을 완료된 상태에서 ndotl연산만 수행하면됨
        output.Diffuse = material.Diffuse * NdotL;
        // 빛이 있을때만 specular 계산
        [flatten]
        if (any(material.Specular.rgb))            
        //if (material.Specular.a > 0.0f)
        {
            // 반사식 /정규호(방향벡터)
            float3 R = normalize(reflect(-direction, normal));
            // saturate ( 0 ~ 1) 제한
            float RdotE = saturate(dot(R, E));
            // pow ( RdotE, shininess)
            float specular = pow(RdotE, material.Specular.a);
            // 반사되는 색이 바꿀 수 있도록 태양같은게 붉은 색이면 그거 맞게
            output.Specular = material.Specular * specular * GlobalLight.Specular;
            
            // Blinn specular :: 박사 벡터를 사용하지 않고 specular값을 구하는 방식
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
        // ( 1-em) ~ 1 : 보간하다 Ndote로
        // lerp는 직선 선형 보간
        // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
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
    return float4(0,0,0,1); // 사용안하는 값 다음 쉐이더로 넘겨주기 위한 용도
}

struct CHullOutput_PointLights
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

// 패치에 적용하기 원하는 테셀레이션 값에 대한 구조체 출력
CHullOutput_PointLights CHS_PointLights()
{
    CHullOutput_PointLights output;
    // 포인트 라이트 구 영역의 정밀도
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
    // 구를 만들 방향을 정의
    // 반구 모양에 상응하는 양의 평타도 상수 값
    float4 direction[2] = { float4(1, 1, 1, 1), float4(-1, 1, -1, 1) };
    
    HullOutput_PointLights output;
    output.Direction = direction[id % 2];
    

    // 네개의 제어 지점에 대한 패치를 생성한다.
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
    // uv -> [-1,1]의 NDC공간상의 좌표로 변혼한
    float2 clipSpace = uv.xy * 2.0f - 1.0f;
    
    // 중심에서 가장 먼 지점의 절댓값 거리 계싼
    float2 clipSpaceAbs = abs(clipSpace.xy);
    // 두 값중 큰걸 구의 넓이로 사용하겠다.
    float maxLength = max(clipSpaceAbs.x, clipSpaceAbs.y);
    
    // (maxLength - 1.0f) 가운데에서 가장 크가 -1
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
        
    // 정점과 포인트 라이트의 중심저믜 차가 조명의 거리보다 크다면 굳이 계산할 필요가 없음
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
    // ( 1-em) ~ 1 : 보간하다 Ndote로
    // lerp는 직선 선형 보간
    // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
    //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = material.Emissive * emissive * desc.Emissive;
        
        
    // 계산을 수월하게 해주기 위해 값이 너무 커지는걸 방지 하기 위해 역수를 취해준다.
    // 1 /  현재 거리 / 범위 
        float temp = 1.0f / saturate(dist / desc.Range);
    // 역수를 취해줘서 가까운곳은 강도가 쎄고 먼곳을 강도가 약하게
    // 구(r^2)로 해서 강도를 줄여 나가는 과정 -> 색의 강도를 줄이는 방법
    // Intencity를 역수로 취해줘야 빛의 강도에 비례해서 값이 적용된다. 
    // 현재 위의 수식이 역수를 취한 결과이기 때문이다.
    // 0 ~ 1 ( 1 - Int) :
        float att = temp * temp * (1.0f / max(1.0f - desc.Intensity, 1e-8f));
        
    // 사방에서 들어오는 동일한 강도의 빛
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

// horn 하나만 그린다.
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
    
    // uv를 클립 공간으로 변환
    float2 clipSpace = uv.xy * float2(2, -2) + float2(-1, 1);
    
    // uv에 대한 버텍스 오프셋 계산 : 밑에 둘레
    float2 clipSpaceAbs = abs(clipSpace.xy);
    float maxLength = max(clipSpaceAbs.x, clipSpaceAbs.y);
    
    // 스포트라이트 메쉬에 필요한 상수값
    float cylinder = 0.2f;
    float expentAmount = (1.0f + cylinder); // 부피
    
    // 원뿔 버텍스를 메시 엣지로 강제 변환
    float2 clipSpaceCylAbs = saturate(clipSpaceAbs * expentAmount);
    float maxLengthCapsule = max(clipSpaceCylAbs.x, clipSpaceCylAbs.y);
    float2 clipSpaceCyl = sign(clipSpace.xy) * clipSpaceCylAbs; // 방향 만들기
    
    // 반구 위치를 가장자리의 원뿔 버텍스로 변화
    float3 halfSpherePosition = normalize(float3(clipSpaceCyl.xy, 1.0f - maxLengthCapsule));
    // 구를 원뿔 밑면 크기로 스케일 조정
    halfSpherePosition = normalize(float3(halfSpherePosition.xy * s, c));
    
    // 월뿐 버텍스의 오프셋 계산 원뿔 밑면은 0
    float cylOffsetZ = saturate((maxLength * expentAmount - 1.0f) / cylinder);
    
    // 원뿔 버텍스를 최종 위치로 오프셋
    float4 position = 0;
    position.xy = halfSpherePosition.xy * (1.0f - cylOffsetZ);
    position.z = halfSpherePosition.z - cylOffsetZ * c;
    position.w = 1.0f;
    
    // 투영 변환 및 uv좌표값 생성
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
        
    // 정점과 포인트 라이트의 중심저믜 차가 조명의 거리보다 크다면 굳이 계산할 필요가 없음
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
    //이미 diffuse color와 texture의 연산을 완료된 상태에서 ndotl연산만 수행하면됨
        output.Diffuse = material.Diffuse * NdotL * desc.Diffuse;
    // 빛이 있을때만 specular 계산
    [flatten]
        if (material.Specular.a > 0.0f)
        {
        // 반사식 /정규호(방향벡터)
            float3 R = normalize(reflect(-light, normal));
        // saturate ( 0 ~ 1) 제한
            float RdotE = saturate(dot(R, E));
            
            float specular = pow(RdotE, material.Specular.a);
            output.Specular = material.Specular * specular * desc.Specular;
        }
    }
    
    [flatten]
    if (material.Emissive.a > 0.0f)
    {
        float NdotE = dot(E, normalize(normal));
    // ( 1-em) ~ 1 : 보간하다 Ndote로
    // lerp는 직선 선형 보간
    // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
    //float emissive = lerp(1.0f - material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = material.Emissive * emissive * desc.Emissive;
    }
        
    // Pow :: 구를 형성하기 위해 사용한다. 
    float temp = pow(saturate(dot(-light, desc.Direction)), (90 - desc.Angle));
    float att = temp * (1.0f / max(1.0f - desc.Intensity, 1e-8f));
        
    // 사방에서 들어오는 동일한 강도의 빛
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
