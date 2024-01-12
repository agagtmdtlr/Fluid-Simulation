#pragma once

enum class eRenderingSystemType : UINT8
{
	marchingCube,
	raycasting
};

class RenderingSystem
{
public: RenderingSystem();
public: ~RenderingSystem();
};
