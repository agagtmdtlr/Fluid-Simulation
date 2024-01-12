struct LightDesc
{
    float4 Ambient;
    float4 Specular;
    float3 Direction;
    float Padding;
    float3 Position;
};

cbuffer CB_Light
{
    LightDesc GlobalLight;
};

// material data
Texture2D DiffuseMap;
Texture2D SpecularMap;
Texture2D NormalMap;
Texture2D ReflectionMap;
Texture2D RefractionMap;

TextureCube SkyCubeMap;

Texture2D ShadowMap;  // 깊이 맵 우리가 볼거
// Cmomparision은 비교함수를 수행한다.[깊이 비교]
SamplerComparisonState ShadowSampler;

// 아래 자료구조는 한 모델의 그룹이 가지게 되는 매터리얼이므로 모델 전체에 동일하게 적용됩니다.
// 하지만 개별 모델 마다 다르게 적용하고 싶어
// 각 모델의 값을 바꾸고 싶을 때는 인스턴스에 별도의 정보를 넣어야 합니다.
// 그래서 슬롯을 여러 개 사용할 수 있도록 수정했습니다. (인스턴스 버퍼를 추가할 수 있계)
struct MaterialDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emissive;
};

cbuffer CB_Material
{
    MaterialDesc Material;
}


MaterialDesc MakeMaterial() // 값 초기화 함수
{
    MaterialDesc output;
    output.Ambient = float4(0, 0, 0, 0);
    output.Diffuse = float4(0, 0, 0, 0);
    output.Specular = float4(0, 0, 0, 0);
    output.Emissive = float4(0, 0, 0, 0);

    return output;
}

void AddMaterial(inout MaterialDesc result, MaterialDesc val)
{
    result.Ambient += val.Ambient;
    result.Diffuse += val.Diffuse;
    result.Specular += val.Specular;
    result.Emissive += val.Emissive;
}

// 가산 혼합 (빛끼리는 더하기)
float3 MaterialToColor(MaterialDesc result)
{
    return (result.Ambient + result.Diffuse + result.Specular + result.Emissive).rgb;
}

//////////////////////////////////////////////////////////////////

void Texture(inout float4 color, Texture2D t, float2 uv, SamplerState samp)
{
    float4 sampling = t.Sample(samp, uv); // texture color
    color = color * sampling; // diffuse color * texture color;
}

void Texture(inout float4 color, Texture2D t, float2 uv)
{
    Texture(color, t, uv, LinearSampler);
}

																			   

/////////////////////////////////////////////////////////////////////////////////////////
// eye는 월드 변환된 포지션에서 계산해야된다.
// 감산혼합
// 가산혼합
void ComputeLight(out MaterialDesc output, float3 normal, float3 wPosition)
{
    //output = (MaterialDesc)0 같은 초기화 방식이다.
    output = MakeMaterial();
    
    float3 direction = -GlobalLight.Direction;
    float NdotL = dot(direction, normalize(normal));
    
    // ambient 종류 자기꺼 / 주광에서 오는거
    output.Ambient = GlobalLight.Ambient * Material.Ambient;
    
    float3 E = normalize(ViewPosition() - wPosition);
    
    [flatten]
    if (NdotL > 0.0f)
    {
        //이미 diffuse color와 texture의 연산을 완료된 상태에서 ndotl연산만 수행하면됨
        output.Diffuse = Material.Diffuse * NdotL;
        // 빛이 있을때만 specular 계산
        [flatten]
        if (Material.Specular.a > 0.0f)
        {
            // 반사식 /정규호(방향벡터)
            float3 R = normalize(reflect(-direction, normal));
            float3 H = normalize(direction + E);
            // saturate ( 0 ~ 1) 제한
            float RdotE = saturate(dot(R, E));            
            float NdotH = saturate(dot(H, normal));            
            // pow ( RdotE, shininess)
            float specular = pow(RdotE, Material.Specular.a);
            specular = pow(NdotH, Material.Specular.a);            
            // 반사되는 색이 바꿀 수 있도록 태양같은게 붉은 색이면 그거 맞게
            output.Specular = Material.Specular * specular * GlobalLight.Specular; 
            
            // Blinn specular :: 박사 벡터를 사용하지 않고 specular값을 구하는 방식
            //float3 HalfWay = normalize(E + direction);
            //float NDotH = saturate(dot(HalfWay, normal));
            //output.Specular = pow(NDotH, Material.Specular.a) *
            //GlobalLight.Specular * DiffuseMap.Sample();

        }
    }
    
    [flatten]
    if(Material.Emissive.a > 0.0f)
    {
        float NdotE = dot(E, normalize(normal));
        // ( 1-em) ~ 1 : 보간하다 Ndote로
        // lerp는 직선 선형 보간
        // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = Material.Emissive * emissive;
    }
}

