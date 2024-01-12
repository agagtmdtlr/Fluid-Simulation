 #include "Framework.h"
#include "RenderTarget.h"

RenderTarget::RenderTarget(UINT width, UINT height, DXGI_FORMAT format, bool useCompute)
	:format(format),
	rtv(nullptr),
	srv(nullptr),
	uav(nullptr)
{
	this->width = (width < 1) ? (UINT)D3D::Width() : width;
	this->height = (height < 1) ? (UINT)D3D::Height() : height;


	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.Width = this->width;
	textureDesc.Height = this->height;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	// srv 이면서 rtv 에 연결된 텍스처
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	if (useCompute)
	{
		textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1;
	Check(D3D::GetDevice()->CreateTexture2D(&textureDesc, NULL, &texture));

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Check(D3D::GetDevice()->CreateRenderTargetView(texture, &rtvDesc, &rtv));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = format; // texture format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	Check(D3D::GetDevice()->CreateShaderResourceView(texture, &srvDesc, &srv));


	if (useCompute)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uavDesc.Format = format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 1;
		Check(D3D::GetDevice()->CreateUnorderedAccessView(texture, &uavDesc, &uav));
	}
}

RenderTarget::~RenderTarget()
{
	SafeRelease(texture);
	SafeRelease(rtv);
	SafeRelease(srv);
	SafeRelease(uav);
}

void RenderTarget::SaveTexture(wstring file)
{
	Check(D3DX11SaveTextureToFile(D3D::GetDC(), texture, D3DX11_IFF_PNG, file.c_str()));
}

void RenderTarget::PreRender(DepthStencil * depthStencil)
{
	D3D::GetDC()->OMSetRenderTargets(1, &rtv, depthStencil->DSV());
	// rtv 의 버퍼 데이터를 비워준다.
	D3D::Get()->Clear(Color(0,0,0,1), rtv, depthStencil->DSV());
}

void RenderTarget::ClearRTV(Color color)
{
	D3D::GetDC()->ClearRenderTargetView(rtv, color);
}

void RenderTarget::PreRender(RenderTarget ** targets, UINT count, DepthStencil * depthStencil)
{
	vector<ID3D11RenderTargetView* > rtvs;
	for (UINT i = 0; i < count; i++)
	{
		ID3D11RenderTargetView* rtv = targets[i]->RTV();
		rtvs.push_back(rtv);

		D3D::GetDC()->ClearRenderTargetView(rtv, Color(0, 0, 0, 1));
	}

	D3D::GetDC()->ClearDepthStencilView(depthStencil->DSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	D3D::GetDC()->OMSetRenderTargets(rtvs.size(), &rtvs[0], depthStencil->DSV()); 
}
