#ifndef _GLOBAL_SIM_
    #include "00_Global_sim.fx"    
#endif


[numthreads(PARTICLE_THREADS, 1, 1)]
void InsertToCountingBoundary(uint3 particleId : SV_DispatchThreadID)
{
    
    uint index = particleId.x;
    uint offset;
    int i, j, k;   
    
    if (index < boundaryInfo.boundaryCount)
    {
        PositionToIJK(inputBoundaryParticle[index].position, i, j, k);
        InterlockedAdd(countEachBoundaryCellIndex[hashingBoundary(i, j, k)], 1, offset);
        //InterlockedAdd(countEachBoundaryCellIndex[hashing(i, j, k)], 1, offset);
        
        OffsetEachBoundaryParticle[index] = offset;
    }
}

[numthreads(SCAN_THREADS, 1, 1)]
void LocalScanKernelBoundary(uint3 globalId : SV_DispatchThreadID , uint3 localId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{    
    uint gIdx = globalId.x;
    uint lIdx = localId.x;
    
    sharedData[2 * lIdx] = (2 * gIdx < hashGridInfo.maxBoundaryCellCount) ? countEachBoundaryCellIndex[2 * gIdx] : 0;
    sharedData[2 * lIdx + 1] = (2 * gIdx + 1 < hashGridInfo.maxBoundaryCellCount) ? countEachBoundaryCellIndex[2 * gIdx + 1] : 0;
    uint sum = ScanExclusive(SCAN_THREADS * 2, lIdx);
    
    if (lIdx == 0)
    {
        sumBuffer[groupId.x] = sum;
    }

    if (2 * gIdx < hashGridInfo.maxBoundaryCellCount)
    {
        countEachBoundaryCellIndex[2 * gIdx] = sharedData[2 * lIdx];
    }
    if (2 * gIdx + 1 < hashGridInfo.maxBoundaryCellCount)
    {
        countEachBoundaryCellIndex[2 * gIdx + 1] = sharedData[2 * lIdx + 1];
    }
}


[numthreads(SCAN_THREADS, 1, 1)]
void SumBufferScanKernelBoundary(uint3 sumBufferId : SV_DispatchThreadID)
{
    uint gIdx = sumBufferId.x;
    
    sharedData[2 * gIdx] = sumBuffer[2 * gIdx];
    sharedData[2 * gIdx + 1] = sumBuffer[2 * gIdx + 1];    
    
    ScanExclusive(SCAN_THREADS * 2, gIdx);        
    
    sumBuffer[2 * gIdx] = sharedData[2 * gIdx];
    sumBuffer[2 * gIdx + 1] = sharedData[2 * gIdx + 1];   
}

[numthreads(1024, 1, 1)]
void ApplySumBufferKernelBoundary(uint3 cellId : SV_DispatchThreadID, uint groupId : SV_GroupID)
{

    if (cellId.x < hashGridInfo.maxBoundaryCellCount)
    {
        countEachBoundaryCellIndex[cellId.x] += sumBuffer[groupId.x];
    }
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void InsertToOuptutBoundary(uint3 particleId : SV_DispatchThreadID)
{
    Particle particle;
    uint index = particleId.x;
    uint offset;
    int i, j, k;
    uint key;    
    
    if (index < boundaryInfo.boundaryCount)
    {
        particle = inputBoundaryParticle[index];
        PositionToIJK(particle.position, i, j, k);
        key = hashingBoundary(i, j, k);
        
        outputBoundaryParticle[countEachBoundaryCellIndex[key] + OffsetEachBoundaryParticle[index]] = particle;        
    }
}