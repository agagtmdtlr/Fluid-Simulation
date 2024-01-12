#pragma once

class GPUTime
{
public:
	static GPUTime* Get();

	static void Create();
	static void Delete();

	void Start();
	void Stop();

	// bind 한 query 의 인덱스를 반환하여
	// 이후 Get time 의 호출시 이때 반환 받은 값으로 검색한다.
	UINT CreateQuery()
	{ 

		D3D11_QUERY_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.MiscFlags = 0;
		desc.Query = D3D11_QUERY_TIMESTAMP;
		ID3D11Query * query;
		D3D::GetDevice()->CreateQuery(&desc, &query);
		_queryTimers.push_back(query);
		_elapse.push_back(0);

		return _queryTimers.size() - 1;
	}

	bool RequestTimer(UINT index)
	{
		D3D::GetDC()->End(_queryTimers[index]);
		return true;
	}

	float GetElapse(UINT index)
	{
		return _elapse[index];
	}

	

private:
	GPUTime(void);
	~GPUTime(void);

	static GPUTime* instance;///< 싱글톤 객체

	D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;

	ID3D11Query* _queryDisjoint;
	ID3D11Query* _queryBeginFrame;
	ID3D11Query* _queryEndFrame;

	vector<ID3D11Query*> _queryTimers;
	vector<float> _elapse;
};

