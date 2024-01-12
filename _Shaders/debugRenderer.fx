cbuffer cbInfo_Debug
{
    matrix view;
    matrix projection;
};


struct VertexInput
{
    float4 Position : Position;
    float3 iPosition : Inst1_Position;
};

struct VertexOutput
{
    float4 Position : SV_Position;
};


VertexOutput vs(VertexInput input)
{
    VertexOutput output;
    output.Position = input.Position;
    
    matrix scaleup =
    {
        float4(input.iPosition.x * 3, 0, 0, 0), float4(0, input.iPosition.y * 3, 0, 0), float4(0, 0, input.iPosition.z * 3, 0), float4(input.iPosition.xyz * 3 / 2, 1)
    };
    
    output.Position = mul(output.Position, scaleup);
    output.Position = mul(output.Position, view);
    output.Position = mul(output.Position, projection);

    return output;
}


float4 ps(VertexOutput input) : SV_Target
{
    return float4(0, 1, 0 , 1);
}

technique11 T0
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, vs()));
        SetPixelShader(CompileShader(ps_5_0, ps()));
    }
}