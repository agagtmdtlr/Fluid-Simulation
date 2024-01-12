#include "Framework.h"
#include "Physics/Data/FluidResourceManager.h"
#include "Systems/GPUTime.h"
#include "Physics/ISPH/SESPHFluidGpu.h"


#define G_PI_CONST (3.1415926535897f)

//#define PARTICLE_LIMIT	(500000)
#define PARTICLE_LIMIT	(1000000)
#define CELL_LIMIT		(PARTICLE_LIMIT * 2)
#define CREATE_WEIGHT	(2.0f)
#define SIMULATION_ITERATION (5)

constexpr float delta_weight = 31.0f;


SESPHFluidGpu::SESPHFluidGpu(Vector3 boundaryVolumeSize, float particleSize, float countForAxis, float kernelCount )
	:
	_boundaryVolumeSize(boundaryVolumeSize),
	_particleSize(particleSize),
	_kernelCount(kernelCount),  // 커널 볼륨내 이웃 파티클 카운트
	_boundaryParticleSorted(false),
	_timers(static_cast<UINT>(eTimerCatergory::categorySize))
{
	if (_fluidVolumeSize > _boundaryVolumeSize) { assert(false); }
	if (_particleSize < Math::EPSILON) { assert(false); }
	if (_kernelCount < 1.0f) { assert(false); }

	_fluidVolumeSize = {
		countForAxis * (_particleSize * CREATE_WEIGHT),
		countForAxis * (_particleSize * CREATE_WEIGHT),
		countForAxis * (_particleSize * CREATE_WEIGHT)
	};

	_boundaryVolumeSize.z = _fluidVolumeSize.z;

	_boundaryInfo.xmin = 0.0f;
	_boundaryInfo.xmax = _boundaryVolumeSize.x;
	_boundaryInfo.ymin = 0.0f;
	_boundaryInfo.ymax = _boundaryVolumeSize.y;
	_boundaryInfo.zmin = 0.0f;
	_boundaryInfo.zmax = _boundaryVolumeSize.z;

	float x = 0, y = 0, z = 0;
	UINT xcount = 0, ycount = 0, zcount = 0;
	for (; x < _fluidVolumeSize.x || y < _fluidVolumeSize.y || z < _fluidVolumeSize.z;)
	{
		if (x < _fluidVolumeSize.x)
		{
			x += _particleSize;
			xcount++;
		}
		if (y < _fluidVolumeSize.y)
		{
			y += _particleSize;
			ycount++;
		}
		if (z < _fluidVolumeSize.z)
		{
			z += _particleSize;
			zcount++;
		}
	}
	if (xcount / 2 * ycount / 2 * zcount / 2 > PARTICLE_LIMIT) { assert(false); }
	
	initFluid();
	initBoundary();
	initialize();
	initializeShader();

}

SESPHFluidGpu::~SESPHFluidGpu()
{
}


void SESPHFluidGpu::initFluid()
{
	_particleInfo.deltaTime = 0.01f;
	// 테스트 용 볼륨 사이즈
	_kernelInfo.idealDensity = 1000.0f;
	_particleInfo.particleSize = _particleSize;
	_particleInfo.paritlceCount = FluidResourceManager::Get()->SetParticleResource(_fluidVolumeSize, _particleSize);
	_fluidParticleCount = _particleInfo.paritlceCount;
	_particleInfo.mass = _kernelInfo.idealDensity * Math::Volume(_fluidVolumeSize) / static_cast<float>(_fluidParticleCount);
	// h , m , particle count and create resource
	float totalVolume = Math::Volume(_fluidVolumeSize);


	_kernelInfo.h = cbrt(
		(3.0f *  totalVolume  * _kernelCount)
		/ (4.0f * G_PI_CONST * (_fluidParticleCount))
	);

	debugoffsetData.resize(_particleInfo.paritlceCount);
	debugNeighborCountData.resize(_particleInfo.paritlceCount);
	/////////////////////////////////////////////////////////////////////////////
	// 해쉬 테이블 생성 초기화
	/////////////////////////////////////////////////////////////////////////////
	_hashGridInfo.cellSize = _kernelInfo.h;
	_hashGridInfo.gridResolution = _boundaryVolumeSize / _hashGridInfo.cellSize;
	_hashGridInfo.maxCellCount = FluidResourceManager::Get()->SetCellResource(_particleInfo.paritlceCount, _boundaryVolumeSize, _hashGridInfo.cellSize);
	// cell count and create resource
	debugCountData.resize(_hashGridInfo.maxCellCount);

	initBoundary();
	_hashGridInfo.maxCellCount = FluidResourceManager::Get()->SetCellResource(_particleInfo.paritlceCount, _boundaryVolumeSize, _hashGridInfo.cellSize);
}