/////////////////////////////////////////////////////////////////////////////
#define MAX_POINT_LIGHTS 256
struct PointLight
{
    // 포인트 라이팅도 물체의 색에 영향을 주므로 포인트 라이팅도 매터리얼을 가지고 있다.
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emissive;
    
    float3 Position; //

    float Range;
    
    float Intensity;
    // 배열로 버퍼의 값을 받기 위해 패딩을 사용합니다.
    // 만약 없으면 데이터가 당겨져 제대로 받을 수 없습니다.
    float3 Padding; 
};

cbuffer CB_PointLights
{
    uint PointLightCount;
    float3 CB_PointLights_Padding;
    
    PointLight PointLights[MAX_POINT_LIGHTS];
};


void ComputePointLight(inout MaterialDesc output, float3 normal, float3 wPosition)
{
    output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    // 기본 루프는 갯수만큼 복사가 아니라 참인거 아니거 다 복사한다
    // unroll은 무조거 주어진 갯수 까지만 복사 여기서 더 비효율적...
    //[unroll(MAX_POINT_LIGHTS)]
    for (uint i = 0; i < PointLightCount; i++)
    {
        float3 light = PointLights[i].Position - wPosition;
        float dist = length(light);
        
        // 정점과 포인트 라이트의 중심저믜 차가 조명의 거리보다 크다면 굳이 계산할 필요가 없음
        [flatten]
        if(dist > PointLights[i].Range)
            continue;
        
        light /= dist; // normalize;
        
        result.Ambient = PointLights[i].Ambient * Material.Ambient;
    
        float NdotL = dot(light, normalize(normal));
        float3 E = normalize(ViewPosition() - wPosition);
        
        
        [flatten]
        if (NdotL > 0.0f)
        {
        //이미 diffuse color와 texture의 연산을 완료된 상태에서 ndotl연산만 수행하면됨
            result.Diffuse = Material.Diffuse * NdotL * PointLights[i].Diffuse;
        // 빛이 있을때만 specular 계산
        [flatten]
            if (Material.Specular.a > 0.0f)
            {
            // 반사식 /정규호(방향벡터)
                float3 R = normalize(reflect(-light, normal));
                float3 H = normalize(light + E);                
            // saturate ( 0 ~ 1) 제한
                float RdotE = saturate(dot(R, E));
                float NdotH = saturate(dot(H, normal));
            
                float specular = pow(RdotE, Material.Specular.a);
                specular = pow(NdotH, Material.Specular.a);
                result.Specular = Material.Specular * specular * PointLights[i].Specular;
            }
        }
    
        [flatten]
        if (Material.Emissive.a > 0.0f)
        {
            float NdotE = dot(E, normalize(normal));
        // ( 1-em) ~ 1 : 보간하다 Ndote로
        // lerp는 직선 선형 보간
        // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
            float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
            output.Emissive = Material.Emissive * emissive * PointLights[i].Emissive;
        }
        
        // 계산을 수월하게 해주기 위해 값이 너무 커지는걸 방지 하기 위해 역수를 취해준다.
        // 1 /  현재 거리 / 범위 
        float temp = 1.0f / saturate(dist / PointLights[i].Range);
        // 역수를 취해줘서 가까운곳은 강도가 쎄고 먼곳을 강도가 약하게
        // 구(r^2)로 해서 강도를 줄여 나가는 과정 -> 색의 강도를 줄이는 방법
        // Intencity를 역수로 취해줘야 빛의 강도에 비례해서 값이 적용된다. 
        // 현재 위의 수식이 역수를 취한 결과이기 때문이다.
        // 0 ~ 1 ( 1 - Int) :
        float att = temp * temp * (1.0f / max(1.0f - PointLights[i].Intensity, 1e-8f));
        
        // 사방에서 들어오는 동일한 강도의 빛
        output.Ambient += result.Ambient * temp;
        output.Diffuse += result.Diffuse * att;
        output.Specular += result.Specular * att;
        output.Emissive += result.Emissive * att;
        
    }

}

