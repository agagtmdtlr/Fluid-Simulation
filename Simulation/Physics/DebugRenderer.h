#pragma once
#include "Renders/VertexLayouts.h"


class DebugRenderer
{
private:
	enum : UINT {
		VERTEX_BUFFER = 0,
		INSTANCE_BUFFER = 1
	};

private: struct DebugCBufferInfo
{
	Matrix view;
	Matrix projection;
};	

public:	typedef Vertex DebugBoxVertex;

public: struct DebugBoxInstance 
{
	Vector3 Position;
};

public: DebugRenderer(const GridInfo info, const UINT position);
public: ~DebugRenderer();

public: void SetBoundary(Vector3 bb) { _instanceData[0] = bb; }

public: void Update();
public: void Render();

private: void Create();

private: GridInfo _gridInfo;
private: UINT _bufferCount;

private: DebugCBufferInfo _cbufferInfo;

private: vector<DebugBoxVertex> _vertexData;	
private: vector<UINT> _indexData;
private: vector<Vector3> _instanceData;

private: unique_ptr<Shader> _debugShader;

private: unique_ptr<ConstantBuffer> _infoConstantBuffer;

	
private: unique_ptr<VertexBuffer> _debugBoxVertexBuffer;
private: unique_ptr<VertexBuffer> _debugBoxInstanceBuffer;
private: unique_ptr<IndexBuffer>  _debugBoxIndexBuffer;

};
