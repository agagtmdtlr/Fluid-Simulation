// <float4> 형식으로 이미지 어레이를 받갰다.
Texture2DArray<float4> Input;
// RW output 데이터 형식
RWTexture2DArray<float4> Output;

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    // DispatchThreadID는 그룹에 상관없이 자신의 스레드 방향으로 계속 증가되어 가므로
    // 결국 픽셀의 값이랑 일치하게 됩니다.
    float4 color = Input.Load(int4(id, 0));
    
    // 색상 반전
    //Output[id] = 1.0f - color;
    // 흑백화
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