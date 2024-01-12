#pragma once

// 렌더링에 필요한 버퍼를 관리하기위한 클래스이다.
class Renderer
{
public:
	Renderer(Shader* shader);
	Renderer(wstring shaderFile);
	// 가상소멸자를 이용한 상속 클래스에서 콜되도록함
	virtual ~Renderer();

	Shader* GetShader() { return shader; }

	UINT& Pass() { return pass; }
	void Pass(UINT val) { pass = val; }

	virtual void Update();
	virtual void Render();

	Transform* GetTransform() { return transform; }

private:
	void Initialize();

protected:
	void Topology(D3D11_PRIMITIVE_TOPOLOGY val) { topology = val; }

protected:
	Shader* shader;

	// 공통적인 변수
	PerFrame* perFrame;

	Transform* transform;
	VertexBuffer* vertexBuffer = NULL;
	IndexBuffer* indexBuffer = NULL;

	UINT vertexCount = 0;
	UINT indexCount = 0;

private:
	// 쉐이더를 받아올지 아닐지에 따라 쉐이터를 내부에서 지울지 결정할때 사용한다.
	bool bCreateShader = false;

	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT pass = 0;

	
};