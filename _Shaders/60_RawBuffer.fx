ByteAddressBuffer Input; // �Է��� �ִٸ� ��� SRV
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
    // ��ü �׷쿡���� thread id
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;
};

// thread group������ thread�� ����
[numthreads(10, 8, 3)] 
void CS(ComputeInput input)
{
    Group group;
    // �������� �����ϴ� ĳ���� asuint
    group.GroupID = asuint(input.GroupID);
    group.GroupThreadID = asuint(input.GroupThreadID);
    group.DispatchThreadID = asuint(input.DispatchThreadID);
    group.GroupIndex = asuint(input.GroupIndex);
    
    // write address
    // groupid * group size+ index
    uint index = input.GroupID.x * 10 * 8 * 3 + input.GroupIndex;
    // 10 = uint3 + uint3 + uint3 + uint + float
    // 4 = �׷쳻������ �ε���
    uint outAddress = index * 11 * 4;
    
    uint inAddress = index * 4;    
    group.RetValue = asfloat(Input.Load(inAddress));
    
    Output.Store3(outAddress + 0, asuint(group.GroupID)); // 12
    Output.Store3(outAddress + 12, asuint(group.GroupThreadID)); // 24
    Output.Store3(outAddress + 24, asuint(group.DispatchThreadID)); // 36
    Output.Store(outAddress + 36, asuint(group.GroupIndex)); // 40
    // float �������� asuint���� ĳ�����ؼ� �������� �մϴ�.
    Output.Store(outAddress + 40, asuint(group.RetValue));

}


technique11 __XB_S_BCNT0_U32
{
    pass P0
    {
        // �⺻ shader�� null�� ����� ��� �Ѵ�.
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        
        SetComputeShader(CompileShader(cs_5_0, CS()));

    }

}