///////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define MAX_SPOT_LIGHTS 256
struct SpotLight
{
    // 포인트 라이팅도 물체의 색에 영향을 주므로 포인트 라이팅도 매터리얼을 가지고 있다.
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emissive;
    
    float3 Position; //
    float Range;
    
    float3 Direction;
    float Angle;
    
    float Intensity;
    // 배열로 버퍼의 값을 받기 위해 패딩을 사용합니다.
    // 만약 없으면 데이터가 당겨져 제대로 받을 수 없습니다.
    float3 Padding;
};

cbuffer CB_SpotLights
{
    uint SpotLightCount;
    float3 CB_SpotLights_Padding;
    
    SpotLight SpotLights[MAX_SPOT_LIGHTS];
};


void ComputeSpotLight(inout MaterialDesc output, float3 normal, float3 wPosition)
{
    output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    
    for (uint i = 0; i < SpotLightCount; i++)
    {
        float3 light = SpotLights[i].Position - wPosition;
        float dist = length(light);
        
        // 정점과 포인트 라이트의 중심저믜 차가 조명의 거리보다 크다면 굳이 계산할 필요가 없음
        [flatten]
        if (dist > SpotLights[i].Range)
            continue;
        
        light /= dist; // normalize;
        
        result.Ambient = SpotLights[i].Ambient * Material.Ambient;
    
        float NdotL = dot(light, normalize(normal));
        float3 E = normalize(ViewPosition() - wPosition);
        
        [flatten]
        if (NdotL > 0.0f)
        {
        //이미 diffuse color와 texture의 연산을 완료된 상태에서 ndotl연산만 수행하면됨
            result.Diffuse = Material.Diffuse * NdotL * SpotLights[i].Diffuse;
        // 빛이 있을때만 specular 계산
        [flatten]
            if (Material.Specular.a > 0.0f)
            {
             // 반사식 /정규호(방향벡터)
                float3 R = normalize(reflect(-light, normal));
                float3 H = normalize(light + E);
            // saturate ( 0 ~ 1) 제한
                float RdotE = saturate(dot(R, E));
                float NdotH = saturate(dot(H, normal));
            
                float specular = pow(RdotE, Material.Specular.a);
                specular = pow(NdotH, Material.Specular.a);
                result.Specular = Material.Specular * specular * SpotLights[i].Specular;
            }
        }
    
        [flatten]
        if (Material.Emissive.a > 0.0f)
        {
            float NdotE = dot(E, normalize(normal));
        // ( 1-em) ~ 1 : 보간하다 Ndote로
        // lerp는 직선 선형 보간
        // smoothStep은 lerp와는 다르게 시작일도 값이 커졌다가 작아지므로 더 부드러운 표현 가능
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
            float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
            output.Emissive = Material.Emissive * emissive * SpotLights[i].Emissive;
        }
        
        // Pow :: 구를 형성하기 위해 사용한다. 
        float temp = pow(saturate(dot(-light, SpotLights[i].Direction)), (90 - SpotLights[i].Angle));
        float att = temp * (1.0f / max(1.0f - SpotLights[i].Intensity, 1e-8f));
        
        // 사방에서 들어오는 동일한 강도의 빛
        output.Ambient += result.Ambient * temp;
        output.Diffuse += result.Diffuse * att;
        output.Specular += result.Specular * att;
        output.Emissive += result.Emissive * att;
    }
}

