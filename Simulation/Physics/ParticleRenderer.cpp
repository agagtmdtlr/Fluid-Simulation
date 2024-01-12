#include "Framework.h"
#include "Systems/GPUTime.h"
#include "Meshes/MeshGrid.h"
#include "Meshes/MeshRender.h"
#include "Physics/SPHcommon.h"
#include "Physics/ParticleRenderer.h"
#include "Physics/Data/FluidResourceManager.h"

constexpr float relative_scale = 3.0f;
constexpr float plane_scale = 2.0f;

ParticleRenderer::ParticleRenderer(UINT instanceCount, float radius, eRenderingMode mode, Vector3 boundary)
	:_instanceCount(instanceCount),
	_radius(radius * 2.0f),
	_timers((UINT)eTimerCatergory::categorySize),
	_mode(mode)
{
	_info.boundary = boundary * relative_scale;

	//Create();
	CreateQuad();
	CreateCube();
	CreateView();
	CreateState();

	_instanceBuffer = make_unique<VertexBuffer>(nullptr, _instanceCount, sizeof(ParticleRenderData), 1, true); // particle instance position;
	_vertexBuffer = make_unique<VertexBuffer>(_vertices, _vertexCount, sizeof(ParticleVertexType), 0, true);	// particle sphere
	_indexBuffer = make_unique<IndexBuffer>(_indices, _indexCount);									// sphere index;

	_bgVertexBuffer = make_unique<VertexBuffer>(_bgVertices, _bgVertexCount, sizeof(ParticleVertexType), 0, true);
	_bgIndexBuffer = make_unique<IndexBuffer>(_bgIndices, _bgIndexCount);									
	_constantBuffer = make_unique<ConstantBuffer>(&_info, sizeof(cbInfo_Render));
	_shader = new Shader(L"rendererSystem.fx");


	CreatePlane();
}

ParticleRenderer::~ParticleRenderer()
{
	SafeDelete(_shader);
	SafeDeleteArray(_vertices);
	SafeDeleteArray(_indices);
}

void ParticleRenderer::copyToBuffer(const vector<ParticleRenderData>& data)
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(_instanceBuffer->Buffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, &data[0], _instanceCount * sizeof(ParticleRenderData));
	}
	D3D::GetDC()->Unmap(_instanceBuffer->Buffer(), 0);
}

void ParticleRenderer::copyToBuffer(const ParticleRenderData * data)
{
}

void ParticleRenderer::copyToBufferFromResourceManager()
{
	if (_mode == eRenderingMode::fluidMode)
	{
		ID3D11Resource* srcResource = FluidResourceManager::Get()->GetRenderResourceBuffer()->GetOutput();

		D3D::GetDC()->CopyResource(_instanceBuffer->Buffer(),
			srcResource
		);
	}
	else
	{

	}
	
}

void ParticleRenderer::Update()
{
	if (isTimerInit == false)
	{
		UINT index = GPUTime::Get()->CreateQuery();
		_timers[static_cast<UINT>(eTimerCatergory::renderingBegin)] = index;
		index = GPUTime::Get()->CreateQuery();
		_timers[static_cast<UINT>(eTimerCatergory::renderingEnd)] = index;
		isTimerInit = true;
	}

	ImGui::Begin("Rendering Particle");
	{
		Context::Get()->GetCamera()->GetMatrix(&_info.view);
		Context::Get()->GetPerspective()->GetMatrix(&_info.projection);
		D3DXMatrixInverse(&_info.invView, NULL, &_info.view);
		_info.radius = _radius;
		_info.farDepth = Context::Get()->GetPerspective()->GetFarClip();
		_info.nearDepth = Context::Get()->GetPerspective()->GetNearClip();
		_info.width = D3D::Width();
		_info.height = D3D::Height();

		ImGui::SliderFloat("texel deviation", &_info.texel_deviation, 0.001f, 10.0f);
		ImGui::SliderFloat("depth deviation", &_info.depth_deviation, 0.0001f, 0.1f, "%.5f");
		ImGui::SliderFloat("filter Radius", &_info.filterRadius, 5.0f, 15.0f);
		ImGui::SliderFloat("transparency", &_info.transparency, 0.0f, 1.0f);
		ImGui::SliderFloat("distance range", &_info.distance_range, 0.0f, 1.0f);
		ImGui::ColorEdit3("diffuse Color", (float*)&_info.diffuseColor);
		
		D3DXMatrixMultiply(&_info.invVP, &_info.view, &_info.projection);
		D3DXMatrixInverse(&_info.invVP, NULL, &_info.invVP);

		_planeRenderer->Update();

		float begin = GPUTime::Get()->GetElapse(_timers[static_cast<UINT>(eTimerCatergory::renderingBegin)]);
		float end = GPUTime::Get()->GetElapse(_timers[static_cast<UINT>(eTimerCatergory::renderingEnd)]);;
		ImGui::Text("particle rendering : %.2f ms", end - begin);
	}
	ImGui::End();
}

