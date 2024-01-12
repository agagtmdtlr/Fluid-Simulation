#pragma once

class Mesh
{
public:
	typedef VertexTextureNormalTangent MeshVertex;
	
public:
	Mesh();
	virtual ~Mesh();

	void SetShader(Shader* shader);
	void Pass(UINT val) { pass = val; }
	void Topology(D3D11_PRIMITIVE_TOPOLOGY t) { topology = t; }

	void Update();
	void Render(UINT drawCount);

protected:
	virtual void Create() = 0;

protected:
	Shader* shader;
	UINT pass = 0;
	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	VertexBuffer* vertexBuffer = NULL;
	IndexBuffer* indexBuffer = NULL;

	MeshVertex* vertices;
	UINT* indices;

	UINT vertexCount, indexCount;
};