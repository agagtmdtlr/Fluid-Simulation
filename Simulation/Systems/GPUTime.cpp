#include "framework.h"
#include "GPUTime.h"

GPUTime* GPUTime::instance = NULL;

GPUTime::GPUTime(void) 
{
	///////////////////////////////////////////////////////////////////////////
	// GPU 프로파일링 객체 생성
	///////////////////////////////////////////////////////////////////////////
	{
		D3D11_QUERY_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_QUERY_DESC));
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		Check(D3D::GetDevice()->CreateQuery(&desc, &_queryDisjoint));
	}
	{
		D3D11_QUERY_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_QUERY_DESC));
		desc.Query = D3D11_QUERY_TIMESTAMP;
		Check(D3D::GetDevice()->CreateQuery(&desc, &_queryBeginFrame));
		Check(D3D::GetDevice()->CreateQuery(&desc, &_queryEndFrame));
	}
}

GPUTime::~GPUTime(void)
{
	for (UINT i = 0; i < _queryTimers.size(); i++)
	{
		SafeRelease(_queryTimers[i]);
	}
}

GPUTime* GPUTime::Get()
{
	assert(instance != NULL);

	return instance;
}

void GPUTime::Create()
{
	assert(instance == NULL);

	instance = new GPUTime();
}

void GPUTime::Delete()
{
	SafeDelete(instance);
}

void GPUTime::Start()
{
	D3D::GetDC()->Begin(_queryDisjoint);
	D3D::GetDC()->End(_queryBeginFrame);
}

void GPUTime::Stop()
{
	D3D::GetDC()->End(_queryEndFrame);
	D3D::GetDC()->End(_queryDisjoint);

	//return;

	UINT cnt = 0;
	while (D3D::GetDC()->GetData(_queryDisjoint, NULL, 0, 0) == S_FALSE)
	{
		Sleep(0); //시간 손실이 크지 않을까?
		//cnt++;
	}

	//if (cnt > 0)
	//{
	//	assert(true);
	//}
	

	D3D::GetDC()->GetData(_queryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
	if (tsDisjoint.Disjoint)
	{
		return;
	}

	UINT64 tsBeginFrame;
	UINT64 tsQueryTimer;
	D3D::GetDC()->GetData(_queryBeginFrame, &tsBeginFrame, sizeof(UINT64), 0);

	for (UINT i = 0; i < _queryTimers.size(); i++)
	{
		D3D::GetDC()->GetData(_queryTimers[i], &tsQueryTimer, sizeof(UINT64), 0);

		// ms 단위로 저장된다.
		_elapse[i] = static_cast<float>(tsQueryTimer - tsBeginFrame) /
			static_cast<float>(tsDisjoint.Frequency) * 1000.0f;
	}

}
