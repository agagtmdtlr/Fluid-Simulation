#define _GLOBAL_SIM_
#define SCAN_THREADS (512)
#define PARTICLE_THREADS (1024)
#define INVALID_KEY (22222222)

static const float epsilon = 1e-6f;
static const float PI = 3.1415926f;

static const float max_acceleration = 100.0f;
static const float max_velocity = 100.0f;

StructuredBuffer<float> lookupTableCubicSplineKernel;
StructuredBuffer<float> lookupTableCubicSplineKernelGradient;

groupshared uint sharedData[1024];

// 데이터 크기를 줄일 수 있으면 좋을거 같다. 32byte로 최적화 가능?
struct Particle
{
    float3 position; // 0
    float3 velocity; // 12
    float density;
    float pressure;
    float state;
};
///////////////////////////////////////////////////////////////
struct ParticleRenderData
{
    float3 position;
    float3 color; // 디버깅 및 다양한 데이터를 저장해 놓을 수 있다.
};

struct ParticleInfo
{
    uint paritlceCount;
    float mass;
    float deltaTime;
    float particleSize;
};
cbuffer cbParticleInfo
{
    ParticleInfo particleInfo;
};
///////////////////////////////////////////////////////////////
struct KernelInfo
{
    float h;
    float idealDensity;
    float pressure_cof;
    float viscosity_cof;
    //
    float cubic_spline_cof;        
    float invLookupOffset;
    float poly6cof;
    float poly6cofGrad;
    //
    float h2;
    float padding[3];
};
cbuffer cbKernelInfo
{
    KernelInfo kernelInfo;
};
///////////////////////////////////////////////////////////////

struct BoundaryInfo
{
    float xmin;
    float xmax;
    float ymin;
    float ymax;
	//
    float zmin;
    float zmax;
	uint boundaryCount;    
    float padding[1];
};
cbuffer cbBoundaryInfo
{
    BoundaryInfo boundaryInfo;
};
///////////////////////////////////////////////////////////////

struct HashGridInfo
{
    uint maxCellCount;
    float cellSize;
    uint maxBoundaryCellCount;
    float padding[1];
    float3 gridResoultion;
};

cbuffer cbHashGridInfo
{
    HashGridInfo hashGridInfo;
};

/////////////////////////////////////////////////////////////////


struct DebugInfo
{
    float debugScalar;
    float3 debugVector;
};

cbuffer cbDebugInfo
{
    DebugInfo debugInfo;
};


void PositionToIJK(in float3 p, out int i , out int j, out int k)
{
    i = int(floor(p.x / hashGridInfo.cellSize));
    j = int(floor(p.y / hashGridInfo.cellSize));
    k = int(floor(p.z / hashGridInfo.cellSize));
}

const uint hashing(in int i,in int j,in int k)
{
    return (i + (j * hashGridInfo.gridResoultion.x) + (k * hashGridInfo.gridResoultion.x * hashGridInfo.gridResoultion.y));
    //return (uint) ((i * 73856093) ^ (j * 19349663) ^ (k * 83492791)) % (hashGridInfo.maxCellCount - 1);
}

const uint hashingBoundary(in int i,in int j,in int k)
{
    return (uint) ((i * 73856093) ^ (j * 19349663) ^ (k * 83492791)) % (hashGridInfo.maxBoundaryCellCount - 1);
}

///////////////////////////////////////////////////////////////

const float cubic_spline_kernel_scalar(in const float r, in const float h)
{
    const float q = r / h;
    return kernelInfo.cubic_spline_cof * (q > 0.5f ? \
    (2.0f * pow(1.0f - q, 3.0f)) : \
    ((6.0f * q * q * (q - 1.0f) + 1.0f)));        
}

const float3 cubic_spline_gradient_kernel_vector(in const float3 rv, in const float h)
{
    const float r = length(rv);    
    const float q = r / h;      
    return rv * kernelInfo.cubic_spline_cof * \
    (\
    (q > 0.5f) ? \
    (-6.0f * pow(1.0f - q, 2.0f)) : \
    (q * (18.0f * q - 12.0f))\
    )\
    / (h * (r + epsilon));
}

const float cubic_spline_kernel_laplacian(in const float r, in const float h)
{
    const float q = r / h;
    return kernelInfo.cubic_spline_cof * \
    ( \
    (q > 0.5f) ? \
    (36.0f * q - 12.0f) : \
    (12.0f * ( 1.0f - q)) \
    ) / ( (r + epsilon));
}

const float viscosity_kernel_scalar(in const float3 pi, in const float3 pj, in const float h)
{
    const float r = length(pi - pj);
    return (r <= h) ? (45.0f * (h - r) / (PI * pow(h, 6.0f))) : 0.0f;
}

///////////////////////////////////////////
// 보류
///////////////////////////////////////////
//float3 surface_tension_gradient_kernel_vector(in float3 pi, in float3 pj, in float h)
//{
//    const float3 rv = pi - pj;
//    const float r = length(rv);
//    
//    if( r > h || r < epsilon)
//    {
//        return float3(0, 0, 0);
//    }
//    else
//    {
//        const float r3 = pow(r, 3);
//        const float h3 = pow(h, 3);
//        const float hr3 = pow(h - r, 3);
//        const float3 a = 136.0241f * -r / (PI * h3 * h3 * h3 * r);
//        return a * ((2.0f * r <= h) ?   (2.0f * hr3 * r3 - 0.0156f * h3 * h3) : (hr3 * r3));
//    }
//}
////////////////////////////////////////////////////////////////////////////
// 표면 장력 테스트
////////////////////////////////////////////////////////////////////////////

