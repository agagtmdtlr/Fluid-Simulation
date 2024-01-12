//////////////////////////////////////////////////////////////////////////////////////////////////////
// Billboard
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexBillboard
{
    float4 Position : Position;
    float2 Scale : Scale;
    uint MapIndex : MapIndex;
    // 들어온 정점의 인덱스를 알려준다.
    // uint VertexIndex : SV_VertexID;
};

struct BillboardOutput
{
    float4 Position : Position; // geometry로 넘기는 거니깐 SV_Poistion이 붙지 않는다.
    float2 Scale : Scale;
    uint MapIndex : MapIndex;
};


BillboardOutput VS(VertexBillboard input)
{
    BillboardOutput output;
    // VS 에서 world 변환만
    // GS 에서 ViewProjection 변환을 수행한다.
    output.Position = WorldPosition(input.Position);
    output.Scale = input.Scale;
    output.MapIndex = input.MapIndex;
 
    return output;
    
}

struct GeometryOutput
{
    float4 Position : SV_Position;
    float2 Uv : Uv;
    uint MapIndex : MapIndex;
};

// geometry shader attribute
//Matxvertexcount란 지오메트리에서 출력된 최대개수
// 정점을 하나씩 받으면 point 세개씩 받으면 triangle 
// point input[1] / triangle input[3] 
// 지오메트리 세이더는 리턴 방식이 아닌 TriangleStream에 정점을 추가해주는 방식으로 처리한다.
// template
[maxvertexcount(4)]
void GS_Billboard(point BillboardOutput input[1], inout TriangleStream<GeometryOutput> stream)
{
    // 월드 변환을 하나의 정점에서 해놓으면 
    // 4개로 늘려서 각각 또 일일히 월드변환을 시킬 필요가 없어서 좋다.
    float3 up = float3(0, 1, 0);
    //float3 forward = float3(0, 0, 1);
    float3 forward = input[0].Position.xyz - ViewPosition();
    float3 right = normalize(cross(up, forward));
    
    float2 size = input[0].Scale * 0.5f;
    
    float4 position[4];
    
    // -0.5 -0.5 +1.0좌하단.
    position[0] = float4(input[0].Position.xyz - size.x * right - size.y * up, 1);
    position[1] = float4(input[0].Position.xyz - size.x * right + size.y * up, 1);
    position[2] = float4(input[0].Position.xyz + size.x * right - size.y * up, 1);
    position[3] = float4(input[0].Position.xyz + size.x * right + size.y * up, 1);
    
    
    float2 uv[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
    };
    
    GeometryOutput output;
    
    // triangle strip으로 그린다.
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        output.Position = ViewProjection(position[i]);
        output.Uv = uv[i];
        output.MapIndex = input[0].MapIndex;
        
        stream.Append(output);
    }

}

// 정점 하나로 면을 두개를 생성하므로 MaxvertexCount는 8개가 됩니다.
[maxvertexcount(16)]
void GS_Cross(point BillboardOutput input[1], inout TriangleStream<GeometryOutput> stream)
{
    float3 up = float3(0, 1, 0);
    
    
    float3 forward = float3(0, 0, 1);
    float3 forward2 = normalize(float3(1, 0, 1));
    
    float3 right = normalize(cross(up, forward));
    float3 right2 = normalize(cross(up, forward2));
    
    float2 size = input[0].Scale * 0.5f;
    
    float4 position[16];
    
    // -0.5 -0.5 +1.0좌하단.
    position[0] = float4(input[0].Position.xyz - size.x * right - size.y * up, 1);
    position[1] = float4(input[0].Position.xyz - size.x * right + size.y * up, 1);
    position[2] = float4(input[0].Position.xyz + size.x * right - size.y * up, 1);
    position[3] = float4(input[0].Position.xyz + size.x * right + size.y * up, 1);
    
    position[4] = float4(input[0].Position.xyz - size.x * forward - size.y * up, 1);
    position[5] = float4(input[0].Position.xyz - size.x * forward + size.y * up, 1);
    position[6] = float4(input[0].Position.xyz + size.x * forward - size.y * up, 1);
    position[7] = float4(input[0].Position.xyz + size.x * forward + size.y * up, 1);
    
    position[8] = float4(input[0].Position.xyz - size.x * right2 - size.y * up, 1);
    position[9] = float4(input[0].Position.xyz - size.x * right2 + size.y * up, 1);
    position[10] = float4(input[0].Position.xyz + size.x * right2 - size.y * up, 1);
    position[11] = float4(input[0].Position.xyz + size.x * right2 + size.y * up, 1);
    
    position[12] = float4(input[0].Position.xyz - size.x * forward2 - size.y * up, 1);
    position[13] = float4(input[0].Position.xyz - size.x * forward2 + size.y * up, 1);
    position[14] = float4(input[0].Position.xyz + size.x * forward2 - size.y * up, 1);
    position[15] = float4(input[0].Position.xyz + size.x * forward2 + size.y * up, 1);
    
    
    float2 uv[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
    };
    
    GeometryOutput output;
    
    [unroll(16)]
    for (int i = 0; i < 16; i++)
    {
        output.Position = ViewProjection(position[i]);
        output.Uv = uv[i % 4];
        output.MapIndex = input[0].MapIndex;
        
        stream.Append(output);
        
        [flatten]
        if (i % 4 == 3) // 한면을 그리고 두번째 면을 그리지전에 끊고 다시 시작
            stream.RestartStrip();

    }

}

Texture2DArray BillboardMap;
float4 PS_Billboard(GeometryOutput input) : SV_Target
{
    // 배열 인덱스는 상수만 가능하다...
    // 그래서 Textrue2DArray를 사용한다. 여기서 입력값은 float3(uv.x , uv.y , arrayIndex )
    return BillboardMap.Sample(LinearSampler, float3(input.Uv, input.MapIndex)) * 1.75f;
}

//////////////////////////////////////////////////////////////////////////////
