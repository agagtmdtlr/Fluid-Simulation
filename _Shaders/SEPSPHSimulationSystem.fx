#include "00_Global_sim.fx"    
#include "countingSort.fx"
#include "countingSort_Boundary.fx"
#include "simulation.fx"
//#include "home_simulation.fx"


technique11 __XB_S_BCNT0_U32
{
	pass P0_insertCount
	{
		SetVertexShader(NULL);
		SetPixelShader(NULL);

		SetComputeShader(CompileShader(cs_5_0, InsertToCounting()));
	}
	pass P1_LocalScan
	{
		SetVertexShader(NULL);
		SetPixelShader(NULL);

		SetComputeShader(CompileShader(cs_5_0, LocalScanKernel()));
	}
    pass P2_sumBufferScan
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, SumBufferScanKernel()));
    }
    pass P3_globalIncrement
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, ApplySumBufferKernel()));
    }
    pass P4_insertSortedOutput
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, InsertToOuptut()));
    }
    pass P5_computeDensityAndPressure
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, computeDensityAndPressure()));
    }
    pass P6_computeAcceleration
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, computeAcceleration()));
    }
    pass P7_comptueVelocityAndPosition
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, computeVelocityAndPosition()));
    }
    pass P8_copyToRenderResource
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, copyToRenderResource()));
    }
    pass P9_copyToInputFromOutput
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, copyToInputFromOutput()));
    }
    pass P10_copyToOutputFromInput
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, copyToOutputFromInput()));
    }
    pass P11_clearSortData
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, clearSortData()));
    }
///////////////////////////////////////////////////////////////////////////
// 현재 테스트 용으로 만든거임
///////////////////////////////////////////////////////////////////////////
    pass P12_computeForceAndUpdatePosition
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, computeForceAndUpdatePosition()));
    }

///////////////////////////////////////////////////////////////////////////
// Boundary sort
///////////////////////////////////////////////////////////////////////////
    pass P13_InsertToCountingBoundary
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, InsertToCountingBoundary()));
    }
    pass P14_LocalScanKernelBoundary
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, LocalScanKernelBoundary()));
    }
    pass P15_SumBufferScanKernelBoundary
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, SumBufferScanKernelBoundary()));
    }
    pass P16_ApplySumBufferKernelBoundary
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, ApplySumBufferKernelBoundary()));
    }
    pass P17_InsertToOuptutBoundary
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, InsertToOuptutBoundary()));
    }

}