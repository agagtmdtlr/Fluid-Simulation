#include "Framework.h"
#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::~Mesh()
{
	SafeDelete(vertexBuffer);
	SafeDelete(indexBuffer);

	SafeDeleteArray(vertices);
	SafeDeleteArray(indices);
}

void Mesh::SetShader(Shader * shader)
{
	this->shader = shader;
}

void Mesh::Update()
{
}

void Mesh::Render(UINT drawCount)
{
	if (vertexBuffer == NULL || indexBuffer == NULL)
	{
		Create();

		vertexBuffer = new VertexBuffer(vertices, vertexCount, sizeof(MeshVertex));
		indexBuffer = new IndexBuffer(indices, indexCount);
	}

	vertexBuffer->Render();
	indexBuffer->Render();	
	
	D3D::GetDC()->IASetPrimitiveTopology(topology);
	// 인스턴싱을 이용하여 드로우 한다.
	shader->DrawIndexedInstanced(0, pass, indexCount, drawCount);
}