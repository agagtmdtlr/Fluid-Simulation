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

Texture2D ShadowMap;  // ���� �� �츮�� ����
// Cmomparision�� ���Լ��� �����Ѵ�.[���� ��]
SamplerComparisonState ShadowSampler;

// �Ʒ� �ڷᱸ���� �� ���� �׷��� ������ �Ǵ� ���͸����̹Ƿ� �� ��ü�� �����ϰ� ����˴ϴ�.
// ������ ���� �� ���� �ٸ��� �����ϰ� �;�
// �� ���� ���� �ٲٰ� ���� ���� �ν��Ͻ��� ������ ������ �־�� �մϴ�.
// �׷��� ������ ���� �� ����� �� �ֵ��� �����߽��ϴ�. (�ν��Ͻ� ���۸� �߰��� �� �ְ�)
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


MaterialDesc MakeMaterial() // �� �ʱ�ȭ �Լ�
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

// ���� ȥ�� (�������� ���ϱ�)
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
// eye�� ���� ��ȯ�� �����ǿ��� ����ؾߵȴ�.
// ����ȥ��
// ����ȥ��
void ComputeLight(out MaterialDesc output, float3 normal, float3 wPosition)
{
    //output = (MaterialDesc)0 ���� �ʱ�ȭ ����̴�.
    output = MakeMaterial();
    
    float3 direction = -GlobalLight.Direction;
    float NdotL = dot(direction, normalize(normal));
    
    // ambient ���� �ڱⲨ / �ֱ����� ���°�
    output.Ambient = GlobalLight.Ambient * Material.Ambient;
    
    float3 E = normalize(ViewPosition() - wPosition);
    
    [flatten]
    if (NdotL > 0.0f)
    {
        //�̹� diffuse color�� texture�� ������ �Ϸ�� ���¿��� ndotl���길 �����ϸ��
        output.Diffuse = Material.Diffuse * NdotL;
        // ���� �������� specular ���
        [flatten]
        if (Material.Specular.a > 0.0f)
        {
            // �ݻ�� /����ȣ(���⺤��)
            float3 R = normalize(reflect(-direction, normal));
            float3 H = normalize(direction + E);
            // saturate ( 0 ~ 1) ����
            float RdotE = saturate(dot(R, E));            
            float NdotH = saturate(dot(H, normal));            
            // pow ( RdotE, shininess)
            float specular = pow(RdotE, Material.Specular.a);
            specular = pow(NdotH, Material.Specular.a);            
            // �ݻ�Ǵ� ���� �ٲ� �� �ֵ��� �¾簰���� ���� ���̸� �װ� �°�
            output.Specular = Material.Specular * specular * GlobalLight.Specular; 
            
            // Blinn specular :: �ڻ� ���͸� ������� �ʰ� specular���� ���ϴ� ���
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
        // ( 1-em) ~ 1 : �����ϴ� Ndote��
        // lerp�� ���� ���� ����
        // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
        float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
        output.Emissive = Material.Emissive * emissive;
    }
}

/////////////////////////////////////////////////////////////////////////////
#define MAX_POINT_LIGHTS 256
struct PointLight
{
    // ����Ʈ �����õ� ��ü�� ���� ������ �ֹǷ� ����Ʈ �����õ� ���͸����� ������ �ִ�.
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emissive;
    
    float3 Position; //

    float Range;
    
    float Intensity;
    // �迭�� ������ ���� �ޱ� ���� �е��� ����մϴ�.
    // ���� ������ �����Ͱ� ����� ����� ���� �� �����ϴ�.
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
    // �⺻ ������ ������ŭ ���簡 �ƴ϶� ���ΰ� �ƴϰ� �� �����Ѵ�
    // unroll�� ������ �־��� ���� ������ ���� ���⼭ �� ��ȿ����...
    //[unroll(MAX_POINT_LIGHTS)]
    for (uint i = 0; i < PointLightCount; i++)
    {
        float3 light = PointLights[i].Position - wPosition;
        float dist = length(light);
        
        // ������ ����Ʈ ����Ʈ�� �߽����� ���� ������ �Ÿ����� ũ�ٸ� ���� ����� �ʿ䰡 ����
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
        //�̹� diffuse color�� texture�� ������ �Ϸ�� ���¿��� ndotl���길 �����ϸ��
            result.Diffuse = Material.Diffuse * NdotL * PointLights[i].Diffuse;
        // ���� �������� specular ���
        [flatten]
            if (Material.Specular.a > 0.0f)
            {
            // �ݻ�� /����ȣ(���⺤��)
                float3 R = normalize(reflect(-light, normal));
                float3 H = normalize(light + E);                
            // saturate ( 0 ~ 1) ����
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
        // ( 1-em) ~ 1 : �����ϴ� Ndote��
        // lerp�� ���� ���� ����
        // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
            float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
            output.Emissive = Material.Emissive * emissive * PointLights[i].Emissive;
        }
        
        // ����� �����ϰ� ���ֱ� ���� ���� �ʹ� Ŀ���°� ���� �ϱ� ���� ������ �����ش�.
        // 1 /  ���� �Ÿ� / ���� 
        float temp = 1.0f / saturate(dist / PointLights[i].Range);
        // ������ �����༭ �������� ������ ��� �հ��� ������ ���ϰ�
        // ��(r^2)�� �ؼ� ������ �ٿ� ������ ���� -> ���� ������ ���̴� ���
        // Intencity�� ������ ������� ���� ������ ����ؼ� ���� ����ȴ�. 
        // ���� ���� ������ ������ ���� ����̱� �����̴�.
        // 0 ~ 1 ( 1 - Int) :
        float att = temp * temp * (1.0f / max(1.0f - PointLights[i].Intensity, 1e-8f));
        
        // ��濡�� ������ ������ ������ ��
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
    // ����Ʈ �����õ� ��ü�� ���� ������ �ֹǷ� ����Ʈ �����õ� ���͸����� ������ �ִ�.
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emissive;
    
    float3 Position; //
    float Range;
    
    float3 Direction;
    float Angle;
    
    float Intensity;
    // �迭�� ������ ���� �ޱ� ���� �е��� ����մϴ�.
    // ���� ������ �����Ͱ� ����� ����� ���� �� �����ϴ�.
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
        
        // ������ ����Ʈ ����Ʈ�� �߽����� ���� ������ �Ÿ����� ũ�ٸ� ���� ����� �ʿ䰡 ����
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
        //�̹� diffuse color�� texture�� ������ �Ϸ�� ���¿��� ndotl���길 �����ϸ��
            result.Diffuse = Material.Diffuse * NdotL * SpotLights[i].Diffuse;
        // ���� �������� specular ���
        [flatten]
            if (Material.Specular.a > 0.0f)
            {
             // �ݻ�� /����ȣ(���⺤��)
                float3 R = normalize(reflect(-light, normal));
                float3 H = normalize(light + E);
            // saturate ( 0 ~ 1) ����
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
        // ( 1-em) ~ 1 : �����ϴ� Ndote��
        // lerp�� ���� ���� ����
        // smoothStep�� lerp�ʹ� �ٸ��� �����ϵ� ���� Ŀ���ٰ� �۾����Ƿ� �� �ε巯�� ǥ�� ����
        //float emissive = lerp(1.0f - Material.Emissive, 1.0f, 1.0f - saturate(NdotE));
            float emissive = smoothstep(1.0f - Material.Emissive.a, 1.0f, 1.0f - saturate(NdotE));
        
            output.Emissive = Material.Emissive * emissive * SpotLights[i].Emissive;
        }
        
        // Pow :: ���� �����ϱ� ���� ����Ѵ�. 
        float temp = pow(saturate(dot(-light, SpotLights[i].Direction)), (90 - SpotLights[i].Angle));
        float att = temp * (1.0f / max(1.0f - SpotLights[i].Intensity, 1e-8f));
        
        // ��濡�� ������ ������ ������ ��
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
    
    // any : ��� ������ ���� 0�̸� false/ �ϳ��� 0���� ũ�� True
    [flatten]      
    if (any(map.rgb) == false)
        return;
    
    float3 coord = map.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    
    
    // ź��Ʈ ����
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // ���� ����ȭ
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    // ���� �̹��� �����̾��� coord�� ���� �츮�� ���� 3D �������� ��ȯ���ش�.
    coord = mul(coord, TBN);
    // r g b => X Z Y
    // r : X
    // b : Y
    // g : Z
    
    
    // ������ ������ ���� ���̹Ƿ� Diffuse�� �����ϸ� �����ϰ� Lambert ������ ���
    Material.Diffuse *= saturate(dot(-GlobalLight.Direction, coord));
    
    
}

void NormalMappingByBase(float2 uv, float3 normal, float3 tangent, SamplerState samp)
{
    // obtain normal from normal map in range[0,1]
    float4 map = DiffuseMap.Sample(samp, uv);
    
    // any : ��� ������ ���� 0�̸� false/ �ϳ��� 0���� ũ�� True
    [flatten]      
    if (any(map.rgb) == false)
        return;
    
    float3 coord = map.rgb * 2.0f - 1.0f; // -1 ~ + 1;
    
    
    
    // ź��Ʈ ����
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N) * N); // ���� ����ȭ
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    // ���� �̹��� �����̾��� coord�� ���� �츮�� ���� 3D �������� ��ȯ���ش�.
    coord = mul(coord, TBN);
    // r g b => X Z Y
    // r : X
    // b : Y
    // g : Z    
    
    // ������ ������ ���� ���̹Ƿ� Diffuse�� �����ϸ� �����ϰ� Lambert ������ ���
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
    float3 T = normalize(tangent - dot(tangent, N) * N); // ���� ����ȭ
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
    float3 T = normalize(tangent - dot(tangent, N) * N); // ���� ����ȭ
    float3 B = cross(N, T); // Binormal
    
    float3x3 TBN = float3x3(T, B, N);
    coord = mul(coord, TBN);
    
    return coord;
}