void ParticleRenderer::Render()
{
	GPUTime::Get()->RequestTimer(_timers[static_cast<UINT>(eTimerCatergory::renderingBegin)]);
	{
		_constantBuffer->Render();
		_shader->AsConstantBuffer("cbInfo_Render")->SetConstantBuffer(_constantBuffer->Buffer());
		
		//_vertexBuffer->Render();
		//_indexBuffer->Render();
		//_instanceBuffer->Render();
		//
		//D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//// draw front face normal and depth
		//{
		//	UINT pass = static_cast<UINT>(eDispatch::quad);
		//	_shader->DrawIndexedInstanced(0, pass, _indexCount, _instanceCount, 0, 0, 0);
		//}
		//return;

		if (_mode == eRenderingMode::fluidMode)
		{
			RenderBackGround();
		}

		_vertexBuffer->Render();
		_indexBuffer->Render();
		_instanceBuffer->Render();

		if (_mode == eRenderingMode::fluidMode)
		{
			RenderRefractor();		
			RenderThickness();
			RenderSmoothing();
			//RenderDebug();
			RenderForeGround();
		}
		else if (_mode == eRenderingMode::boundaryMode)
		{

		}
	}
	

	GPUTime::Get()->RequestTimer(_timers[static_cast<UINT>(eTimerCatergory::renderingEnd)]);

	
}

void ParticleRenderer::RenderRefractor()
{
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// draw front face normal and depth
	{
		ID3D11RenderTargetView* rtv = _rrRefractorFrontFace.rtv->RTV();
		ID3D11DepthStencilView* dsv = _rrCommon.dsv->DSV();
		FLOAT clearColor[4] = { _info.farDepth ,0 ,0, 0 };

		D3D::GetDC()->ClearRenderTargetView(rtv, clearColor);
		D3D::GetDC()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1,0);
		D3D::GetDC()->OMSetRenderTargets(1, &rtv, dsv);
		
		UINT frontFacePass = static_cast<UINT>(eDispatch::frontFace);
		_shader->DrawIndexedInstanced(0, frontFacePass, _indexCount, _instanceCount, 0, 0, 0);
	}

	// draw back face normal and depth
	{
		ID3D11RenderTargetView* rtv = _rrRefractorBackFace.rtv->RTV();
		ID3D11DepthStencilView* dsv = _rrCommon.dsv->DSV();
		FLOAT clearColor[4] = { _info.nearDepth ,0 ,0, 0 };

		D3D::GetDC()->ClearRenderTargetView(rtv, clearColor);
		D3D::GetDC()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 0, 0);
		D3D::GetDC()->OMSetRenderTargets(1, &rtv, dsv);

		_shader->AsDepthStencil("backFaceDSS")->SetDepthStencilState(0,_rsRefractor.dss.Get());

		UINT backFacePass = static_cast<UINT>(eDispatch::backFace);
		_shader->DrawIndexedInstanced(0, backFacePass, _indexCount, _instanceCount, 0, 0, 0);
	}
	_shader->AsSRV("frontTargetTexture")->SetResource(_rrRefractorFrontFace.rtv->SRV());
	_shader->AsSRV("backTargetTexture")->SetResource(_rrRefractorBackFace.rtv->SRV());
}

void ParticleRenderer::RenderThickness()
{
	// draw front face normal and depth
	{
		ID3D11RenderTargetView* rtv = _rrThickness.rtv->RTV();

		FLOAT clearColor[4] = { 0,0,0,0 };
		D3D::GetDC()->ClearRenderTargetView(rtv, clearColor);
		D3D::GetDC()->OMSetRenderTargets(1, &rtv, nullptr);

		_shader->AsBlend("thicknessBLS")->SetBlendState(0, _rsThickness.bls.Get());
		_shader->AsDepthStencil("thicknessDSS")->SetDepthStencilState(0, _rsThickness.dss.Get());
		_viewport->RSSetViewport();
		UINT thicknessPass = static_cast<UINT>(eDispatch::thickness);
		if (thicknessPass == UINT_MAX)
		{
			assert(false);
		}
		_shader->DrawIndexedInstanced(0, thicknessPass, _indexCount, _instanceCount, 0, 0, 0);
		_viewportReset->RSSetViewport();

	}
	_shader->AsSRV("thicknessTexture")->SetResource(_rrThickness.rtv->SRV());
}