void SESPHFluidGpu::initBoundary()
{
	_boundaryInfo.boundaryCount = FluidResourceManager::Get()->SetBoundaryParticleResource(_boundaryVolumeSize, _particleSize);
	//_hashGridInfo.maxBoundaryCellCount = FluidResourceManager::Get()->SetBoundaryCellResource(_boundaryInfo.boundaryCount);
}

void SESPHFluidGpu::initialize()
{

	const float & h = _kernelInfo.h;
	_kernelInfo.h2 = h * h;
	_kernelInfo.gasStiffness = _kernelInfo.idealDensity * _soundOfSpeed / 7.0f;
	_kernelInfo.viscosity_cof = 0.01f;
	_kernelInfo.coubic_spline_cof = 8.0f / (G_PI_CONST * powf(h, 3.0f));

	_kernelInfo.poly6cof = 315.0f / (64.0f * G_PI_CONST * powf(h, 9.0f));
	_kernelInfo.poly6cofGrad =  945.0f / (32.0f * G_PI_CONST * powf(h, 9.0f));


	// 룩업테이블 보류
	_cubicSplineKernelData.resize(10001);
	_cubicSplineKernelGradinetData.resize(10001);
	float r = 0;
	float offset = h / 10000.0f;
	_kernelInfo.invLookupOffset = 1.0f / offset;

	////////////////////////////////////////////////////////////////////////////////////
	// 현재 GPU 에서 커널을 호출마다 계산하지만
	// 룩업테이블을 이용해 미리 연산된 값을 사용하여 최적화를 기대했지만,
	// 차이가 나타나지 않는다....
	////////////////////////////////////////////////////////////////////////////////////
	for (UINT i = 0; i < 10001; i++)
	{	
		{
			const float q = r / h;

			_cubicSplineKernelData[i] =  _kernelInfo.coubic_spline_cof * (q > 0.5f ? \
				(2.0f * pow(1.0f - q, 3.0f)) : \
				((6.0f * q * q * (q - 1.0f) + 1.0f)));
		}
		{
			const float q = r / h;

			_cubicSplineKernelGradinetData[i] = _kernelInfo.coubic_spline_cof * \
				(\
				(q > 0.5f) ? \
					(-6.0f * pow(1.0f - q, 2.0f)) : \
					(q * (18.0f * q - 12.0f))\
					)\
				/ (h * (r + Math::EPSILON));
		}
		r += offset;
	}
	_lookupTableCubicSplineKernel = make_unique<StructuredBuffer>(&_cubicSplineKernelData[0], sizeof(float), _cubicSplineKernelData.size(), sizeof(float), 1);
	_lookupTableCubicSplineKernelGradient = make_unique<StructuredBuffer>(&_cubicSplineKernelGradinetData[0], sizeof(float), _cubicSplineKernelGradinetData.size(), sizeof(float), 1);
}


