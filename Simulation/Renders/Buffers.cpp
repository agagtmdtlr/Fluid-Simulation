#include "Framework.h"
#include "Buffers.h"

VertexBuffer::VertexBuffer(void * data, UINT count, UINT stride, UINT slot, bool bCpuWrite, bool bGpuWrite)
	: data(data), count(count), stride(stride), slot(slot)
	, bCpuWrite(bCpuWrite), bGpuWrite(bGpuWrite)
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = stride * count;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// Usage
	// cpu �� gpu�� �б�,���� ���¸� �˷��� 
	// �� ���¿� ���� ������ �ӵ��� �ٸ�
	if (bCpuWrite == false && bGpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE; //GPU �б�
	}
	else if (bCpuWrite == true && bGpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_DYNAMIC; //CPU ����, GPU �б�
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	}
	else if (bCpuWrite == false && bGpuWrite == true)
	{
		//CPU ���� ���� - UpdateSubResource
		desc.Usage = D3D11_USAGE_DEFAULT;
	}
	else
	{
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	}

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = data;

	Check(D3D::GetDevice()->CreateBuffer(&desc, (data == nullptr ? nullptr : &subResource) , &buffer));
}

VertexBuffer::~VertexBuffer()
{
	SafeRelease(buffer);
}

void VertexBuffer::Update()
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{		
		memcpy(subResource.pData, data, count * stride);
	}
	D3D::GetDC()->Unmap(buffer, 0);
}

void VertexBuffer::Render()
{
	UINT offset = 0;
	D3D::GetDC()->IASetVertexBuffers(slot, 1, &buffer, &stride, &offset);
}

///////////////////////////////////////////////////////////////////////////////

IndexBuffer::IndexBuffer(void * data, UINT count)
	: data(data), count(count)
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = sizeof(UINT) * count;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;


	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = data;

	Check(D3D::GetDevice()->CreateBuffer(&desc, &subResource, &buffer));
}

IndexBuffer::~IndexBuffer()
{
	SafeRelease(buffer);
}

void IndexBuffer::Render()
{
	D3D::GetDC()->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
}

///////////////////////////////////////////////////////////////////////////////

ConstantBuffer::ConstantBuffer(void * data, UINT dataSize)
	:data(data), dataSize(dataSize)
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = dataSize;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	// cpu ���� gpu �� ��� �����ٰ��̹Ƿ� dynamic �� ����Ѵ�.
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));
}

ConstantBuffer::~ConstantBuffer()
{
	SafeRelease(buffer);
}

void ConstantBuffer::Render()
{
	// �ش� ������ �ּ��� resource �� �����ؿ�������
	D3D11_MAPPED_SUBRESOURCE subResource;
	// D3D11_MAP_WRITE_DISCARD :: ����� ������
	D3D::GetDC()->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		// ������ ������ ������ ���ŵ� data �� ������.
		// read �� ���� �ش� ������ ������ �ٲ��.
		memcpy(subResource.pData, data, dataSize);
	}
	// ���� �ɷ��־ �ش� ���ۿ� ������ �Ұ����ϴ�.
	D3D::GetDC()->Unmap(buffer, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

CsResource::CsResource()
{
}

CsResource::~CsResource()
{
	SafeRelease(input);
	SafeRelease(srv);

	SafeRelease(output);
	SafeRelease(uav);

	SafeRelease(result);
}


void CsResource::CreateBuffer()
{
	CreateInput();
	CreateSRV();

	CreateOutput();
	CreateUAV(); // gpu���� ��� ���� uav

	CreateResult(); // ��µ� �����͸� cpu���� �޾ƿ� ���� ����
}

/////////////////////////////////////////////////////////////////////////////////////////////


RawBuffer::RawBuffer(void * inputData, UINT inputByte, UINT outputByte)
	:CsResource()
	,inputData(inputData), inputByte(inputByte) , outputByte(outputByte)
{
	CreateBuffer();
}

RawBuffer::~RawBuffer()
{
}

void RawBuffer::CreateInput()
{
	if (inputByte < 1) return;

	ID3D11Buffer* buffer = NULL;

	//CD3D11_BUFFER_DESC ���λ� c�� �������� ����ü�� ��ӹ޾� �ʱ갪�� ������ �ֱ� ���� ����ü
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = inputByte;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // resource ������ ���� �����
	// shader���� byte address buffer�� ������ �ִ� ���۶�°� ����ϴ� ���̴�.
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = inputData;

	Check(D3D::GetDevice()->CreateBuffer(&desc, inputData != NULL ? &subResource : NULL, &buffer));

	input = (ID3D11Resource *)buffer;


}

void RawBuffer::CreateSRV()
{
	if (inputByte < 1) return;


	ID3D11Buffer* buffer = (ID3D11Buffer *)input;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc); // �������� ������ �˱����� ������ desc�� �����´�.


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	// format : 4byte, R32 �ϳ��� ����ϰڴٰ� ��� ( GPU�� �ڷ����� float �ϳ��̹Ƿ� GPU������ �⺻ Byte�� 4Byte)
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // gpu�� �⺻ �ڷ� ������ 4byte�� raw������ �����ϴ�.
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX; 
	// �Ʒ� bufferex.flag / numelements ������ֱ� ����
	// viewDimesion���� 'D3D11_SRV_DIMENSION_BUFFEREX' bufferex�� �ٴ� enum�� ����Ѵ�.
	srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW; // srv rawbuffer�� ����ϴ� �ڷ��°� ������ش�.
	srvDesc.BufferEx.NumElements = desc.ByteWidth / 4; // �������� ����

	Check(D3D::GetDevice()->CreateShaderResourceView(buffer, &srvDesc, &srv));
}

