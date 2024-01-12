#ifndef _GLOBAL_RENDER_
    #include "00_Global_render.fx"
#endif



struct VertexInput_Plane
{
    float4 Position : Position;
    float2 Uv : Uv;
    float3 Normal : Normal;
    float3 Tangent : Tangent;    
    matrix Transform : Inst1_Transform;
};

struct VertexOutput_Plane
{
    float4 Position : SV_Position;
    float3 wPosition : Position1;
    float2 Uv : UV0;
};

VertexOutput_Plane vsPlane(VertexInput_Plane input)
{
    VertexOutput_Plane output;
    output.Position = mul(input.Position, input.Transform);
    output.wPosition = output.Position.xyz;
    output.Position = mul(output.Position, view);
    output.Position = mul(output.Position, projection);
    output.Uv = input.Uv;
    return output;
}

float4 psPlane(VertexOutput_Plane input) : SV_Target
{
    float4 output = float4(0.5, 0.5, 0.5, 1);
    return output;
}