void SESPHFluidGpu::initializeShader()
{
	_particleInfoCBuffer	= make_unique<ConstantBuffer>(&_particleInfo, sizeof(cbParticleInfo));
	_kernelInfoCBuffer		= make_unique<ConstantBuffer>(&_kernelInfo, sizeof(cbKernelInfo));
	_boundaryInfoCBuffer	= make_unique<ConstantBuffer>(&_boundaryInfo, sizeof(cbBoundaryInfo));
	_hashGridInfoCBuffer	= make_unique<ConstantBuffer>(&_hashGridInfo, sizeof(cbHashGridInfo));
	_debugInfoCBuffer		= make_unique<ConstantBuffer>(&_debugInfo, sizeof(cbDebugInfo));

	_shader = make_unique<Shader>(L"SEPSPHSimulationSystem.fx");
	
	_shader->AsConstantBuffer("cbParticleInfo")->SetConstantBuffer(_particleInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbKernelInfo")->SetConstantBuffer(_kernelInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbBoundaryInfo")->SetConstantBuffer(_boundaryInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbHashGridInfo")->SetConstantBuffer(_hashGridInfoCBuffer->Buffer());


	_shader->AsUAV("inputParticle")->SetUnorderedAccessView(FluidResourceManager::Get()->GetInputParticleBuffer()->UAV());
	_shader->AsUAV("outputParticle")->SetUnorderedAccessView(FluidResourceManager::Get()->GetOutputParticleBuffer()->UAV());
	_shader->AsUAV("countEachCellIndex")->SetUnorderedAccessView(FluidResourceManager::Get()->GetCellIndexBuffer()->UAV());
	_shader->AsUAV("OffsetEachParticle")->SetUnorderedAccessView(FluidResourceManager::Get()->GetOffsetBuffer()->UAV());
	_shader->AsUAV("sumBuffer")->SetUnorderedAccessView(FluidResourceManager::Get()->GetSumBuffer()->UAV());
	_shader->AsUAV("resourceRender")->SetUnorderedAccessView(FluidResourceManager::Get()->GetRenderResourceBuffer()->UAV());

	//////////////////////////////////////////////
	//lookup table;
	//////////////////////////////////////////////
	_shader->AsSRV("lookupTableCubicSplineKernel")->SetResource(_lookupTableCubicSplineKernel->SRV());
	_shader->AsSRV("lookupTableCubicSplineKernelGradient")->SetResource(_lookupTableCubicSplineKernelGradient->SRV());

	_shader->AsUAV("debugBuffer")->SetUnorderedAccessView(FluidResourceManager::Get()->GetDebugBuffer()->UAV());


	////////////////////////////////////////////////////////////////////
	// 경계 파티클 쉐이더 리소스
	////////////////////////////////////////////////////////////////////
	//_shader->AsSRV("inputBoundaryParticle")->SetResource(FluidResourceManager::Get()->GetBoundaryParticleBuffer()->SRV());
	//_shader->AsUAV("outputBoundaryParticle")->SetUnorderedAccessView(FluidResourceManager::Get()->GetBoundaryParticleBuffer()->UAV());
	//_shader->AsUAV("countEachBoundaryCellIndex")->SetUnorderedAccessView(FluidResourceManager::Get()->GetBoundaryCellIndexBuffer()->UAV());
	//_shader->AsUAV("OffsetEachBoundaryParticle")->SetUnorderedAccessView(FluidResourceManager::Get()->GetBoundaryOffsetBuffer()->UAV());
}

void SESPHFluidGpu::sortBoundary()
{
	FluidResourceManager::Get()->GetBoundaryCellIndexBuffer()->ResetUAV();
	FluidResourceManager::Get()->GetBoundaryOffsetBuffer()->ResetUAV();
	FluidResourceManager::Get()->GetSumBuffer()->ResetUAV();


	UINT pass;
	UINT groupSize;
	UINT particleGroupSize = (UINT)((_boundaryInfo.boundaryCount - 1) / 1024) + 1;
	UINT cellGroupSize = (UINT)((_hashGridInfo.maxBoundaryCellCount - 1) / 1024) + 1;

	// increment counting , offset
	{
		pass = static_cast<UINT>(eDispatchMode::insertToCountingBoundary);
		groupSize = particleGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// local thread group exclusive scan
	{
		pass = static_cast<UINT>(eDispatchMode::localScanBoundary);
		groupSize = cellGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// sum buffer exclusive scan
	{
		pass = static_cast<UINT>(eDispatchMode::sumBufferScanBoundary);
		_shader->Dispatch(
			0,
			pass,
			1, 1, 1);
	}
	// sum buffer 값 cell index 에 적용 시키기
	{
		pass = static_cast<UINT>(eDispatchMode::globaclIncrementBoundary);
		groupSize = cellGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// output 에 정렬시켜 삽입하기
	{
		pass = static_cast<UINT>(eDispatchMode::insertSortedOutputBoundary);
		groupSize = particleGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
}

void SESPHFluidGpu::update()
{
	static float pivot = _particleInfo.deltaTime;
	static float weight = delta_weight;
	ImGui::SliderFloat("delta Time", &weight, 1.0f, 100.0f);
	_particleInfo.deltaTime = pivot * (weight / 100.0f);	
	ImGui::SliderFloat("Debug Scalar", &_debugInfo.debugScalar, 0.1f, 10.0f);

	_kernelInfo.gasStiffness = _kernelInfo.idealDensity * _soundOfSpeed / 7.0f;

	UINT pass = 0;
	UINT groupSize = 0;
	UINT particleGroupSize = (UINT)((_particleInfo.paritlceCount - 1) / 1024) + 1;
	UINT cellGroupSize = (UINT)((_hashGridInfo.maxCellCount - 1) / 1024) + 1;

	_particleInfoCBuffer->Render();
	_kernelInfoCBuffer->Render();
	_boundaryInfoCBuffer->Render();
	_hashGridInfoCBuffer->Render();
	_debugInfoCBuffer->Render();

	_shader->AsConstantBuffer("cbParticleInfo")->SetConstantBuffer(_particleInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbKernelInfo")->SetConstantBuffer(_kernelInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbBoundaryInfo")->SetConstantBuffer(_boundaryInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbHashGridInfo")->SetConstantBuffer(_hashGridInfoCBuffer->Buffer());
	_shader->AsConstantBuffer("cbDebugInfo")->SetConstantBuffer(_debugInfoCBuffer->Buffer());


	if (_boundaryParticleSorted == false)
	{
		//sortBoundary();
		_boundaryParticleSorted = true;
	}

	if (isTimerInit == false)
	{
		UINT index = GPUTime::Get()->CreateQuery();
		_timers[static_cast<UINT>(eTimerCatergory::neighborSearchEnd)] = index;
		index = GPUTime::Get()->CreateQuery();
		_timers[static_cast<UINT>(eTimerCatergory::simulationEnd)] = index;
		isTimerInit = true;
	}

	static float wavePivot = _boundaryInfo.xmin;
	static float waveTime = 0.0f;
	static float waveRange = 2.0f;

	UINT simulation_iteration = 0;
	GPUTime::Get()->RequestTimer(_timers[static_cast<UINT>(eTimerCatergory::neighborSearchEnd)]);
	while (simulation_iteration < SIMULATION_ITERATION)
	{
		waveTime += _particleInfo.deltaTime;
		_boundaryInfo.xmin = wavePivot + abs(sinf(waveTime)) * waveRange;
		_boundaryInfoCBuffer->Render();

		updateHashGrid();
		updateSimulation();
		simulation_iteration++;
	}
	GPUTime::Get()->RequestTimer(_timers[static_cast<UINT>(eTimerCatergory::simulationEnd)]);

	updateRenderResource();
}

void SESPHFluidGpu::updateHashGrid()
{
	// Reset Data to sort;
	FluidResourceManager::Get()->GetCellIndexBuffer()->ResetUAV();
	FluidResourceManager::Get()->GetOffsetBuffer()->ResetUAV();
	FluidResourceManager::Get()->GetSumBuffer()->ResetUAV();

	UINT pass;
	UINT groupSize;
	UINT particleGroupSize = static_cast<UINT>((_particleInfo.paritlceCount - 1 ) / 1024) + 1;
	UINT cellGroupSize = static_cast<UINT>((_hashGridInfo.maxCellCount - 1) / 1024) + 1;

	// increment counting , offset
	{
		pass = static_cast<UINT>(eDispatchMode::insertCount);
		groupSize = particleGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// local scan
	{
		pass = static_cast<UINT>(eDispatchMode::localScan);
		groupSize = cellGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// sum buffer scan
	{
		pass = static_cast<UINT>(eDispatchMode::sumBufferScan);
		_shader->Dispatch(
			0,
			pass,
			1, 1, 1); // 
	}
	// global Increment
	{
		// 
		
		pass = static_cast<UINT>(eDispatchMode::globaclIncrement);
		groupSize = cellGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// insert SortedOuptut
	{
		pass = static_cast<UINT>(eDispatchMode::insertSortedOutput);
		groupSize = particleGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}


}

void SESPHFluidGpu::updateSimulation()
{
	UINT pass;
	UINT groupSize;
	UINT particleGroupSize = static_cast<UINT>((_particleInfo.paritlceCount - 1 ) / 1024) + 1;
	UINT cellGroupSize = static_cast<UINT>((_hashGridInfo.maxCellCount - 1 ) / 1024) + 1;
	UINT simulateGroupSize = static_cast<UINT>((_particleInfo.paritlceCount - 1) / 37) + 1;
	// compute Density And Pressure
	{
		pass = static_cast<UINT>(eDispatchMode::computeDensityAndPressure);
		groupSize = particleGroupSize;
		//groupSize = simulateGroupSize;

		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
	// compute Acceleration + Velocity + Position
	{
		pass = static_cast<UINT>(eDispatchMode::computeAcceleration);
		groupSize = particleGroupSize;
		//groupSize = simulateGroupSize;

		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}

}

void SESPHFluidGpu::updateRenderResource()
{
	UINT pass;
	UINT groupSize;
	UINT particleGroupSize = static_cast<UINT>((_particleInfo.paritlceCount - 1) / 1024) + 1;

	// compute copy to render resource
	{
		pass = static_cast<UINT>(eDispatchMode::copyToRenderResource);
		groupSize = particleGroupSize;
		_shader->Dispatch(
			0,
			pass,
			groupSize, 1, 1);
	}
}


void SESPHFluidGpu::Render()
{
	RenderState();
}

void SESPHFluidGpu::RenderState()
{
	ImGui::Text("particle count: %d", _particleInfo.paritlceCount);
	ImGui::Text("cell max count: %d" , _hashGridInfo.maxCellCount);

	ImGui::Text("boundary count: %d", _boundaryInfo.boundaryCount);

	ImGui::Text("support radius : %f", _kernelInfo.h);
	ImGui::Text("particle size : %f", _particleSize);
	ImGui::Text("particle mass : %f", _particleInfo.mass);

	ImGui::Text("simulation delta time : %.1f ms", _particleInfo.deltaTime * 1000.0f);
	ImGui::Text("rendering delta time : %.2f ms", _particleInfo.deltaTime * 1000.0f * float(SIMULATION_ITERATION));

	float nns = GPUTime::Get()->GetElapse(_timers[static_cast<UINT>(eTimerCatergory::neighborSearchEnd)]);
	float sim = GPUTime::Get()->GetElapse(_timers[static_cast<UINT>(eTimerCatergory::simulationEnd)]);
	float next = GPUTime::Get()->GetElapse(_timers[static_cast<UINT>(eTimerCatergory::simulationEnd)] + 2);

	ImGui::Text("simulation : %.4f ms", sim - nns);
	ImGui::Text("simulation + rendering : %.4f ms", next - nns);

	ImGui::Text("frame Rate : %.4f fps", ImGui::GetIO().Framerate);
	ImGui::Text("relative performance : %.4f fps", 
		(1000 * _particleInfo.deltaTime * float(SIMULATION_ITERATION) /(next - nns) * ImGui::GetIO().Framerate)) ;


	ImGui::SliderFloat("sound of speed : %.4f", &_soundOfSpeed, 1.0f, 100.0f);
	ImGui::SliderFloat("viscosity coefficient : %.0f", &_kernelInfo.viscosity_cof, 0.01f,100.0f);
	ImGui::Text("pressure coefficient : %.0f", _kernelInfo.gasStiffness);
}