void RawBuffer::CreateOutput()
{
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = outputByte;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));

	output = (ID3D11Resource *)buffer;
}

void RawBuffer::CreateUAV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)output;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc; // usage_defalut
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.NumElements = desc.ByteWidth / 4;

	Check(D3D::GetDevice()->CreateUnorderedAccessView(buffer, &uavDesc, &uav));
}

void RawBuffer::CreateResult()
{
	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc;
	((ID3D11Buffer*)output)->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0; // unorded access�� �����ϱ� ���� 0
	desc.MiscFlags = 0;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));

	result = (ID3D11Resource *)buffer;
}

void RawBuffer::CopyToInput(void * data)
{
	D3D11_MAPPED_SUBRESOURCE subResource;

	D3D::GetDC()->Map(input, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, data, inputByte);
	}
	D3D::GetDC()->Unmap(input, 0);
}

void RawBuffer::CopyFromOutput(void * data)
{
	//CopyResource��
	// � USAGE�� ���� src ���ҽ��� �����ؼ� dest ���Ҹ��� ������ �ݴϴ�.
	D3D::GetDC()->CopyResource(result, output);

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource);
	{
		memcpy(data, subResource.pData, outputByte);
	}
	D3D::GetDC()->Unmap(result, 0);

}


///////////////////////////////////////////////////////////////////////////////
// ����ü�� �ʱ�ȭ ������ (�Է�)
// �Է� ����ü�� �ϳ��� ũ��
// �Է� ������ ����
// ��µ� ����ü �ϳ��� ũ��
// ��� �������� ����

StructuredBuffer::StructuredBuffer(void* inputData, UINT inputStride, UINT inputCount, UINT outputStride, UINT outputCount, void * outputData)
	: CsResource()
	, inputData(inputData)
	, inputStride(inputStride), inputCount(inputCount)
	, outputData(outputData)
	, outputStride(outputStride), outputCount(outputCount)
{
	if (outputStride == 0 || outputCount == 0)
	{
		this->outputStride = inputStride; 
		this->outputCount = inputCount;
	}

	CreateBuffer();
}

StructuredBuffer::~StructuredBuffer()
{

}

void StructuredBuffer::CreateInput()
{
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = InputByteWidth();
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = inputStride;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = inputData;

	Check(D3D::GetDevice()->CreateBuffer(&desc, inputData != NULL ? &subResource : NULL, &buffer));

	input = (ID3D11Resource *)buffer; // �Է¿� ���� �ڿ� �ϼ�
}

void StructuredBuffer::CreateSRV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)input;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);

	// ���̴� �Է¿� �ڿ� �� ����
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.NumElements = inputCount; // ���� �Ѱ��ֱ�

	Check(D3D::GetDevice()->CreateShaderResourceView(buffer, &srvDesc, &srv));
}

