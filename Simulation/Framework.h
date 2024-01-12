#pragma once

#include <Windows.h>
#include <cassert>
// STL
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <string>
#include <list>
#include <unordered_map>
#include <functional>
#include <iterator>
#include <mutex> // thread race condition
#include <thread> // paralle instruction
#include <wrl/client.h> // ComPtr]

//Direct3D

#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <d3d11.h>
#include <d3dx10math.h>
#include <d3dx11async.h>
#include <d3dx11effect.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Effects11d.lib")



//ImGui
#include <ImGui_New/imgui.h>
#include <ImGui_New/imgui_impl_dx11.h>
#include <ImGui_New/imgui_impl_win32.h>
#pragma comment(lib, "ImGui_New/imgui.lib")

//DirectXTex
#include <DirectXTex.h>
#pragma comment(lib, "directxtex.lib")

using namespace std;
using namespace Microsoft::WRL; // included ComPtr namespace

#define SafeRelease(p){ if(p){ (p)->Release(); (p) = NULL; } }
#define SafeDelete(p){ if(p){ delete (p); (p) = NULL; } }
#define SafeDeleteArray(p){ if(p){ delete [] (p); (p) = NULL; } }

typedef D3DXCOLOR Color;
typedef D3DXVECTOR2 Vector2;
typedef D3DXVECTOR3 Vector3;
typedef D3DXVECTOR4 Vector4;
typedef D3DXMATRIX Matrix;
typedef D3DXPLANE Plane;
typedef D3DXQUATERNION Quaternion;


#define Check(hr) { assert(SUCCEEDED(hr)); }
#define Super __super

#define __FILENAME__ ((strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

//#define my_assert(message , expression) \
//{ \
//	std::wstring str = L"Asseration in File : ";\
//	str += String::ToWString(__FILENAME__);\
//	str += L" at : ";\
//	str += std::to_wstring(__LINE__);\
//	str += L"\ncause is "; \
//	str += message; \
//	if(!(bool)(expression)) \
//	{ \
//		int msgBoxID = MessageBox(D3D::GetHandle(),str.c_str(),L"error", MB_ICONERROR | MB_ABORTRETRYIGNORE); \
//		switch(msgBoxID) \
//		{\
//			case IDABORT : \
//				std::abort();\
//				break;\
//			default: \
//				break; \
//		} \
//	} } 
//// my_assert
//




#include "Systems/IExecute.h"
#include "Systems/D3D.h"
#include "Systems/Gui.h"
#include "Systems/Keyboard.h"
#include "Systems/Mouse.h"
#include "Systems/Time.h"

#include "Viewer/Viewport.h"
#include "Viewer/Projection.h"
#include "Viewer/Perspective.h"
#include "Viewer/Orthographic.h"
#include "Viewer/Camera.h"
#include "Viewer/Freedom.h"
#include "Viewer/RenderTarget.h"
#include "Viewer/DepthStencil.h"
#include "Viewer/Fixity.h"
#include "Viewer/Spin.h"

#include "Renders/Shader.h"
#include "Renders/Texture.h"
#include "Renders/VertexLayouts.h"
#include "Renders/Context.h"
#include "Renders/DebugLine.h"
#include "Renders/Buffers.h"
#include "Renders/Transform.h"
#include "Renders/Lighting.h"
#include "Renders/PerFrame.h"
#include "Renders/Renderer.h"
#include "Renders/Material.h"
#include "Renders/Render2D.h"

#include "Meshes/Mesh.h"

#include "Utilities/Math.h"
#include "Utilities/String.h"
#include "Utilities/Path.h"
#include "Utilities/AssertMessage.h"

