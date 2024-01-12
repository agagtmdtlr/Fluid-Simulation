#pragma once
#define MAX_MESH_INSTANCE 500

class MeshRender
{
public:
	// �׸� �Ž��� �Ѿ��
	MeshRender(Shader* shader, Mesh* mesh);
	~MeshRender();

	Mesh* GetMesh() { return mesh; }

	void Update();
	void Render();

	void Pass(UINT val) { mesh->Pass(val); }

	Transform* AddTransform();
	Transform* GetTransform(UINT index) { return transforms[index]; }
	UINT GetTransformCount() { return transforms.size();	}
	void UpdateTransforms();

private:
	Mesh* mesh;

	vector<Transform *> transforms;
	// gpu ����
	Matrix worlds[MAX_MESH_INSTANCE];

	VertexBuffer* instanceBuffer;
};