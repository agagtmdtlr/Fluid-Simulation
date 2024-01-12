#include "00_Global_render.fx"
#include "particleRenderer.fx"
#include "MeshRenderer.fx"

technique11 T0
{
    pass P0_Basic_Splatting
    {
        SetVertexShader(CompileShader(vs_5_0, vsQuad()));
        SetPixelShader(CompileShader(ps_5_0, psQuad()));
    }

    pass P1_Sphere
    {
        SetVertexShader(CompileShader(vs_5_0, vsSphere()));
        SetPixelShader(CompileShader(ps_5_0, psSphere()));
    }

    pass P2_Boundary
    {
        SetVertexShader(CompileShader(vs_5_0, vsQuad()));
        SetPixelShader(CompileShader(ps_5_0, psBoundary()));
    }

    pass P3_BackGround
    {
        SetDepthStencilState(backGroundDSS, 1);
        SetRasterizerState(FrontCounterClockwise_True);        
        SetVertexShader(CompileShader(vs_5_0, vsBackGround()));
        SetPixelShader(CompileShader(ps_5_0, psBackGround()));
    }

    pass P4_Debugs
    {
        SetDepthStencilState(screenDSS, 1);
        SetVertexShader(CompileShader(vs_5_0, vsScreen()));
        SetPixelShader(CompileShader(ps_5_0, psDebug()));
    }

    pass P5_RefractorFrontFace
    {        
        SetVertexShader(CompileShader(vs_5_0, vsQuad()));
        SetPixelShader(CompileShader(ps_5_0, psRefractFrontFace()));
    }

    pass P6_RefractorBackFace
    {
        SetDepthStencilState(backFaceDSS, 1);
        SetVertexShader(CompileShader(vs_5_0, vsQuad()));
        SetPixelShader(CompileShader(ps_5_0, psRefractBackFace()));
    }

    pass P7_Smoothing
    {
        SetDepthStencilState(screenDSS, 1);
        SetVertexShader(CompileShader(vs_5_0, vsScreen()));
        SetPixelShader(CompileShader(ps_5_0, psSmoothing()));
    }

    pass P8_Thickness
    {
        SetBlendState(thicknessBLS, float4(0, 0, 0, 0), 0xFFFFFFFF);
        SetDepthStencilState(thicknessDSS, 1);
        SetVertexShader(CompileShader(vs_5_0, vsQuad()));
        SetPixelShader(CompileShader(ps_5_0, psThichkness()));
    }

    pass P9_ForeGround
    {
        SetDepthStencilState(screenDSS, 1);
        SetVertexShader(CompileShader(vs_5_0, vsScreen()));
        SetPixelShader(CompileShader(ps_5_0, psForeGround()));
    }
    
    pass P10_Plane
    {
        SetVertexShader(CompileShader(vs_5_0, vsPlane()));
        SetPixelShader(CompileShader(ps_5_0, psPlane()));
    }   
}