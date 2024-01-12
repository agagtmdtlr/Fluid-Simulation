#include "Framework.h"
#include "Physics/SPHcommon.h"
#include "Physics/DebugRenderer.h"

DebugRenderer::DebugRenderer(const GridInfo info, const UINT position)
	:
	_gridInfo(info),
	_bufferCount(info.x * info.y * info.z),
	_debugShader(make_unique<Shader>(L"debugRenderer.fx")),
	_instanceData(1)
{
	Create();

	Context::Get()->GetCamera()->GetMatrix(&_cbufferInfo.view);
	Context::Get()->GetPerspective()->GetMatrix(&_cbufferInfo.projection);

	_infoConstantBuffer = make_unique<ConstantBuffer>(&_cbufferInfo,sizeof(DebugCBufferInfo));

	_debugBoxVertexBuffer = make_unique<VertexBuffer>(&_vertexData[0], _vertexData.size(), sizeof(DebugBoxVertex), VERTEX_BUFFER, true, false);
	_debugBoxInstanceBuffer = make_unique<VertexBuffer>(&_instanceData[0], _instanceData.size() ,sizeof(DebugBoxInstance), INSTANCE_BUFFER, true, false);
	_debugBoxIndexBuffer = make_unique<IndexBuffer>(&_indexData[0], _indexData.size());
}
DebugRenderer::~DebugRenderer()
{
}
void DebugRenderer::Update()
{
	Context::Get()->GetCamera()->GetMatrix(&_cbufferInfo.view);
	Context::Get()->GetPerspective()->GetMatrix(&_cbufferInfo.projection);
	//_instanceData[0] = { 4.5, 4.5f, 4.5f };
	_debugBoxInstanceBuffer->Update();
}
void DebugRenderer::Render()
{
	// resource binding
	_infoConstantBuffer->Render();

	_debugBoxVertexBuffer->Render();
	_debugBoxIndexBuffer->Render();
	_debugBoxInstanceBuffer->Render();

	_debugShader->AsConstantBuffer("cbInfo_Debug")->SetConstantBuffer(_infoConstantBuffer->Buffer());
	// render
	D3D::Get()->SetRenderTarget();
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	_debugShader->DrawIndexedInstanced(0, 0,
		_debugBoxIndexBuffer->Count(),
		_debugBoxInstanceBuffer->Count(),
		0,0,0
	);

}
void DebugRenderer::Create()
{
	float w = 0.5f;
	float h = 0.5f;
	float d = 0.5f;

	// ÅºÁ¨Æ® U (¿À¸¥ÂÊ ¹æÇâ)
	//Front
	_vertexData.push_back(DebugBoxVertex(-w, -h, -d));
	_vertexData.push_back(DebugBoxVertex(-w, +h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, -h, -d));

	//Back
	_vertexData.push_back(DebugBoxVertex(-w, -h, +d));
	_vertexData.push_back(DebugBoxVertex(+w, -h, +d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, +d));
	_vertexData.push_back(DebugBoxVertex(-w, +h, +d));

	//Top
	_vertexData.push_back(DebugBoxVertex(-w, +h, -d));
	_vertexData.push_back(DebugBoxVertex(-w, +h, +d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, +d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, -d));

	//Bottom
	_vertexData.push_back(DebugBoxVertex(-w, -h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, -h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, -h, +d));
	_vertexData.push_back(DebugBoxVertex(-w, -h, +d));

	//Left
	_vertexData.push_back(DebugBoxVertex(-w, -h, +d));
	_vertexData.push_back(DebugBoxVertex(-w, +h, +d));
	_vertexData.push_back(DebugBoxVertex(-w, +h, -d));
	_vertexData.push_back(DebugBoxVertex(-w, -h, -d));

	//Right
	_vertexData.push_back(DebugBoxVertex(+w, -h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, -d));
	_vertexData.push_back(DebugBoxVertex(+w, +h, +d));
	_vertexData.push_back(DebugBoxVertex(+w, -h, +d));		

	_indexData = 
	{
		0, 1, 1, 2, 2, 3, 3, 0,			// front	
		4, 5, 5, 6, 6, 7, 7, 4,			// back
		8, 9, 9, 10, 10, 11, 11, 8,		// top
		12, 13, 13, 14, 14, 15, 15, 12,	// bottom
		16, 17, 17, 18, 18, 19, 19, 16, // left
		20, 21, 21, 22, 22, 23, 23, 20, // right
	};
}
