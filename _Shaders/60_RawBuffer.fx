ByteAddressBuffer Input; // 입력이 있다면 사용 SRV
RWByteAddressBuffer Output; // UAV

struct Group
{
    uint3 GroupID;
    uint3 GroupThreadID;
    uint3 DispatchThreadID;
    uint GroupIndex;
    float RetValue;
};

struct ComputeInput
{
    uint3 GroupID : SV_GroupID;
    uint3 GroupThreadID : SV_GroupThreadID;
    // 전체 그룹에서의 thread id
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;
};

// thread group내에서 thread를 분할
[numthreads(10, 8, 3)] 
void CS(ComputeInput input)
{
    Group group;
    // 안정성을 보장하는 캐스팅 asuint
    group.GroupID = asuint(input.GroupID);
    group.GroupThreadID = asuint(input.GroupThreadID);
    group.DispatchThreadID = asuint(input.DispatchThreadID);
    group.GroupIndex = asuint(input.GroupIndex);
    
    // write address
    // groupid * group size+ index
    uint index = input.GroupID.x * 10 * 8 * 3 + input.GroupIndex;
    // 10 = uint3 + uint3 + uint3 + uint + float
    // 4 = 그룹내에서의 인덱스
    uint outAddress = index * 11 * 4;
    
    uint inAddress = index * 4;    
    group.RetValue = asfloat(Input.Load(inAddress));
    
    Output.Store3(outAddress + 0, asuint(group.GroupID)); // 12
    Output.Store3(outAddress + 12, asuint(group.GroupThreadID)); // 24
    Output.Store3(outAddress + 24, asuint(group.DispatchThreadID)); // 36
    Output.Store(outAddress + 36, asuint(group.GroupIndex)); // 40
    // float 형리ㅏ도 asuint으로 캐스팅해서 내보내야 합니다.
    Output.Store(outAddress + 40, asuint(group.RetValue));

}


technique11 __XB_S_BCNT0_U32
{
    pass P0
    {
        // 기본 shader라서 null로 만들어 줘야 한다.
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        
        SetComputeShader(CompileShader(cs_5_0, CS()));

    }

}