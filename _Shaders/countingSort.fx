#ifndef _GLOBAL_SIM_
    #include "00_Global_sim.fx"    
#endif


// thread group내에서 thread를 분할
[numthreads(PARTICLE_THREADS, 1, 1)]
void InsertToCounting(uint3 particleId : SV_DispatchThreadID)
{
    
    uint index = particleId.x;
    uint offset;      
    int i, j, k;
    
    
    if (index < particleInfo.paritlceCount)
    {
        PositionToIJK(inputParticle[index].position, i, j, k);
        InterlockedAdd(countEachCellIndex[hashing(i, j, k)], 1, offset);
        OffsetEachParticle[index] = offset;
    }
      
}


[numthreads(SCAN_THREADS, 1, 1)]
void LocalScanKernel(uint3 globalId : SV_DispatchThreadID , uint3 localId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{    
    uint gIdx = globalId.x;
    uint lIdx = localId.x;
    
    sharedData[2 * lIdx] = (2 * gIdx < hashGridInfo.maxCellCount) ? countEachCellIndex[2 * gIdx] : 0;
    sharedData[2 * lIdx + 1] = (2 * gIdx + 1 < hashGridInfo.maxCellCount) ? countEachCellIndex[2 * gIdx + 1] : 0;
    uint sum = ScanExclusive(SCAN_THREADS * 2, lIdx);
    
    if (lIdx == 0)
    {
        sumBuffer[groupId.x] = sum;
    }

    if (2 * gIdx < hashGridInfo.maxCellCount)
    {
        countEachCellIndex[2 * gIdx] = sharedData[2 * lIdx];
    }
    if (2 * gIdx + 1 < hashGridInfo.maxCellCount)
    {
        countEachCellIndex[2 * gIdx + 1] = sharedData[2 * lIdx + 1];
    }
}


[numthreads(SCAN_THREADS, 1, 1)]
void SumBufferScanKernel(uint3 sumBufferId : SV_DispatchThreadID)
{
    uint gIdx = sumBufferId.x;
    
    sharedData[2 * gIdx] = sumBuffer[2 * gIdx];
    sharedData[2 * gIdx + 1] = sumBuffer[2 * gIdx + 1];    
    
    ScanExclusive(SCAN_THREADS * 2, gIdx);        
    
    sumBuffer[2 * gIdx] = sharedData[2 * gIdx];
    sumBuffer[2 * gIdx + 1] = sharedData[2 * gIdx + 1];   
}

[numthreads(1024, 1, 1)]
void ApplySumBufferKernel(uint3 cellId : SV_DispatchThreadID, uint groupId : SV_GroupID)
{

    if (cellId.x < hashGridInfo.maxCellCount)
    {
        countEachCellIndex[cellId.x] += sumBuffer[groupId.x];
    }
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void InsertToOuptut(uint3 particleId : SV_DispatchThreadID, uint groupId : SV_GroupID)
{
    Particle particle;
    uint index = particleId.x;
    uint offset;
    int i, j, k;
    uint key;
    
    
    if (index < particleInfo.paritlceCount)
    {
        particle = inputParticle[index];
        PositionToIJK(particle.position, i, j, k);
        key = hashing(i, j, k);
        outputParticle[countEachCellIndex[key] + OffsetEachParticle[index]] = particle;
    }
}


[numthreads(PARTICLE_THREADS, 1, 1)]
void clearSortData(uint3 globalId : SV_DispatchThreadID)
{
    uint index = globalId.x;
    
    if (index < hashGridInfo.maxCellCount)
    {
        countEachCellIndex[index] = 0;
    }
    
    if (index < particleInfo.paritlceCount)
    {
        OffsetEachParticle[index] = 0;
    }
    
    if (index < 1024)   // sumbuffer;
    {
        sumBuffer[index] = 0;
    }
}