void ParticleRenderer::RenderBackGround()
{

	_shader->AsSRV("backGroundCube")->SetResource(_cubeMap->SRV());

	_bgVertexBuffer->Render();
	_bgIndexBuffer->Render();

	ID3D11RenderTargetView* rtv = _rrBackGround.rtv->RTV();
	ID3D11DepthStencilView* dsv = _rrCommon.dsv->DSV();
	
	D3D::GetDC()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
	D3D::Get()->SetRenderTarget(rtv,dsv);

	_shader->AsDepthStencil("backGroundDSS")->SetDepthStencilState(0, _rsBackGround.dss.Get());
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	{
		_shader->DrawIndexed(0, (UINT)eDispatch::backGround, _bgIndexCount);
	}
	//_planeRenderer->Render();
	_shader->AsSRV("backGroundTexture")->SetResource(_rrBackGround.rtv->SRV());

}

void ParticleRenderer::RenderForeGround()
{
	_shader->AsDepthStencil("screenDSS")->SetDepthStencilState(0, _rsDebug.dss.Get());
	D3D::Get()->SetRenderTarget();
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_shader->Draw(0, (UINT)eDispatch::foreGround, 4, 0);
	
}

void ParticleRenderer::RenderDebug()
{
	_shader->AsSRV("debugTexture")->SetResource(_rrRefractorFrontFace.rtv->SRV());
	_shader->AsSRV("debugCube")->SetResource(_cubeMap->SRV());
	_shader->AsDepthStencil("screenDSS")->SetDepthStencilState(0, _rsDebug.dss.Get());

	D3D::Get()->SetRenderTarget();
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_shader->Draw(0, (UINT)eDispatch::debug, 4, 0);
}

void ParticleRenderer::RenderSmoothing()
{
	_shader->AsDepthStencil("screenDSS")->SetDepthStencilState(0, _rsDebug.dss.Get());

	_rrRefractorSmooth.rtv->ClearRTV();
	ID3D11RenderTargetView* rtv	= _rrRefractorSmooth.rtv->RTV();
	ID3D11DepthStencilView* dsv = _rrCommon.dsv->DSV();
	D3D::GetDC()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
	D3D::GetDC()->OMSetRenderTargets(1, &rtv, dsv);

	UINT pass = (UINT)eDispatch::smoothing;
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	{
		_shader->Draw(0, pass, 4);
	}
	_shader->AsSRV("smoothTexture")->SetResource(_rrRefractorSmooth.rtv->SRV());

}

void ParticleRenderer::CreateView()
{
	_cubeMap = make_unique<Texture>(L"stonewall.dds");

	{
		_rrCommon.dsv = make_unique<DepthStencil>(D3D::Width(), D3D::Height());
	}
	{
		_rrBackGround.rtv = make_unique<RenderTarget>(D3D::Width(), D3D::Height());
	}
	{
		_rrThickness.rtv = make_unique<RenderTarget>(D3D::Width() / 2.0f , D3D::Height() / 2.0f, DXGI_FORMAT_R32_FLOAT);
		_viewport = make_unique<Viewport>(D3D::Width() / 2.0f, D3D::Height() / 2.0f );
		_viewportReset = make_unique<Viewport>(D3D::Width() , D3D::Height() );

	}
	{
		_rrRefractorFrontFace.rtv = make_unique<RenderTarget>(D3D::Width(), D3D::Height(), DXGI_FORMAT_R32_FLOAT);
		_rrRefractorBackFace.rtv = make_unique<RenderTarget>(D3D::Width(), D3D::Height(), DXGI_FORMAT_R32_FLOAT);
	}
	{
		_rrRefractorSmooth.rtv = make_unique<RenderTarget>(D3D::Width(), D3D::Height(), DXGI_FORMAT_R32G32_FLOAT);
	}
}