/////////////////////////////////////////////////////////////////////////////
// �Ϲ����� ��ó�� �Լ��� �ϰ����� �� ó�����ִ� �Լ�
// ��� ���� -> diffuse -> specular -> directlight -> pointLight -> spotLight
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
    
    // -1 ~ 1�� ������ 0 ~ 1�� ��������
    uvw.x = wvp.x / wvp.w * 0.5f + 0.5f;
    // uv ��ǥ�� �»���� [0,0]�̴ϱ� ������ �ֱ�
    uvw.y = -wvp.y / wvp.w * 0.5f + 0.5f;
    // 0 ~ 1������ �̹�
    uvw.z = wvp.z / wvp.w;
    
    [flatten]
    if (saturate(uvw.x) == uvw.x && saturate(uvw.y) == uvw.y && saturate(uvw.z) == uvw.z)
    {
        // �ش� uv��ǥ�� �ؽ�ó ��������
        float4 map = ProjectorMap.Sample(LinearSampler, uvw.xy);
        // ������ ó�� (�� �����ϱ� ���ؼ�
        map.rgb *= Projector.Color.rgb;
        // diffuse �� projector map�� color �������� �ش� �ȼ��� �� ���ϱ�
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
    // position =  sPosition : Light���⿡�� wvp
    
    position.xyz /= position.w; // Light NDC transform
    
    // Light�� NDC ������ ��� ���̶�� �׸��ڰ� ���� �����̴�.
    [flatten]
    if (position.x < -1.0f || position.x > +1.0f ||
        position.y < -1.0f || position.y > +1.0f ||
        position.z < +0.0f || position.z > +1.0f)
    {
        return color;
    }
    // ndc -> uv ��ǥ�� ��ȭ
    position.x = position.x * 0.5f + 0.5f;
    position.y = -position.y * 0.5f + 0.5f;
    
    float depth = 0;
    // ���� �д� ������ shadowbias 
    float z = position.z - ShadowBias;
    float factor = 0;
    
    if (ShadowQuality == 0)
    {
        // detph�� r32�� �����ϱ� r������ �о�� �Ѵ�.
        depth = ShadowMap.Sample(LinearSampler, position.xy).r;
        // detph >= z �׸��� �����°�
        // compare with shadow depth and element surface depth
        // �տ��� �������� �ȴ�.
        factor = (float) (depth >= z); // 0 or 1 false true  
    }
    else if (ShadowQuality == 1)
    {
        depth = position.z;
        //SampleBias[���� ����]
        //SampCmp�� migmap�� �����ϴ� ��쿡 ����Ѵ�.
        factor = ShadowMap.SampleCmpLevelZero(ShadowSampler, position.xy, depth);
    }
    else if (ShadowQuality == 2) //PCF + Blur
    {
        depth = position.z;
        
        float2 size = 1.0f / ShadowMapSize; // �ػ� ���ȼ�ũ��
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
    // ���� �ڿ������� ����.
    factor = saturate(factor + depth);
    return float4(color.rgb * factor, 1);
}


/////////////////////////////////////////////////////////////////////////////////


