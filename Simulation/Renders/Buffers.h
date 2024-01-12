#pragma once
/*
원래는 interface를 받아서  재정의 하여 사용하는것이 정석이다.
class VertexBuffer : public ID3D11Buffer
{
};
*/

//wrapper class 원본 클래스를 멤버로 감싸서 사용하는 방식
class VertexBuffer
{
public:
	VertexBuffer(void* data, UINT count, UINT stride, UINT slot = 0, bool bCpuWrite = false, bool bGpuWrite = false);
	~VertexBuffer();

	UINT Count() { return count; }
	UINT Stride() { return stride; }
	ID3D11Buffer* Buffer() { return buffer; }

	void Update();
	void Render();

private:
	ID3D11Buffer* buffer;
	// 정점이 들어갈 변수 언제 무엇이 들어올지 모르니 void 형식
	void* data;
	// 정점 개수
	UINT count;
	UINT stride;
	UINT slot;
	// cpu에서 쓸수 있는지
	bool bCpuWrite;
	// gpu에서 쓸수 있는지
	bool bGpuWrite;
};

///////////////////////////////////////////////////////////////////////////////

class IndexBuffer
{
public:
	IndexBuffer(void* data, UINT count);
	~IndexBuffer();

	UINT Count() { return count; }
	ID3D11Buffer* Buffer() { return buffer; }

	void Render();

private:
	ID3D11Buffer* buffer;

	void* data;
	UINT count;
};

///////////////////////////////////////////////////////////////////////////////

class ConstantBuffer
{
public:
	ConstantBuffer(void* data, UINT dataSize);
	~ConstantBuffer();

	ID3D11Buffer* Buffer() { return buffer; }
	// 데이터를 밀어주기 위해서 사용하는것이다.
	void Render();

private:
	ID3D11Buffer* buffer;

	void* data;
	// 구조체의 크기
	UINT dataSize;
};

/////////////////////////////////////////////////////////////////////////////////
class CsResource
{
	//ID3DResource : ID3D11Buffer, ID3D11Texture 등의 부모 인터페이스
public:
	CsResource();
	virtual ~CsResource();

public:
	ID3D11Resource* GetOutput(){ return output; }

protected:

	virtual void CreateInput() {}
	virtual void CreateSRV() {}

	virtual void CreateOutput() {}
	virtual void CreateUAV() {}

	virtual void CreateResult() {}

	void CreateBuffer();

public:
	ID3D11ShaderResourceView* SRV() { return srv; }
	ID3D11UnorderedAccessView* UAV() { return uav; }


protected:
	ID3D11Resource* input = NULL; // 입력용 자원
	// buffer / texture등도 srv로 넘기듯 resource도 srv로 넘길수 있다.
	ID3D11ShaderResourceView* srv = NULL; // input

	ID3D11Resource* output = NULL;
	// 정렬되어 있지 않은 상태 ( default 상태 )
	ID3D11UnorderedAccessView* uav = NULL; // output : 연산결과를 받는다.

	// default resource를 복사해서 cpu에서 접근 가능하게 해준다.
	ID3D11Resource* result = NULL; // 출력용 자원
	

};

/////////////////////////////////////////////////////////////////////////////////////////////




class RawBuffer : public CsResource
{
public:
	RawBuffer(void* inputData, UINT inputByte, UINT outputByte);
	~RawBuffer();

private:
	void CreateInput() override;
	void CreateSRV() override;

	void CreateOutput() override;
	void CreateUAV() override;

	void CreateResult() override;

public:
	void CopyToInput(void* data); // 데이터를 넣어주기
	void CopyFromOutput(void* data); // 데이터를 꺼내주기


private:
	void* inputData;

	UINT inputByte;
	UINT outputByte;
};


///////////////////////////////////////////////////////////////////////////////

// StructuredBuffer 구조체를 CS의 입력/출력 자료형으로 사용
class StructuredBuffer : public CsResource
{
public:
	StructuredBuffer(void* inputData, UINT inputStride, UINT inputCount, UINT outputStride = 0, UINT outputCount = 0 , void * outputData = nullptr);
	~StructuredBuffer();

private:
	void CreateInput() override;
	void CreateSRV() override;

	void CreateOutput() override;
	void CreateUAV() override;

	void CreateResult() override;


public:
	// 전체 입력 크기
	UINT InputByteWidth() { return inputStride * inputCount; }
	// 전체 출력 크기
	UINT OutputByteWidth() { return outputStride * outputCount; }

	void CopyToInput(void* data);
	void CopyFromInput(void* data, UINT dataCount);
	void CopyFromOutput(void* data);
	void CopyFromOutput(void* data, UINT dataCount);

	void CopyToInputFromOutput();

	void CopyToOutput(void* data);

	void ResetUAV();

private:
	void* inputData;

	UINT inputStride;
	UINT inputCount;

	UINT outputStride;
	UINT outputCount;

	void* outputData;
};

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

class TextureBuffer : public CsResource
{
public:
	TextureBuffer(ID3D11Texture2D* src);
	~TextureBuffer();

private:
	void CreateSRV() override;

	void CreateOutput() override;
	void CreateUAV() override;

	void CreateResult() override;

public:
	UINT Width() { return width; }
	UINT Height() { return height; }
	UINT ArraySize() { return arraySize; }

	ID3D11Texture2D* Output() { return (ID3D11Texture2D *)output; }
	ID3D11ShaderResourceView* OutputSRV() { return outputSRV; }

	void CopyToInput(ID3D11Texture2D* texture);
	ID3D11Texture2D* CopyFromOutput();
	ID3D11Texture2D* Result() { return (ID3D11Texture2D *)result; }

private:
	UINT width, height, arraySize;
	DXGI_FORMAT format;

	ID3D11ShaderResourceView* outputSRV;
};

/////////////////////////////////////////////////////////////////////////////////
class CsResourceOnlyUAV
{
	//ID3DResource : ID3D11Buffer, ID3D11Texture 등의 부모 인터페이스
public:
	CsResourceOnlyUAV();
	virtual ~CsResourceOnlyUAV();

protected: virtual void CreateResource() {}
protected: virtual void CreateSRV() {}
protected: virtual void CreateUAV() {}

protected: virtual void CreateResult() {}

protected: void CreateBuffer();

public:
	ID3D11ShaderResourceView * SRV() { return srv; }
	ID3D11UnorderedAccessView* UAV() { return uav; }

protected:
	ID3D11Resource * resource = NULL; // 입력용 자원
	ID3D11ShaderResourceView* srv = NULL; // input
	ID3D11UnorderedAccessView* uav = NULL; // output : 연산결과를 받는다.

	ID3D11Resource* result = NULL; // 출력용 자원


};
/////////////////////////////////////////////////////////////////////////////////

class StructuredBufferOnlyUAV : public CsResourceOnlyUAV
{
public:
	StructuredBufferOnlyUAV(void* data, UINT stride, UINT count);
	~StructuredBufferOnlyUAV();

private:
	virtual void CreateResource() override;
	virtual void CreateSRV() override;
	virtual void CreateUAV() override;

	virtual void CreateResult() override;

public:
	// 전체 입력 크기
	UINT ByteWidth() { return stride * count; }

	void CopyToInput(void* data);
	void CopyFromOutput(void* data);

	void ResetUAV();

private:
	void* _data;

	UINT stride;
	UINT count;

};

///////////////////////////////////////////////////////////////////////////////