void ParticleRenderer::CreateState()
{
	{
		D3D11_BLEND_DESC desc;
		D3D11_RENDER_TARGET_BLEND_DESC rtvdesc;
		ZeroMemory(&desc, sizeof(D3D11_BLEND_DESC));
		ZeroMemory(&rtvdesc, sizeof(D3D11_RENDER_TARGET_BLEND_DESC));

		desc.AlphaToCoverageEnable = FALSE;
		rtvdesc.BlendEnable = TRUE;

		rtvdesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		//rtvdesc.DestBlend = D3D11_BLEND_DEST_COLOR;
		rtvdesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		//rtvdesc.SrcBlend = D3D11_BLEND_SRC_COLOR;
		rtvdesc.BlendOp = D3D11_BLEND_OP_ADD;

		rtvdesc.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtvdesc.SrcBlendAlpha = D3D11_BLEND_ONE;
		rtvdesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;

		rtvdesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[0] = rtvdesc;
		D3D::GetDevice()->CreateBlendState(&desc, _rsThickness.bls.GetAddressOf());

	}


	///////////////////////////////////////////////////////////////////////////////
	// DSS
	///////////////////////////////////////////////////////////////////////////////
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		D3D11_DEPTH_STENCILOP_DESC opDesc;
		ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		ZeroMemory(&opDesc, sizeof(opDesc));
		opDesc.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		opDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		opDesc.StencilPassOp = D3D11_STENCIL_OP_REPLACE;		
		opDesc.StencilFunc = D3D11_COMPARISON_ALWAYS;	
		desc.BackFace = opDesc;
		desc.DepthEnable = TRUE;
		desc.DepthFunc = D3D11_COMPARISON_GREATER;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.FrontFace = opDesc;
		desc.StencilEnable = FALSE;
		desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;

		D3D::GetDevice()->CreateDepthStencilState(&desc, _rsRefractor.dss.GetAddressOf());

		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		//desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthEnable = FALSE;
		D3D::GetDevice()->CreateDepthStencilState(&desc, _rsDebug.dss.GetAddressOf());
		D3D::GetDevice()->CreateDepthStencilState(&desc, _rsBackGround.dss.GetAddressOf());
		D3D::GetDevice()->CreateDepthStencilState(&desc, _rsForeGround.dss.GetAddressOf());

		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		desc.DepthEnable = FALSE;
		D3D::GetDevice()->CreateDepthStencilState(&desc, _rsThickness.dss.GetAddressOf());
	}

	///////////////////////////////////////////////////////////////////////
	// RSS
	///////////////////////////////////////////////////////////////////////
	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.CullMode = D3D11_CULL_FRONT;
	}

}



void ParticleRenderer::Create()
{
	vector<ParticleVertexType> v;
	v.push_back(ParticleVertexType{ Vector3(0, _radius, 0), Vector2(0,0) });

	float phiStep = Math::PI / _stackCount;
	float thetaStep = Math::PI * 2.0f / _sliceCount;

	for (UINT i = 1; i <= _stackCount - 1; i++)
	{
		float phi = i * phiStep;

		for (UINT k = 0; k <= _sliceCount; k++)
		{
			float theta = k * thetaStep;
			if (k == _sliceCount)
				theta = 0.01f;

			Vector3 p = Vector3
			(
				(_radius * sinf(phi) * cosf(theta)),
				(_radius * cos(phi)),
				(_radius * sinf(phi) * sinf(theta))
			);

			Vector3 n;
			D3DXVec3Normalize(&n, &p);

			Vector3 t = Vector3
			(
				-(_radius * sinf(phi) * sinf(theta)),
				0.0f,
				(_radius * sinf(phi) * cosf(theta))
			);
			D3DXVec3Normalize(&t, &t);


			Vector2 uv = Vector2(theta / (Math::PI * 2), phi / Math::PI);
			if (k == _sliceCount)
				uv = Vector2(1.0f, phi / Math::PI);


			v.push_back(ParticleVertexType{ p , uv });
		}
	}
	v.push_back(ParticleVertexType{ Vector3(0, -_radius, 0) , Vector2(0,0) });


	_vertices = new ParticleVertexType[v.size()];
	_vertexCount = v.size();

	copy(v.begin(), v.end(), stdext::checked_array_iterator<ParticleVertexType *>(_vertices, _vertexCount));

	vector<UINT> i;
	for (UINT k = 1; k <= _sliceCount; k++)
	{
		i.push_back(0);
		i.push_back(k + 1);
		i.push_back(k);
	}

	UINT baseIndex = 1;
	UINT ringVertexCount = _sliceCount + 1;
	for (UINT k = 0; k < _stackCount - 2; k++)
	{
		for (UINT j = 0; j < _sliceCount; j++)
		{
			i.push_back(baseIndex + k * ringVertexCount + j);
			i.push_back(baseIndex + k * ringVertexCount + j + 1);
			i.push_back(baseIndex + (k + 1) * ringVertexCount + j);

			i.push_back(baseIndex + (k + 1) * ringVertexCount + j);
			i.push_back(baseIndex + k * ringVertexCount + j + 1);
			i.push_back(baseIndex + (k + 1) * ringVertexCount + j + 1);
		}
	}

	UINT southPoleIndex = v.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;

	for (UINT k = 0; k < _sliceCount; k++)
	{
		i.push_back(southPoleIndex);
		i.push_back(baseIndex + k);
		i.push_back(baseIndex + k + 1);
	}

	_indices = new UINT[i.size()];
	_indexCount = i.size();
	copy(i.begin(), i.end(), stdext::checked_array_iterator<UINT *>(_indices, _indexCount));

}

