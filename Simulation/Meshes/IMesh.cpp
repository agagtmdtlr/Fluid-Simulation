#include "Framework.h"
#include "IMesh.h"

IMesh::IMesh(Shader * shader)
	:
	mShader(shader)
	, mTechnique(0)
	, mPass(0)
{
	//my_assert(L"shader NA Error",shader != nullptr);
}

void IMesh::Initialize()
{
}
void IMesh::Update()
{
}
// build assert class
void IMesh::Render()
{
	//my_assert(L"vertexbuffer NA Error", false);
	mVertexBuffer->Render();
	if (mIndexBuffer != nullptr)
	{
		mIndexBuffer->Render();
		mShader->DrawIndexed(mTechnique,mPass,mIndexBuffer->Count());
	}
	else
	{
		mShader->Draw(mTechnique, mPass, mVertexBuffer->Count());
	}
}
