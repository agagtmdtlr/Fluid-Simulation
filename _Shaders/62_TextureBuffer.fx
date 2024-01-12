// <float4> �������� �̹��� ��̸� �ް���.
Texture2DArray<float4> Input;
// RW output ������ ����
RWTexture2DArray<float4> Output;

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    // DispatchThreadID�� �׷쿡 ������� �ڽ��� ������ �������� ��� �����Ǿ� ���Ƿ�
    // �ᱹ �ȼ��� ���̶� ��ġ�ϰ� �˴ϴ�.
    float4 color = Input.Load(int4(id, 0));
    
    // ���� ����
    //Output[id] = 1.0f - color;
    // ���ȭ
    Output[id] = (color.r + color.g + color.b) / 3.0f;
}

technique11 T0
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, CS()));
    }
}