void ParticleRenderer::CreateQuad()
{

	float r = _radius;
	_vertexCount = 4;
	_vertices = new ParticleVertexType[_vertexCount];
	_vertices[0] = ParticleVertexType{ {-r, +r, 0}, {0, 0} };
	_vertices[1] = ParticleVertexType{ {+r, +r, 0}, {1, 0} };
	_vertices[2] = ParticleVertexType{ {+r, -r, 0}, {1, 1} };
	_vertices[3] = ParticleVertexType{ {-r, -r, 0}, {0, 1} };

	_indexCount = 6;
	_indices = new UINT[_indexCount];
	_indices[0] = 0;
	_indices[1] = 1;
	_indices[2] = 2;
	_indices[3] = 2;
	_indices[4] = 3;
	_indices[5] = 0;
}

void ParticleRenderer::CreateCube()
{
	float w = 0.5f;
	float h = 0.5f;
	float d = 0.5f;

	vector<ParticleVertexType> v;

	//Front
	v.push_back(ParticleVertexType{ { -w, -h, -d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { -w, +h, -d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { +w, +h, -d },{ 1, 0 } });
	v.push_back(ParticleVertexType{ { +w, -h, -d },{ 1, 1 } });
	//Back							  			 	   	 
	v.push_back(ParticleVertexType{ { -w, -h, +d },{ 1, 1 } });
	v.push_back(ParticleVertexType{ { +w, -h, +d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { +w, +h, +d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { -w, +h, +d },{ 1, 0 } });
	//Top							  			 	   	 
	v.push_back(ParticleVertexType{ { -w, +h, -d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { -w, +h, +d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { +w, +h, +d },{ 1, 0 } });
	v.push_back(ParticleVertexType{ { +w, +h, -d },{ 1, 1 } });
	//Bottom						  			 	   	 
	v.push_back(ParticleVertexType{ { -w, -h, -d },{ 1, 1 } });
	v.push_back(ParticleVertexType{ { +w, -h, -d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { +w, -h, +d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { -w, -h, +d },{ 1, 0 } });
	//Left							  			 	   	 
	v.push_back(ParticleVertexType{ { -w, -h, +d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { -w, +h, +d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { -w, +h, -d },{ 1, 0 } });
	v.push_back(ParticleVertexType{ { -w, -h, -d },{ 1, 1 } });
	//Right							  			 	   	 
	v.push_back(ParticleVertexType{ { +w, -h, -d },{ 0, 1 } });
	v.push_back(ParticleVertexType{ { +w, +h, -d },{ 0, 0 } });
	v.push_back(ParticleVertexType{ { +w, +h, +d },{ 1, 0 } });
	v.push_back(ParticleVertexType{ { +w, -h, +d },{ 1, 1 } });


	_bgVertices = new ParticleVertexType[v.size()];
	_bgVertexCount = v.size();

	copy(v.begin(), v.end(), stdext::checked_array_iterator<ParticleVertexType *>(_bgVertices, _bgVertexCount));

	_bgIndexCount = 36;
	this->_bgIndices = new UINT[_bgIndexCount]
	{
		0, 1, 2, 0, 2, 3,
		4, 5, 6, 4, 6, 7,
		8, 9, 10, 8, 10, 11,
		12, 13, 14, 12, 14, 15,
		16, 17, 18, 16, 18, 19,
		20, 21, 22, 20, 22, 23
	};
}

void ParticleRenderer::CreatePlane()
{	
	_planeRenderer = make_unique<MeshRender>(_shader, new MeshGrid(5.0f, 5.0f));
	_planeRenderer->Pass(static_cast<UINT>(eDispatch::plane));
	Transform* transform =_planeRenderer->AddTransform();
	transform->Scale(plane_scale , 1.0f , plane_scale);
	transform->Position(_info.boundary.x * 0.5f, 0.0f, _info.boundary.z * 0.5f);
	transform->Update();

	_planeRenderer->UpdateTransforms();
}