////////////////////////////////////////////////////////////////////////////////
// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
void NormalMapping(float2 uv, float3 normal, float3 tangent, SamplerState samp)
{
    // obtain normal from normal map in range[0,1]
    float4 map = NormalMap.Sample(samp, uv);
    
    // any : 모든 변수의 값이 0이면 false/ 하나라도 0보다 크면 True
    [flatten]      
    if (any(map.rgb) == false)
        return;
    
    float3 coord = map.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    
    
    // 탄젝트 공간
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // 정규 직교화
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    // 단지 이미지 공간이었다 coord를 현재 우리가 만든 3D 공간으로 변환해준다.
    coord = mul(coord, TBN);
    // r g b => X Z Y
    // r : X
    // b : Y
    // g : Z
    
    
    // 음영은 본인이 가신 색이므로 Diffuse에 적요하명 동일하게 Lambert 식으로 계산
    Material.Diffuse *= saturate(dot(-GlobalLight.Direction, coord));
    
    
}

void NormalMappingByBase(float2 uv, float3 normal, float3 tangent, SamplerState samp)
{
    // obtain normal from normal map in range[0,1]
    float4 map = DiffuseMap.Sample(samp, uv);
    
    // any : 모든 변수의 값이 0이면 false/ 하나라도 0보다 크면 True
    [flatten]      
    if (any(map.rgb) == false)
        return;
    
    float3 coord = map.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    
    
    // 탄젝트 공간
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // 정규 직교화
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    // 단지 이미지 공간이었다 coord를 현재 우리가 만든 3D 공간으로 변환해준다.
    coord = mul(coord, TBN);
    // r g b => X Z Y
    // r : X
    // b : Y
    // g : Z    
    
    // 음영은 본인이 가신 색이므로 Diffuse에 적요하명 동일하게 Lambert 식으로 계산
    Material.Diffuse *= saturate(dot(-GlobalLight.Direction, coord));   
    
}


void NormalMapping(float2 uv, float3 normal, float3 tangent)
{
    NormalMapping(uv, normal, tangent, LinearSampler);
}

///////////////////////////////
void NormalMappingByMap(float2 uv, float3 normal, float3 tangent, SamplerState samp, Texture2D map)
{
    float4 m = map.Sample(samp, uv);
    [flatten]      
    if (any(m.rgb) == false)
        return;
    
    float3 coord = m.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // 정규 직교화
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    coord = mul(coord, TBN);
    
    float NdotL = dot(-GlobalLight.Direction, coord);
    //NdotL = pow(NdotL, 1/2.2);
    
    Material.Diffuse *= NdotL;
    
    
}


void NormalMappingByMap(float2 uv, float3 normal, float3 tangent, Texture2D map)
{
    NormalMappingByMap(uv, normal, tangent, LinearSampler, map);
}

float3 NormalMappingGetNormal(float2 uv, float3 normal, float3 tangent)
{
    float4 m = NormalMap.Sample(LinearSampler, uv);
    [flatten]      
    if (any(m.rgb) == false)
        return normal;
    
    float3 coord = m.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // 정규 직교화
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    coord = mul(coord, TBN);
    
    return coord;
}