const float3 default_Gradient(in const float3 rv, in const float h)
{
    const float r2 = dot(rv, rv);
    return (length(rv) <= h) ? (rv * 9.4f / pow(h, 9.0f) * pow(h * h - r2, 2.0f)) : float3(0, 0, 0);
}

const float default_Laplacian(in const float3 rv, in const float h)
{
    const float r2 = dot(rv, rv);
    const float h2 = h * h;
    return (length(rv) <= h) ? ( 9.4f / pow(h, 9.0f) * (h2 - r2) * (3.0f * h * h - 7.0f * r2) ) : (0);
}



//////////////////////////////////////////////////////////////////
// prefixsum
////////////////////////////////////////////////////////////////////
const uint ScanExclusive(in uint n, in uint lIdx)
{
    uint blocksum = 0;
    uint offset = 1;
    
    uint nActive;
    // UP SWEEP ///////////////////////////////////////////////////
    // n : 1024
    // 1) nActive = 512 (n>>1) , offset = 1
    //      ai = 1 * ( 2 * 0 + 1 ) - 1 = 0
    //      bi = 1 * ( 2 * 0 + 2 ) - 1 = 1;
    // 2) nActive = 256 , offset = 2
    // 3) nActive = 128 , offset = 2;
    for (nActive = n >> 1; nActive > 0; nActive >>= 1, offset <<= 1)
    {
        AllMemoryBarrierWithGroupSync();
        if (lIdx < nActive)
        {
            uint ai = offset * (2 * lIdx + 1) - 1;
            uint bi = offset * (2 * lIdx + 2) - 1;
            sharedData[bi] += sharedData[ai];
        }
    }
    
    AllMemoryBarrierWithGroupSync();

    if (lIdx == 0)
    {
        // blocksum : thread group간의 동기화를 위한 값
        blocksum = sharedData[n - 1]; 
        sharedData[n - 1] = 0;
    }
    
    AllMemoryBarrierWithGroupSync();
    // DOWN SWEEP ///////////////////////////////////////////////////
    offset >>= 1;
    
    for (nActive = 1; nActive < n; nActive <<= 1, offset >>= 1)
    {
        AllMemoryBarrierWithGroupSync();
        if (lIdx < nActive)
        {
            uint ai = offset * (2 * lIdx + 1) - 1;
            uint bi = offset * (2 * lIdx + 2) - 1;
            uint temp = sharedData[ai];
            sharedData[ai] = sharedData[bi];
            sharedData[bi] += temp;
        }
    }

    AllMemoryBarrierWithGroupSync();
    
    return blocksum;
}

void enforceBoundaryPosition(inout Particle p)
{
    // 벽에 충돌 시 에너지 손실
    const float dampWeight = 0.99f;
    if (p.position.x < boundaryInfo.xmin + particleInfo.particleSize)
    {
        p.position.x = boundaryInfo.xmin + epsilon + particleInfo.particleSize;
        p.velocity.x = -p.velocity.x * dampWeight;
    }
    else if (p.position.x > boundaryInfo.xmax - particleInfo.particleSize)
    {
        p.position.x = boundaryInfo.xmax - epsilon - particleInfo.particleSize;
        p.velocity.x = -p.velocity.x * dampWeight;
    }

    if (p.position.y < boundaryInfo.ymin + particleInfo.particleSize)
    {
        p.position.y = boundaryInfo.ymin + epsilon + particleInfo.particleSize;
        p.velocity.y = -p.velocity.y * dampWeight;
    }
    else if (p.position.y > boundaryInfo.ymax - particleInfo.particleSize)
    {
        p.position.y = boundaryInfo.ymax - epsilon - particleInfo.particleSize;
        p.velocity.y = -p.velocity.y * dampWeight;
    }

    if (p.position.z < boundaryInfo.zmin + particleInfo.particleSize)
    {
        p.position.z = boundaryInfo.zmin + particleInfo.particleSize + epsilon;
        p.velocity.z = -p.velocity.z * dampWeight;
    }
    else if (p.position.z > boundaryInfo.zmax - particleInfo.particleSize)
    {
        p.position.z = boundaryInfo.zmax - epsilon - particleInfo.particleSize;
        p.velocity.z = -p.velocity.z * dampWeight;
    }
}

RWStructuredBuffer<Particle> inputParticle;             // unsorted
RWStructuredBuffer<Particle> outputParticle;            // sorted
StructuredBuffer<Particle> inputBoundaryParticle;       // unsorted
RWStructuredBuffer<Particle> outputBoundaryParticle;    // sorted

RWStructuredBuffer<ParticleRenderData> resourceRender;
 
RWStructuredBuffer<float> debugBuffer;

RWStructuredBuffer<uint> sumBuffer;

RWStructuredBuffer<uint> countEachCellIndex;
RWStructuredBuffer<uint> OffsetEachParticle;
RWStructuredBuffer<uint> countEachBoundaryCellIndex;
RWStructuredBuffer<uint> OffsetEachBoundaryParticle;