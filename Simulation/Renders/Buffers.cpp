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
	// cpu 및 gpu의 읽기,쓰기 상태를 알려줌 
	// 각 상태에 따라 렌더리 속도가 다름
	if (bCpuWrite == false && bGpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE; //GPU 읽기
	}
	else if (bCpuWrite == true && bGpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_DYNAMIC; //CPU 쓰기, GPU 읽기
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	}
	else if (bCpuWrite == false && bGpuWrite == true)
	{
		//CPU 쓰기 가능 - UpdateSubResource
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
	// cpu 에서 gpu 로 계속 보내줄것이므로 dynamic 을 사용한다.
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
	// 해당 버퍼의 주소의 resource 를 복사해오기위함
	D3D11_MAPPED_SUBRESOURCE subResource;
	// D3D11_MAP_WRITE_DISCARD :: 쓰기용 덮어씌우기
	D3D::GetDC()->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		// 버퍼의 데이터 구간에 갱신된 data 를 덮어씌운다.
		// read 일 경우는 해당 인자의 순서가 바뀐다.
		memcpy(subResource.pData, data, dataSize);
	}
	// 락이 걸려있어서 해당 버퍼에 접근이 불가능하다.
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
	CreateUAV(); // gpu에서 출력 변수 uav

	CreateResult(); // 출력된 데이터를 cpu에서 받아올 변수 생성
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

	//CD3D11_BUFFER_DESC 접두사 c가 붙은것은 구조체를 상속받아 초깃값을 세팅해 주기 위한 구조체
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = inputByte;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // resource 형태의 버퍼 만들기
	// shader에서 byte address buffer로 받을수 있는 버퍼라는걸 명시하는 것이다.
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
	buffer->GetDesc(&desc); // 데이터의 갯수를 알기위해 버퍼의 desc를 가져온다.


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	// format : 4byte, R32 하나만 사용하겠다고 명시 ( GPU의 자료형은 float 하나이므로 GPU에서는 기본 Byte는 4Byte)
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // gpu는 기본 자료 단위가 4byte라서 raw관리가 용이하다.
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX; 
	// 아래 bufferex.flag / numelements 명시해주기 위해
	// viewDimesion에서 'D3D11_SRV_DIMENSION_BUFFEREX' bufferex가 붙는 enum을 사용한다.
	srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW; // srv rawbuffer로 사용하는 자료라는걸 명시해준다.
	srvDesc.BufferEx.NumElements = desc.ByteWidth / 4; // 데이터의 갯수

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
	desc.BindFlags = 0; // unorded access를 해제하기 위함 0
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
	//CopyResource란
	// 어떤 USAGE건 간에 src 리소스를 접근해서 dest 리소르로 복사해 줍니다.
	D3D::GetDC()->CopyResource(result, output);

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource);
	{
		memcpy(data, subResource.pData, outputByte);
	}
	D3D::GetDC()->Unmap(result, 0);

}


///////////////////////////////////////////////////////////////////////////////
// 구조체의 초기화 데이터 (입력)
// 입력 구조체의 하나의 크기
// 입력 데이터 개수
// 출력될 구조체 하나의 크기
// 출력 데이터의 개수

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

	input = (ID3D11Resource *)buffer; // 입력용 버퍼 자원 완성
}

void StructuredBuffer::CreateSRV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)input;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);

	// 세이더 입력용 자원 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.NumElements = inputCount; // 개수 넘겨주기

	Check(D3D::GetDevice()->CreateShaderResourceView(buffer, &srvDesc, &srv));
}

void StructuredBuffer::CreateOutput() // 출력용 자원생성
{
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.ByteWidth = OutputByteWidth(); // 전체크기 구조체 크기 * 개수
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = outputStride; // 출력용 자료의 단위

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = outputData;

	Check(D3D::GetDevice()->CreateBuffer(&desc, outputData != NULL ? &subResource : NULL, &buffer));

	output = (ID3D11Resource *)buffer;
}

void StructuredBuffer::CreateUAV() // 출력용 뷰 생성
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)output;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = outputCount; // ViewDimesion 세팅에 맞춰서 옵션 값 지정을 한다.

	Check(D3D::GetDevice()->CreateUnorderedAccessView(buffer, &uavDesc, &uav));
}


void StructuredBuffer::CreateResult()
{
	ID3D11Buffer* buffer; // UAV의 결과값을 받아서 cpu에 읽을 수 있도록하는 버퍼

	D3D11_BUFFER_DESC desc;
	((ID3D11Buffer *)output)->GetDesc(&desc); // uav의 desc를 참조한다.
	desc.Usage = D3D11_USAGE_STAGING; // cpu read
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	Check(D3D::GetDevice()->CreateBuffer(&desc, NULL, &buffer));

	result = (ID3D11Resource *)buffer;
}

void StructuredBuffer::CopyToInput(void * data) // 입력 데이터 복사하기
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
	D3D::GetDC()->CopyResource(result, input); // 자원복사.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result의 데이터를복사
	{
		memcpy(data, subResource.pData, dataCount * inputStride);
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBuffer::CopyFromOutput(void * data) //결과값을 반환받을 변수
{
	D3D::GetDC()->CopyResource(result, output); // 자원복사.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result의 데이터를복사
	{
		memcpy(data, subResource.pData, OutputByteWidth());
	}
	D3D::GetDC()->Unmap(result, 0);
}

void StructuredBuffer::CopyFromOutput(void * data, UINT dataCount)
{
	D3D::GetDC()->CopyResource(result, output); // 자원복사.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result의 데이터를복사
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
	arraySize = srcDesc.ArraySize; // textureArray도 받아올 수 있다.
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
	Check(D3D::GetDevice()->CreateTexture2D(&desc, NULL, &texture)); // 복사해서 담을 변수 생성
	// 텍스처 리소스 복사하기
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
	// 해당 읽어온 리소스를 다시 gpu에 바인드해주는 리소스로 사용할거라서
	// D3D11_BIND_SHADER_RESOURCE 플래그를 추가해준다.
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
	// 텍스처버퍼의 uav 생성
	Check(D3D::GetDevice()->CreateUnorderedAccessView(texture, &uavDesc, &uav));
}
// Result는 다시 SRV를 통해 세이더로 넘길 수 있또록 작업해 줍니다.
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

	resource = (ID3D11Resource *)buffer; // 입력용 버퍼 자원 완성
}

void StructuredBufferOnlyUAV::CreateSRV()
{
	ID3D11Buffer* buffer = (ID3D11Buffer *)resource;

	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);

	// 세이더 입력용 자원 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.NumElements = count; // 개수 넘겨주기

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
	uavDesc.Buffer.NumElements = count; // ViewDimesion 세팅에 맞춰서 옵션 값 지정을 한다.

	Check(D3D::GetDevice()->CreateUnorderedAccessView(buffer, &uavDesc, &uav));
}

void StructuredBufferOnlyUAV::CreateResult()
{
	ID3D11Buffer* buffer; // UAV의 결과값을 받아서 CPU에 읽을 수 있도록하는 버퍼

	D3D11_BUFFER_DESC desc;
	((ID3D11Buffer *)resource)->GetDesc(&desc); // UAV의 Desc를 참조한다.
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
	D3D::GetDC()->CopyResource(result, resource); // 자원복사.

	D3D11_MAPPED_SUBRESOURCE subResource;
	D3D::GetDC()->Map(result, 0, D3D11_MAP_READ, 0, &subResource); // result 데이터를 복사
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