void StructuredBuffer::CreateOutput() // ��¿� �ڿ�����
{
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = OutputByteWidth(); // ��üũ�� ����ü ũ�� * ����
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = outputStride; // ��¿� �ڷ��� ����

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = outputData;

	Check(D3D::GetDevice()->CreateBuffer(&desc, outputData != NULL ? &subResource : NULL, &buffer));

	output = (ID3D11Resource *)buffer;
}

void StructuredBuffer::CreateUAV() // ��¿� �� ����
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)output;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = outputCount; // ViewDimesion ���ÿ� ���缭 �ɼ� �� ������ �Ѵ�.

	Check(D3D::GetDevice()->CreateUnorderedAccessView(buffer, &uavDesc, &uav));
}


void StructuredBuffer::CreateResult()
{
	ID3D11Buffer* buffer; // UAV�� ������� �޾Ƽ� cpu�� ���� �� �ֵ����ϴ� ����

	D3D11_BUFFER_DESC desc;
	((ID3D11Buffer *)output)->GetDesc(&desc); // uav�� desc�� �����Ѵ�.
	desc.Usage = D3D11_USAGE_STAGING; // cpu read
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));

	result = (ID3D11Resource *)buffer;
}

void StructuredBuffer::CopyToInput(void * data) // �Է� ������ �����ϱ�
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(input, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, data, InputByteWidth());
	}
	D3D::GetDC()->Unmap(input, 0);
}

