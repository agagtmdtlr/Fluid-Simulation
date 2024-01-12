//////////////////////////////////////////////////////////////////////////////////////////////////////
// Billboard
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VertexBillboard
{
    float4 Position : Position;
    float2 Scale : Scale;
    uint MapIndex : MapIndex;
    // ���� ������ �ε����� �˷��ش�.
    // uint VertexIndex : SV_VertexID;
};

struct BillboardOutput
{
    float4 Position : Position; // geometry�� �ѱ�� �Ŵϱ� SV_Poistion�� ���� �ʴ´�.
    float2 Scale : Scale;
    uint MapIndex : MapIndex;
};


BillboardOutput VS(VertexBillboard input)
{
    BillboardOutput output;
    // VS ���� world ��ȯ��
    // GS ���� ViewProjection ��ȯ�� �����Ѵ�.
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
//Matxvertexcount�� ������Ʈ������ ��µ� �ִ밳��
// ������ �ϳ��� ������ point ������ ������ triangle 
// point input[1] / triangle input[3] 
// ������Ʈ�� ���̴��� ���� ����� �ƴ� TriangleStream�� ������ �߰����ִ� ������� ó���Ѵ�.
// template
[maxvertexcount(4)]
void GS_Billboard(point BillboardOutput input[1], inout TriangleStream<GeometryOutput> stream)
{
    // ���� ��ȯ�� �ϳ��� �������� �س����� 
    // 4���� �÷��� ���� �� ������ ���庯ȯ�� ��ų �ʿ䰡 ��� ����.
    float3 up = float3(0, 1, 0);
    //float3 forward = float3(0, 0, 1);
    float3 forward = input[0].Position.xyz - ViewPosition();
    float3 right = normalize(cross(up, forward));
    
    float2 size = input[0].Scale * 0.5f;
    
    float4 position[4];
    
    // -0.5 -0.5 +1.0���ϴ�.
    position[0] = float4(input[0].Position.xyz - size.x * right - size.y * up, 1);
    position[1] = float4(input[0].Position.xyz - size.x * right + size.y * up, 1);
    position[2] = float4(input[0].Position.xyz + size.x * right - size.y * up, 1);
    position[3] = float4(input[0].Position.xyz + size.x * right + size.y * up, 1);
    
    
    float2 uv[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
    };
    
    GeometryOutput output;
    
    // triangle strip���� �׸���.
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        output.Position = ViewProjection(position[i]);
        output.Uv = uv[i];
        output.MapIndex = input[0].MapIndex;
        
        stream.Append(output);
    }

}

// ���� �ϳ��� ���� �ΰ��� �����ϹǷ� MaxvertexCount�� 8���� �˴ϴ�.
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
    
    // -0.5 -0.5 +1.0���ϴ�.
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
        if (i % 4 == 3) // �Ѹ��� �׸��� �ι�° ���� �׸������� ���� �ٽ� ����
            stream.RestartStrip();

    }

}

Texture2DArray BillboardMap;
float4 PS_Billboard(GeometryOutput input) : SV_Target
{
    // �迭 �ε����� ����� �����ϴ�...
    // �׷��� Textrue2DArray�� ����Ѵ�. ���⼭ �Է°��� float3(uv.x , uv.y , arrayIndex )
    return BillboardMap.Sample(LinearSampler, float3(input.Uv, input.MapIndex)) * 1.75f;
}

//////////////////////////////////////////////////////////////////////////////