/////////////////////////////////////////////////////////////////////////////
// 일반적인 빛처리 함수를 일괄적을 다 처리해주는 함수
// 노멀 맵핑 -> diffuse -> specular -> directlight -> pointLight -> spotLight
float4 PS_AllLight(MeshOutput input)
{
    NormalMapping(input.Uv, input.Normal, input.Tangent);
    
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    Texture(Material.Specular, SpecularMap, input.Uv);
    
    MaterialDesc output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    
    ComputeLight(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    
    ComputePointLight(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    
    ComputeSpotLight(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    
    float4 color = float4(MaterialToColor(result).rgb, 1.0f);
    color = CalculateFogColor(color, input.wPosition);
    
    return float4(color.rgb, 1.0f);    
}

////////////////////////////////////////////////////////////////////////////////
Texture2D ProjectorMap;

struct ProjectorDesc
{
    matrix View;
    matrix Projection;
    
    float4 Color;
};

cbuffer CB_Projector
{
    ProjectorDesc Projector;
};
							
void VS_Projector(inout float4 wvp, float4 oPosition)
{
    wvp = WorldPosition(oPosition);
    wvp = mul(wvp, Projector.View);
    wvp = mul(wvp, Projector.Projection);
}

void PS_Projector(inout float4 color, float4 wvp)
{
    float3 uvw = 0;
    
    // -1 ~ 1의 공간을 0 ~ 1의 공간으로
    uvw.x = wvp.x / wvp.w * 0.5f + 0.5f;
    // uv 좌표는 좌상단이 [0,0]이니깐 뒤집어 주기
    uvw.y = -wvp.y / wvp.w * 0.5f + 0.5f;
    // 0 ~ 1공간이 이미
    uvw.z = wvp.z / wvp.w;
    
    [flatten]
    if (saturate(uvw.x) == uvw.x && saturate(uvw.y) == uvw.y && saturate(uvw.z) == uvw.z)
    {
        // 해당 uv좌표로 텍스처 가져오기
        float4 map = ProjectorMap.Sample(LinearSampler, uvw.xy);
        // 보정값 처리 (더 가조하기 위해서
        map.rgb *= Projector.Color.rgb;
        // diffuse 와 projector map의 color 보간으로 해당 픽셀의 색 정하기
        color = lerp(color, map, map.a);
    }
    
}

/////////////////////////////////////////////////////////////////////////////////
cbuffer CB_Shadow
{
    matrix ShadowView;
    matrix ShadowProjection;
    
    float2 ShadowMapSize;
    float ShadowBias;
    
    uint ShadowQuality;
};

float4 PS_Shadow(float4 position, float4 color) : SV_Target
{
    // model color :: AllLight    
    // position =  sPosition : Light방향에서 wvp
    
    position.xyz /= position.w; // Light NDC transform
    
    // Light의 NDC 공간을 벗어난 곳이라면 그림자가 없는 공간이다.
    [flatten]
    if (position.x < -1.0f || position.x > +1.0f ||
        position.y < -1.0f || position.y > +1.0f ||
        position.z < +0.0f || position.z > +1.0f)
    {
        return color;
    }
    // ndc -> uv 좌표로 변화
    position.x = position.x * 0.5f + 0.5f;
    position.y = -position.y * 0.5f + 0.5f;
    
    float depth = 0;
    // 피터 패닝 보정값 shadowbias 
    float z = position.z - ShadowBias;
    float factor = 0;
    
    if (ShadowQuality == 0)
    {
        // detph를 r32로 썻으니깐 r성분을 읽어야 한다.
        depth = ShadowMap.Sample(LinearSampler, position.xy).r;
        // detph >= z 그림자 안지는곳
        // compare with shadow depth and element surface depth
        // 앞에껄 뒤집으면 된다.
        factor = (float) (depth >= z); // 0 or 1 false true  
    }
    else if (ShadowQuality == 1)
    {
        depth = position.z;
        //SampleBias[오차 보정]
        //SampCmp는 migmap이 존재하는 경우에 사용한다.
        factor = ShadowMap.SampleCmpLevelZero(ShadowSampler, position.xy, depth);
    }
    else if (ShadowQuality == 2) //PCF + Blur
    {
        depth = position.z;
        
        float2 size = 1.0f / ShadowMapSize; // 해상동 한픽셀크기
        float2 offsets[] =
        {
            float2(-size.x, -size.y), float2(0.0f, -size.y), float2(+size.x, -size.y), 
            float2(-size.x, 0.0f),float2(0.0f, 0.0f), float2(+size.x, 0.0f),
            float2(-size.x, +size.y), float2(0.0f, +size.y), float2(+size.x, +size.y),
        };
        
        
        float2 uv = 0;
        float sum = 0;
        
        [unroll(9)]
        for (int i = 0; i < 9; i++)
        {
            uv = position.xy + offsets[i];
            sum += ShadowMap.SampleCmpLevelZero(ShadowSampler, uv, depth).r;
        }
        
        factor = sum / 9.0f;
    }
    // 색이 자연스럽게 들어간다.
    factor = saturate(factor + depth);
    return float4(color.rgb * factor, 1);
}


/////////////////////////////////////////////////////////////////////////////////


