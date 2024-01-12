#pragma once
/*
������ interface�� �޾Ƽ�  ������ �Ͽ� ����ϴ°��� �����̴�.
class VertexBuffer : public ID3D11Buffer
{
};
*/

//wrapper class ���� Ŭ������ ����� ���μ� ����ϴ� ���
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
	// ������ �� ���� ���� ������ ������ �𸣴� void ����
	void* data;
	// ���� ����
	UINT count;
	UINT stride;
	UINT slot;
	// cpu���� ���� �ִ���
	bool bCpuWrite;
	// gpu���� ���� �ִ���
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
	// �����͸� �о��ֱ� ���ؼ� ����ϴ°��̴�.
	void Render();

private:
	ID3D11Buffer* buffer;

	void* data;
	// ����ü�� ũ��
	UINT dataSize;
};

/////////////////////////////////////////////////////////////////////////////////
class CsResource
{
	//ID3DResource : ID3D11Buffer, ID3D11Texture ���� �θ� �������̽�
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
	ID3D11Resource* input = NULL; // �Է¿� �ڿ�
	// buffer / texture� srv�� �ѱ�� resource�� srv�� �ѱ�� �ִ�.
	ID3D11ShaderResourceView* srv = NULL; // input

	ID3D11Resource* output = NULL;
	// ���ĵǾ� ���� ���� ���� ( default ���� )
	ID3D11UnorderedAccessView* uav = NULL; // output : �������� �޴´�.

	// default resource�� �����ؼ� cpu���� ���� �����ϰ� ���ش�.
	ID3D11Resource* result = NULL; // ��¿� �ڿ�
	

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
	void CopyToInput(void* data); // �����͸� �־��ֱ�
	void CopyFromOutput(void* data); // �����͸� �����ֱ�


private:
	void* inputData;

	UINT inputByte;
	UINT outputByte;
};


///////////////////////////////////////////////////////////////////////////////

// StructuredBuffer ����ü�� CS�� �Է�/��� �ڷ������� ���
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
	// ��ü �Է� ũ��
	UINT InputByteWidth() { return inputStride * inputCount; }
	// ��ü ��� ũ��
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
	//ID3DResource : ID3D11Buffer, ID3D11Texture ���� �θ� �������̽�
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
	ID3D11Resource * resource = NULL; // �Է¿� �ڿ�
	ID3D11ShaderResourceView* srv = NULL; // input
	ID3D11UnorderedAccessView* uav = NULL; // output : �������� �޴´�.

	ID3D11Resource* result = NULL; // ��¿� �ڿ�


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
	// ��ü �Է� ũ��
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