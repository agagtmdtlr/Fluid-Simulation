#ifndef _GLOBAL_SIM_
    #include "00_Global_sim.fx"    
#endif

#define NNS_ITER (27)

static const float p_cof = 1119.0f;
static const float v_cof = 0.01f;

float3 searchDir[8] = { 
    float3(0,0,0),
    float3(1,0,0),
    float3(0,1,0),
    float3(1,1,0),
    float3(0,0,1),
    float3(1,0,1),
    float3(0,1,1),
    float3(1,1,1)
};

void NeighborSearch(in const float3 pos, out uint nns[NNS_ITER])
{
    int dx, dy, dz;
    int i, j, k;
    PositionToIJK(pos, i, j, k);    
    [unroll(NNS_ITER)]
    for (int cur = 0; cur < NNS_ITER; cur += 1)
    {   
        dx = (cur / 9 - 1);
        dy = ((cur % 9) / 3 - 1);
        dz = (cur % 3 - 1);
        nns[cur] = hashing(i + dx, j + dy, k + dz);                            
    }    
}

// thread group내에서 thread를 분할
[numthreads(PARTICLE_THREADS, 1, 1)]
void computeDensityAndPressure(uint3 particleIdx : SV_DispatchThreadID)
{
    uint index = particleIdx.x;
    
    [flatten]
    if (index >= particleInfo.paritlceCount)
    {
        return;
    }    
    Particle pi;
    Particle pj;
    pi = outputParticle[index];
    //[flatten]    
    //if (pi.state == 0)
    //{
    //    return;
    //}
    uint key;
    uint cur;
    uint end;
   
    float3 rv;
    float r;
    pi.density = 0;
    int i, j, k;
    int dx, dy, dz;
    PositionToIJK(pi.position , i, j, k);
    
    [unroll(NNS_ITER)]        
    for (uint it = 0; it < NNS_ITER; it++)
    {            
        dx = i +(it / 9 - 1);
        dy = j +((it % 9) / 3 - 1);
        dz = k +(it % 3 - 1);
        if (dx >= 0 && dy >= 0 && dz >= 0)
        {
            key = hashing(dx, dy, dz);
            for (cur = countEachCellIndex[key], end = countEachCellIndex[key + 1]; cur < end; cur++)
            {
                pj = outputParticle[cur];
                r = length(pi.position - pj.position);
            [flatten]
                if (r <= kernelInfo.h)
                {
                    pi.density += cubic_spline_kernel_scalar(r, kernelInfo.h);
                }
            } // for pj         
        }
    } // for cell
    pi.density *= particleInfo.mass;
    pi.pressure = max(kernelInfo.pressure_cof * (pow(pi.density / kernelInfo.idealDensity, 7.0f) - 1.0f), 0);
    outputParticle[index] = pi;
}


[numthreads(PARTICLE_THREADS, 1, 1)]
void computeAcceleration(uint3 particleIdx : SV_DispatchThreadID)
{
    
    uint index = particleIdx.x;

    [flatten]
    if (index >= particleInfo.paritlceCount)
    {
        return;
    }
    Particle pi, pj;
    pi = outputParticle[index];
    
    uint key;
    uint cur;
    uint end;
    
    float3 pforce = float3(0, 0, 0);
    float3 vforce = float3(0, 0, 0);
    //float r;
    float r2;
    float dsq;
    float3 rv;
    float3 acceleration = float3(0, 0, 0);             
    int i, j, k;
    int dx, dy, dz;
    PositionToIJK(pi.position, i, j, k);
    float pterm;
    pterm = (pi.pressure / max(epsilon, pi.density * pi.density));
        
    [unroll(NNS_ITER)]
    for (uint it = 0; it < NNS_ITER; it++)
    {
        dx = i + (it / 9 - 1);
        dy = j + ((it % 9) / 3 - 1);
        dz = k + (it % 3 - 1);
        
        if(dx >=0 && dy >=0 && dz >= 0)
        {
            key = hashing(dx, dy, dz);
            for (cur = countEachCellIndex[key], end = countEachCellIndex[key + 1]; cur < end; cur++)
            {
                pj = outputParticle[cur];
                rv = pi.position - pj.position;
                r2 = dot(rv, rv);
                [flatten]
                if (r2 <= kernelInfo.h2)
                {
                    pforce += (pterm + pj.state * (pj.pressure / max(epsilon, pj.density * pj.density))) * cubic_spline_gradient_kernel_vector(pi.position - pj.position, kernelInfo.h);
                    vforce += (pj.state ? viscosity_kernel_scalar(pi.position, pj.position, kernelInfo.h) * (pj.velocity - pi.velocity) : 0);
                }
            } // for pj        
        }
        
    } // for cell        
        
    acceleration -= pforce;
    acceleration += kernelInfo.viscosity_cof * vforce / max(kernelInfo.idealDensity, pi.density);
    acceleration *= particleInfo.mass;
    acceleration += float3(0.0f, -9.82f, 0.0f); 
       
    float accMag = length(acceleration);
    [flatten]
    if (accMag > max_acceleration)
    {
        acceleration *= max_acceleration / accMag;
    }
    pi.velocity += particleInfo.deltaTime * acceleration;        
    float velMag = length(pi.velocity);
    [flatten]        
    if (velMag > max_velocity)
    {
        pi.velocity *= max_velocity / velMag;
    }
    pi.position += particleInfo.deltaTime * pi.velocity;
    enforceBoundaryPosition(pi);
    inputParticle[index] = pi;
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void computeVelocityAndPosition(uint3 particleId : SV_DispatchThreadID, uint3 localId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void copyToRenderResource(uint3 particleIdx : SV_DispatchThreadID)
{   
    uint index = particleIdx.x;
    
    if (index < particleInfo.paritlceCount)
    {
        Particle p = inputParticle[index];
        ParticleRenderData renderdata;
        renderdata.position = p.position;
        renderdata.color = p.velocity;
        
        resourceRender[index] = renderdata;
        
    }    
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void copyToInputFromOutput(uint3 particleIdx : SV_DispatchThreadID)
{
    uint index = particleIdx.x;
    
    if (index < particleInfo.paritlceCount)
    {
        inputParticle[index] = outputParticle[index];
    }
}

[numthreads(PARTICLE_THREADS, 1, 1)]
void copyToOutputFromInput(uint3 particleIdx : SV_DispatchThreadID)
{
    uint index = particleIdx.x;
    
    if (index < particleInfo.paritlceCount)
    {
        outputParticle[index] = inputParticle[index];
    }
}



[numthreads(PARTICLE_THREADS, 1, 1)]
void computeForceAndUpdatePosition(uint3 particleIdx : SV_DispatchThreadID)
{
 
}