void StructuredBuffer::CopyFromInput(void * data, UINT dataCount)
{
	D3D::GetDC()->CopyResource(result, input); // �ڿ�����.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result�� �����͸�����
	{
		memcpy(data, subResource.pData, dataCount * inputStride);
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBuffer::CopyFromOutput(void * data) //������� ��ȯ���� ����
{
	D3D::GetDC()->CopyResource(result, output); // �ڿ�����.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result�� �����͸�����
	{
		memcpy(data, subResource.pData, OutputByteWidth());
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBuffer::CopyFromOutput(void * data, UINT dataCount)
{
	D3D::GetDC()->CopyResource(result, output); // �ڿ�����.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result�� �����͸�����
	{
		memcpy(data, subResource.pData, dataCount * outputStride);
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBuffer::CopyToInputFromOutput()
{
	if (outputCount * outputStride == inputCount * inputStride)
	{
		D3D::GetDC()->CopyResource(input, output);
	}
}

void StructuredBuffer::CopyToOutput(void * data)
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(output, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, data, OutputByteWidth());
	}
	D3D::GetDC()->Unmap(output, 0);
}

void StructuredBuffer::ResetUAV()
{
	UINT clearData[4] = { 0 };
	D3D::GetDC()->ClearUnorderedAccessViewUint(uav, clearData);
}

///////////////////////////////////////////////////////////////////////////////

TextureBuffer::TextureBuffer(ID3D11Texture2D * src)
{
	D3D11_TEXTURE2D_DESC srcDesc;
	src->GetDesc(&srcDesc);

	width = srcDesc.Width;
	height = srcDesc.Height;
	arraySize = srcDesc.ArraySize; // textureArray�� �޾ƿ� �� �ִ�.
	format = srcDesc.Format;

	// input texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = arraySize;
	desc.Format = format;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;

	ID3D11Texture2D* texture = NULL;
	Check(D3D::GetDevice()->CreateTexture2D(&desc, NULL, &texture)); // �����ؼ� ���� ���� ����
	// �ؽ�ó ���ҽ� �����ϱ�
	D3D::GetDC()->CopyResource(texture, src);

	input = (ID3D11Resource *)texture;

	CreateBuffer();
}

TextureBuffer::~TextureBuffer()
{

}

void TextureBuffer::CreateSRV()
{
	ID3D11Texture2D* texture = (ID3D11Texture2D *)input;

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.ArraySize = arraySize;

	Check(D3D::GetDevice()->CreateShaderResourceView(texture, &srvDesc, &srv));
}

void TextureBuffer::CreateOutput()
{
	ID3D11Texture2D* texture = NULL;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = arraySize;
	desc.Format = format;
	// �ش� �о�� ���ҽ��� �ٽ� gpu�� ���ε����ִ� ���ҽ��� ����ҰŶ�
	// D3D11_BIND_SHADER_RESOURCE �÷��׸� �߰����ش�.
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	Check(D3D::GetDevice()->CreateTexture2D(&desc, NULL, &texture));

	output = (ID3D11Resource *)texture;


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.ArraySize = arraySize;

	Check(D3D::GetDevice()->CreateShaderResourceView(texture, &srvDesc, &outputSRV));
}

void TextureBuffer::CreateUAV()
{
	ID3D11Texture2D* texture = (ID3D11Texture2D*)output;

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	//uavDesc.Texture2DArray.MipSlice = 1;
	uavDesc.Texture2DArray.ArraySize = arraySize;
	// �ؽ�ó������ uav ����
	Check(D3D::GetDevice()->CreateUnorderedAccessView(texture, &uavDesc, &uav));
}
// Result�� �ٽ� SRV�� ���� ���̴��� �ѱ� �� �ֶǷ� �۾��� �ݴϴ�.
void TextureBuffer::CreateResult()
{
	ID3D11Texture2D* texture;

	D3D11_TEXTURE2D_DESC desc;
	((ID3D11Texture2D *)output)->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	Check(D3D::GetDevice()->CreateTexture2D(&desc, NULL, &texture));

	result = (ID3D11Resource *)texture;
}

void TextureBuffer::CopyToInput(ID3D11Texture2D * texture)
{
	D3D::GetDC()->CopyResource(input, texture);
}

ID3D11Texture2D * TextureBuffer::CopyFromOutput()
{
	D3D::GetDC()->CopyResource(result, output);

	return (ID3D11Texture2D *)result;
}

///////////////////////////////////////////////////////////////////////////////////////////

CsResourceOnlyUAV::CsResourceOnlyUAV()
{
}

CsResourceOnlyUAV::~CsResourceOnlyUAV()
{
}


void CsResourceOnlyUAV::CreateBuffer()
{
	CreateResource();
	CreateSRV();
	CreateUAV();

	CreateResult();
}


///////////////////////////////////////////////////////////////////////////////////////////

StructuredBufferOnlyUAV::StructuredBufferOnlyUAV(void * data, UINT stride, UINT count)
	:_data(data),
	stride(stride),
	count(count)
{
	CreateBuffer();
}

StructuredBufferOnlyUAV::~StructuredBufferOnlyUAV()
{
}

void StructuredBufferOnlyUAV::CreateResource()
{
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = ByteWidth();
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = stride;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = _data;

	Check(D3D::GetDevice()->CreateBuffer(&desc, _data != NULL ? &subResource : NULL, &buffer));

	resource = (ID3D11Resource *)buffer; // �Է¿� ���� �ڿ� �ϼ�
}

void StructuredBufferOnlyUAV::CreateSRV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)resource;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);

	// ���̴� �Է¿� �ڿ� �� ����
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.NumElements = count; // ���� �Ѱ��ֱ�

	Check(D3D::GetDevice()->CreateShaderResourceView(buffer, &srvDesc, &srv));
}

void StructuredBufferOnlyUAV::CreateUAV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)resource;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = count; // ViewDimesion ���ÿ� ���缭 �ɼ� �� ������ �Ѵ�.

	Check(D3D::GetDevice()->CreateUnorderedAccessView(buffer, &uavDesc, &uav));
}

void StructuredBufferOnlyUAV::CreateResult()
{
	ID3D11Buffer* buffer; // UAV�� ������� �޾Ƽ� CPU�� ���� �� �ֵ����ϴ� ����

	D3D11_BUFFER_DESC desc;
	((ID3D11Buffer *)resource)->GetDesc(&desc); // UAV�� Desc�� �����Ѵ�.
	desc.Usage = D3D11_USAGE_STAGING; // CPU read
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));

	result = (ID3D11Resource *)buffer;
}

void StructuredBufferOnlyUAV::CopyToInput(void * data)
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, data, ByteWidth());
	}
	D3D::GetDC()->Unmap(resource, 0);
}

void StructuredBufferOnlyUAV::CopyFromOutput(void * data)
{
	D3D::GetDC()->CopyResource(result, resource); // �ڿ�����.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result �����͸� ����
	{
		memcpy(data, subResource.pData, ByteWidth());
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBufferOnlyUAV::ResetUAV()
{
	UINT clearData[4] = { 0 };
	D3D::GetDC()->ClearUnorderedAccessViewUint(uav, clearData);
}


