#pragma once
class ParticleRenderer
{
public: enum class eRenderingMode
{
	fluidMode = 0,
	sphereMode = fluidMode + 1, // 테스트용
	boundaryMode = sphereMode + 1
};
private: enum class eDispatch
{
	quad = 0,
	sphere = quad + 1,
	boundaryQuad = sphere + 1,
	backGround = boundaryQuad + 1,
	debug = backGround + 1,
	frontFace = debug + 1,
	backFace = frontFace + 1,
	smoothing = backFace + 1,
	thickness = smoothing + 1,
	foreGround = thickness + 1,
	plane = foreGround + 1
};
private: eRenderingMode _mode = eRenderingMode::fluidMode;

public: struct ParticleVertexType
{
	Vector3 position;
	Vector2 uv;
}; 

public:	ParticleRenderer(UINT instanceCount, float radius, eRenderingMode mode, Vector3 boundary);
public: ~ParticleRenderer();

public:	void copyToBuffer(const vector<ParticleRenderData> & data);
public:	void copyToBuffer(const ParticleRenderData * data);
public: void copyToBufferFromResourceManager();

public: void Update();
public: void Render();
private: void RenderRefractor();
private: void RenderThickness();
private: void RenderBackGround();
private: void RenderForeGround();
private: void RenderDebug();
private: void RenderSmoothing();

private: void Create();
private: void CreateQuad();
private: void CreateCube();
private: void CreatePlane();
private: void CreateView();
private: void CreateState();


private:
	struct cbInfo_Render
	{
		Matrix view;//
		Matrix projection;//
		Vector3 boundary;
		float radius;//
		Matrix invView;//
		Matrix invVP;//
		float farDepth;
		float nearDepth;
		float width;
		float height;//
		float texel_deviation = 0.01f;
		float depth_deviation = 0.00061f;
		float filterRadius = 7.0f;
		float transparency = 1.0f;//
		Vector3 diffuseColor = { 1, 0, 0 };
		float distance_range = 1.0f;//
		Matrix shadowView;//
		Matrix shadowProjection;//
		Matrix invShadowView;//
	};
	
private: Shader* _shader;

private: UINT _vertexCount;
private: ParticleVertexType * _vertices;
private: unique_ptr<VertexBuffer> _vertexBuffer;

private: UINT _indexCount;
private: UINT * _indices;
private: unique_ptr<IndexBuffer> _indexBuffer;

private: UINT _instanceCount;
private: unique_ptr<VertexBuffer> _instanceBuffer;

private: cbInfo_Render _info;
private: unique_ptr<ConstantBuffer> _constantBuffer;
	
private: float _radius = 0.5f;
private: UINT _stackCount = 20;
private: UINT _sliceCount = 20;


private: enum class eTimerCatergory : UINT
{
	renderingBegin = 0,
	renderingEnd = renderingBegin + 1,
	categorySize = renderingEnd + 1
};
private: bool isTimerInit = false;
private: vector<UINT> _timers;

////////////////////////////////////////////////////////////////////////////
// 멀티 패스 렌더링 
////////////////////////////////////////////////////////////////////////////

private: struct RenderResource
{
	unique_ptr<RenderTarget> rtv;
	unique_ptr<DepthStencil> dsv;
};

private: struct RenderState
{
	ComPtr<ID3D11DepthStencilState> dss;
	ComPtr<ID3D11BlendState> bls;
	ComPtr<ID3D11RasterizerState> rss;
};

private: unique_ptr<Texture>		_cubeMap;
	
private: RenderResource				_rrDebug;
private: RenderState				_rsDebug;

private: unique_ptr<VertexBuffer>	_bgVertexBuffer;
private: unique_ptr<IndexBuffer>	_bgIndexBuffer;
private: UINT						_bgVertexCount;
private: UINT						_bgIndexCount;
private: ParticleVertexType *		_bgVertices;
private: UINT *						_bgIndices;


private: RenderResource				_rrCommon;

private: RenderResource				_rrThickness;
private: RenderState				_rsThickness;
private: unique_ptr<Viewport>       _viewport;
private: unique_ptr<Viewport>       _viewportReset;


private: RenderState				_rsForeGround;

private: RenderResource				_rrBackGround;
private: RenderState				_rsBackGround;

/////////////////////////////////////////////////////////////
// refraction 을 위한 자원
////////////////////////////////////////////////////////////
private: RenderResource				_rrRefractorFrontFace;
private: RenderResource				_rrRefractorBackFace;
private: RenderResource				_rrRefractorSmooth;

private: RenderState				_rsRefractor;



////////////////////////////////////////////////////////////
// plane render resource
////////////////////////////////////////////////////////////


private: unique_ptr<class MeshRender> _planeRenderer;
private: RenderState				_rsPlane;
private: unique_ptr<Texture>		_normalMap;
private: unique_ptr<Texture>		_albedoMap;

////////////////////////////////////////////////////////////
// shadow map resource
////////////////////////////////////////////////////////////
private: RenderResource				_rrShadow;
private: RenderState				_rsShadow;


};
