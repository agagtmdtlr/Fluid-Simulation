#pragma once


class IMesh
{
public:
	IMesh(Shader* shader);
	virtual ~IMesh() {}

	virtual void Initialize();
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void ReadFile(wstring file) = 0;

	virtual UINT GetPass() final { return mPass; }
	virtual void SetPass(const UINT & pass) final { mPass = pass; }
	virtual UINT GetTechnique() final { return mTechnique; }
	virtual void SetTechnique(const UINT & technique) final { mTechnique = technique; }

protected:
	unique_ptr<VertexBuffer> mVertexBuffer;
	unique_ptr<VertexBuffer> mIndexBuffer;
private:
	Shader* mShader; // external data;
	UINT mTechnique;
	UINT mPass;
};

