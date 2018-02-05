//
@author : Jeeno
@emial : jeenocruise@gmail.com
//
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <xnamath.h>
#include <dxErr.h>
#include <dinput.h>
#include <D3D10_1.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment (lib, "DXErr.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

//Define variables/constants//
LPCTSTR WndClassName = L"firstwindow";    //Define our window class name
HWND hwnd = NULL;    //Sets our windows handle to NULL
HRESULT hr;

const int Width = 800;    //window width
const int Height = 600;    //window height

//阴影深度图宽、高
const int TextureWidth = 800;
const int TextureHeight = 600;
//灯光摄像机视截体
const float SCREEN_FAR = 1000.0f;  //视截体远裁面
const float SCREEN_NEAR = 1.0f;  //视截体近裁面

IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;

D3D11_VIEWPORT viewport;
ID3D11Buffer* squareIndexBuffer;
ID3D11Buffer* squareVertBuffer;
ID3D11Buffer* landIndexBuffer;
ID3D11Buffer* landVertBuffer;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D11PixelShader* PS_Shadow;	//阴影像素着色器
ID3D10Blob* VS_Buffer;			//顶点着色器缓冲
ID3D10Blob* PS_Buffer;			//像素着色器缓冲
ID3D10Blob* PS_Shadow_Buffer;	//阴影-像素着色器缓冲
ID3D11VertexShader* ShadowMap_VS;	//阴影贴图顶点着色器
ID3D11PixelShader* ShadowMap_PS;	//阴影贴图像素着色器
ID3D10Blob* ShadowMap_VS_Buffer;	//阴影贴图顶点缓冲
ID3D10Blob* ShadowMap_PS_Buffer;	//阴影贴图像素缓冲
ID3D11InputLayout* vertLayout;
ID3D11Buffer* cbPerObjectBuffer;
ID3D11Buffer* cbPerFrameBuffer;
ID3D11Buffer* cbLightBuffer;
ID3D11RasterizerState* WireFrame;			//线框
ID3D11ShaderResourceView* CubesTexture;		//立方体纹理
ID3D11ShaderResourceView* LandTexture;		//底面纹理
ID3D11SamplerState* CubesTexSamplerState;	//立方体纹理采样方式
ID3D11SamplerState* LandTexSamplerState;	//底面纹理采样方式
ID3D11SamplerState* ShadowTexSamplerState;	//阴影纹理采样方式
IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;


bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitScene();
void UpdateScene();
void DrawScene();
void resetViewPort();	//重设视口属性

//变换矩阵
XMMATRIX WVP;
//（相机）模型、观察、投影变换矩阵
XMMATRIX cube2World;	//立方体中心模版矩阵
XMMATRIX camView;
XMMATRIX camProjection;
//（光源）变换矩阵
XMMATRIX lightWorld;
XMMATRIX lightView;
XMMATRIX lightProjection;

//相机参数
XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;

XMMATRIX Rotation;			//旋转矩阵
XMMATRIX Scale;				//伸缩矩阵
XMMATRIX Translation;		//平移矩阵
float rot = 0.01f;			//旋转角度
XMVECTOR camPositionBase = XMVectorSet(0.0f, 3.0f, -8.0f, 0.0f);

DIMOUSESTATE mouseLastState;
LPDIRECTINPUT8 DirectInput;

//阴影贴图相关参数
ID3D11ShaderResourceView* mShaderResourceView;  //Shader资源视图  
ID3D11Texture2D* mDepthStencilTexture;
ID3D11DepthStencilView* mDepthStencilView;

float rotx = 0;			//世界
float roty = 0;
float rotz = 0;
float rotlightx = 0;	//光源旋转角度
float rotlighty = D3DX_PI/4;	//光源初始绕y轴旋转角
float scaleX = 1.0f;
float scaleY = 1.0f;

XMMATRIX Rotationx;
XMMATRIX Rotationy;
XMMATRIX lightRotationy;

//Functions//
bool InitializeWindow(HINSTANCE hInstance,    //Initialize our window
	int ShowWnd,
	int width, int height,
	bool windowed);

int messageloop();    //程序主循环过程
bool InitDirectInput(HINSTANCE hInstance);
void DetectInput();	//对用户的操作输入进行处理

LRESULT CALLBACK WndProc(HWND hWnd,    //Windows callback procedure
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

struct cbPerObject
{
	XMMATRIX WVP;			//观察相机模型*观察*投影矩阵
	XMMATRIX World;			//相机坐标
	XMMATRIX lightPos;
	XMMATRIX ProjectorView;	//光源处辅助相机观察矩阵
	XMMATRIX ProjectorProj; //光源处辅助相机投影矩阵
};

struct cbLightPerFrame{
	XMMATRIX  LIGHT_WVP;
};

struct Light
{
	Light()
	{
		ZeroMemory(this, sizeof(Light));
	}
	XMFLOAT3 dir;		//光线方向
	float cone;			//聚光灯的锥	
	XMFLOAT3 pos;		//光源位置
	float range;		//光源照范围
	XMFLOAT3 att;		//衰减系数
	float pad2;
	XMFLOAT4 ambient;	//环境光
	XMFLOAT4 diffuse;	//散射光
};

//顶点结构
struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z,
		float u, float v,
		float nx, float ny, float nz)
		: pos(x, y, z), texCoord(u, v), normal(nx, ny, nz){}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
};

Light light;
Light light_base;
struct cbPerFrame
{
	Light  light;
	//XMMATRIX world;
	//XMMATRIX view;
};
cbPerObject cbPerObj;
cbLightPerFrame	cbLight;
cbPerFrame constbuffPerFrame;

Vertex land[] = { //加上一个小值0.001来防止阴影中贴合面不该有的白横线
	Vertex(50.0f, -1.00f, 50.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	Vertex(50.0f, -1.00f, -50.0f, 0.0f, 10.0f, 0.0f, 1.0f, 0.0f),
	Vertex(-50.0f, -1.00f, -50.0f, 10.0f, 10.0f, 0.0f, 1.0f, 0.0f),
	Vertex(-50.0f, -1.00f, 50.0f, 10.0f, 0.0f, 0.0f, 1.0f, 0.0f),
};

DWORD land_indices[] = {
	0, 1, 2,
	0, 2, 3
};


//Create the vertex buffer
//放在地面上的立方体
Vertex v[] =
{
	// Front Face
	Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, -1.0f, -1.0f),
	Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f),
	Vertex(1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f),
	Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f),

	// Back Face
	Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f),
	Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f),
	Vertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
	Vertex(-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 1.0f),

	// Top Face
	Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, -1.0f),
	Vertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),
	Vertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f),

	// Bottom Face
	Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),
	Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
	Vertex(1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f),
	Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 1.0f),

	// Left Face
	Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f),
	Vertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),
	Vertex(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, -1.0f),
	Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),

	// Right Face
	Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
	Vertex(1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f),
	Vertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f),
};

//悬浮立方体1
//Vertex v[] =
//{
//	// Front Face
//	Vertex(-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f, -1.0f, -1.0f),
//	Vertex(-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f),
//	Vertex(0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f),
//	Vertex(0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f),
//
//	// Back Face
//	Vertex(-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f),
//	Vertex(0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f),
//	Vertex(0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
//	Vertex(-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 1.0f, 1.0f),
//
//	// Top Face
//	Vertex(-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 1.0f, -1.0f),
//	Vertex(-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),
//	Vertex(0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
//	Vertex(0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f),
//
//	// Bottom Face
//	Vertex(-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),
//	Vertex(0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
//	Vertex(0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f),
//	Vertex(-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, -1.0f, -1.0f, 1.0f),
//
//	// Left Face
//	Vertex(-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f),
//	Vertex(-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),
//	Vertex(-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, -1.0f, 1.0f, -1.0f),
//	Vertex(-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f),
//
//	// Right Face
//	Vertex(0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
//	Vertex(0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f),
//	Vertex(0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
//	Vertex(0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f),
//};

DWORD indices[] = {
	// Front Face
	0, 1, 2,
	0, 2, 3,

	// Back Face
	4, 5, 6,
	4, 6, 7,

	// Top Face
	8, 9, 10,
	8, 10, 11,

	// Bottom Face
	12, 13, 14,
	12, 14, 15,

	// Left Face
	16, 17, 18,
	16, 18, 19,

	// Right Face
	20, 21, 22,
	20, 22, 23
};



D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
D3D11_INPUT_ELEMENT_DESC shadow_in_layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{

	if (!InitializeWindow(hInstance, nShowCmd, Width, Height, true)) //初始化窗口
	{
		MessageBox(0, L"窗口初始化过程出错",
			L"错误", MB_OK);
		return 0;
	}

	if (!InitializeDirect3d11App(hInstance))    //初始化Direct3D
	{
		MessageBox(0, L"Direct3D初始化失败",
			L"错误", MB_OK);
		return 0;
	}

	if (!InitScene())    //初始化场景
	{
		MessageBox(0, L"场景初始化失败",
			L"错误", MB_OK);
		return 0;
	}

	if (!InitDirectInput(hInstance))	//初始化用户操作输入
	{
		MessageBox(0, L"初始化用户操作输入失败",
			L"错误", MB_OK);
		return 0;
	}

	messageloop();	//进入主循环

	ReleaseObjects();

	return 0;
}

bool InitializeWindow(HINSTANCE hInstance,    //Initialize our window
	int ShowWnd,
	int width, int height,
	bool windowed)
{
	//Start creating the window//

	WNDCLASSEX wc;    //Create a new extended windows class

	wc.cbSize = sizeof(WNDCLASSEX);    //Size of our windows class
	wc.style = CS_HREDRAW | CS_VREDRAW;    //class styles
	wc.lpfnWndProc = WndProc;    //Default windows procedure function
	wc.cbClsExtra = NULL;    //Extra bytes after our wc structure
	wc.cbWndExtra = NULL;    //Extra bytes after our windows instance
	wc.hInstance = hInstance;    //Instance to current application
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);    //Title bar Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);    //Default mouse Icon
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);    //Window bg color
	wc.lpszMenuName = NULL;    //Name of the menu attached to our window
	wc.lpszClassName = WndClassName;    //Name of our windows class
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO); //Icon in your taskbar

	if (!RegisterClassEx(&wc))    //Register our windows class
	{
		//if registration failed, display error
		MessageBox(NULL, L"注册窗口类过程出错",
			L"错误", MB_OK | MB_ICONERROR);
		return 1;
	}

	hwnd = CreateWindowEx(    //Create our Extended Window
		NULL,    //Extended style
		WndClassName,    //Name of our windows class
		L"三维场景",    //Name in the title bar of our window
		WS_OVERLAPPEDWINDOW,    //style of our window
		CW_USEDEFAULT, CW_USEDEFAULT,    //Top left corner of window
		width,    //Width of our window
		height,    //Height of our window
		NULL,    //Handle to parent window
		NULL,    //Handle to a Menu
		hInstance,    //Specifies instance of current program
		NULL    //used for an MDI client window
		);

	if (!hwnd)    //Make sure our window has been created
	{
		//If not, display error
		MessageBox(NULL, L"创建窗口出错",
			L"错误", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);    //Shows our window
	UpdateWindow(hwnd);    //Its good to update our window

	return true;    //if there were no errors, return true
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	//光源属性；
	//light.pos = XMFLOAT3(0.0f, 2.0f, -2.0f);
	//light.dir = XMFLOAT3(0.0f, -1.0f,8.0f);	
	light.pos = XMFLOAT3(0.0f, 3.0f, -8.0f);
	light.dir = XMFLOAT3(0.0f, -3.0f, 8.0f);
	//light.pos = XMFLOAT3(3.0f, 6.0f, -8.0f);
	//light.dir = XMFLOAT3(-3.0f, -6.0f, 8.0f);
	//光源观看方向不能平行y轴方向，不然会与lookat函数中的up方向（0,1,0）冲突，而不显示阴影；
	/*light.pos = XMFLOAT3(0.0f, 3.0f, -1.0f);
	light.dir = XMFLOAT3(0.0f, -3.0f, 1.0f);*/
	

	light.range = 1000.0f;
	light.cone = 10.0f;
	light.att = XMFLOAT3(0.4f, 0.02f, 0.000f);
	light.ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	light.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	light_base = light;
	/*light.att = XMFLOAT3(0.0f, 0.08f, 0.0f);
	light.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	light.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);*/

	//Describe our Buffer
	DXGI_MODE_DESC bufferDesc;

	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = Width;
	bufferDesc.Height = Height;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//Describe our SwapChain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 4;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	//创建交换链
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

	//创建后台缓冲
	ID3D11Texture2D* BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

	//创建渲染目标
	hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
	BackBuffer->Release();

	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("d3d11Device->CreateRenderTargetView"), MB_OK);
		return 0;
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.Width = Width;
	depthStencilDesc.Height = Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 4;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	//Create the Depth/Stencil View
	d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
	hr = d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);
	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("d3d11Device->CreateRenderTargetView"), MB_OK);
		return 0;
	}

	//第一,填充深度视图的2D纹理形容结构体,并创建2D渲染纹理  
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	depthBufferDesc.Width = TextureWidth;
	depthBufferDesc.Height = TextureHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; //24位是为了深度缓存，8位是为了模板缓存  
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;  //注意深度缓存(纹理)的绑定标志  
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;
	hr = (d3d11Device->CreateTexture2D(&depthBufferDesc, NULL, &mDepthStencilTexture));
	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("d3d11Device->CreateTexture2D"), MB_OK);
		return 0;
	}

	//第二,填充深度缓存视图形容结构体,并创建深度缓存视图  
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	hr = (d3d11Device->CreateDepthStencilView(mDepthStencilTexture, &depthStencilViewDesc, &mDepthStencilView));
	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("d3d11Device->CreateDepthStencilView"), MB_OK);
		return 0;
	}

	//第三,填充着色器资源视图形容体,并进行创建着色器资源视图,注意这是用深度缓存(纹理)来创建的，而不是渲染目标缓存(纹理)创建的  
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; //此时因为是仅仅进行深度写，而不是颜色写，所以此时Shader资源格式跟深度缓存是一样的  
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = depthBufferDesc.MipLevels;
	hr = (d3d11Device->CreateShaderResourceView(mDepthStencilTexture, &shaderResourceViewDesc, &mShaderResourceView));
	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("d3d11Device->CreateShaderResourceView"), MB_OK);
		return 0;
	}



	return true;
}

bool InitDirectInput(HINSTANCE hInstance)
{
	hr = DirectInput8Create(hInstance,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&DirectInput,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysKeyboard,
		&DIKeyboard,
		NULL);

	/*hr = DirectInput->CreateDevice(GUID_SysMouse,
	&DIMouse,
	NULL);*/

	hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	hr = DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	/*hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	hr = DIMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);*/

	return true;
}

void DetectInput()
{
	//DIMOUSESTATE mouseCurrState;

	BYTE keyboardState[256];

	DIKeyboard->Acquire();
	//DIMouse->Acquire();

	//DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);

	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	if (keyboardState[DIK_ESCAPE] & 0x80)
		PostMessage(hwnd, WM_DESTROY, 0, 0);

	if (keyboardState[DIK_LEFT] & 0x80)
	{
		roty -= D3DX_PI / 36000;
	}
	if (keyboardState[DIK_RIGHT] & 0x80)
	{
		roty += D3DX_PI / 36000;
	}
	if (keyboardState[DIK_UP] & 0x80)
	{
		rotx -= D3DX_PI / 36000;
	}
	if (keyboardState[DIK_DOWN] & 0x80)
	{
		rotx += D3DX_PI / 36000;
	}
	if (keyboardState[DIK_A] & 0x80)
	{
		rotlighty += D3DX_PI / 36000;
	}
	if (keyboardState[DIK_D] & 0x80)
	{
		rotlighty -= D3DX_PI / 36000;
	}

	if (rotx >= 2 * D3DX_PI)
		rotx = 0;
	else if (rotx < 0)
		rotx = D3DX_PI * 2 + rotx;

	if (roty >= D3DX_PI * 2)
		roty = 0;
	else if (roty < 0)
		roty = D3DX_PI * 2 + roty;

	if (rotlighty >= D3DX_PI * 2)
		rotlighty = 0;
	else if (rotlighty < 0)
		rotlighty = D3DX_PI * 2 + rotlighty;

	return;
}

void ReleaseObjects()
{
	//Release the COM Objects we created
	SwapChain->Release();
	d3d11Device->Release();
	d3d11DevCon->Release();

	squareVertBuffer->Release();
	squareIndexBuffer->Release();
	landVertBuffer->Release();
	landIndexBuffer->Release();
	VS->Release();
	PS->Release();
	PS_Shadow->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	PS_Shadow_Buffer->Release();
	ShadowMap_PS->Release();
	ShadowMap_PS_Buffer->Release();
	ShadowMap_VS->Release();
	ShadowMap_VS_Buffer->Release();
	vertLayout->Release();
	depthStencilBuffer->Release();
	depthStencilView->Release();
	mDepthStencilView->Release();
	mDepthStencilTexture->Release();
	mShaderResourceView->Release();
	cbPerObjectBuffer->Release();
	WireFrame->Release();
	cbPerFrameBuffer->Release();

	DIKeyboard->Unacquire();
	//DIMouse->Unacquire();
	DirectInput->Release();


}


bool InitScene()
{
	//lightRotationy = XMMatrixIdentity();

	//Compile Shaders from shader file
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_4_0", 0, 0, 0, &PS_Buffer, 0, 0);
	hr = D3DX11CompileFromFile(L"shadowmap.fx", 0, 0, "VS", "vs_4_0", 0, 0, 0, &ShadowMap_VS_Buffer, 0, 0);
	hr = D3DX11CompileFromFile(L"shadowmap.fx", 0, 0, "PS", "ps_4_0", 0, 0, 0, &ShadowMap_PS_Buffer, 0, 0);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS_Shadow", "ps_4_0", 0, 0, 0, &PS_Shadow_Buffer, 0, 0);

	//hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "D2D_PS", "ps_4_0", 0, 0, 0, &PS_Shadow_Buffer, 0, 0);	//不加任何效果的像素着色器

	//Create the Shader Objects
	hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);
	hr = d3d11Device->CreateVertexShader(ShadowMap_VS_Buffer->GetBufferPointer(), ShadowMap_VS_Buffer->GetBufferSize(), NULL, &ShadowMap_VS);
	hr = d3d11Device->CreatePixelShader(ShadowMap_PS_Buffer->GetBufferPointer(), ShadowMap_PS_Buffer->GetBufferSize(), NULL, &ShadowMap_PS);
	hr = d3d11Device->CreatePixelShader(PS_Shadow_Buffer->GetBufferPointer(), PS_Shadow_Buffer->GetBufferSize(), NULL, &PS_Shadow);

	if (FAILED(hr))
	{
		MessageBox(NULL, DXGetErrorDescription(hr),
			TEXT("编译/生成shader对象过程出错"), MB_OK);
		return 0;
	}

	//设置立方体索引缓冲
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(indices);
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	ZeroMemory(&iinitData, sizeof(iinitData));
	iinitData.pSysMem = indices;
	d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &squareIndexBuffer);


	//设置立方体顶点缓冲
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(v);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = v;
	hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &squareVertBuffer);



	//设置底面索引缓冲
	D3D11_BUFFER_DESC landindexBufferDesc;
	ZeroMemory(&landindexBufferDesc, sizeof(landindexBufferDesc));

	landindexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	landindexBufferDesc.ByteWidth = sizeof(land_indices);
	landindexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	landindexBufferDesc.CPUAccessFlags = 0;
	landindexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData2;

	ZeroMemory(&iinitData2, sizeof(iinitData2));
	iinitData2.pSysMem = land_indices;
	d3d11Device->CreateBuffer(&landindexBufferDesc, &iinitData2, &landIndexBuffer);


	//设置底面顶点缓冲
	D3D11_BUFFER_DESC landvertexBufferDesc;
	ZeroMemory(&landvertexBufferDesc, sizeof(landvertexBufferDesc));

	landvertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	landvertexBufferDesc.ByteWidth = sizeof(land);
	landvertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	landvertexBufferDesc.CPUAccessFlags = 0;
	landvertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA landvertexBufferData;

	ZeroMemory(&landvertexBufferData, sizeof(landvertexBufferData));
	landvertexBufferData.pSysMem = land;
	hr = d3d11Device->CreateBuffer(&landvertexBufferDesc, &landvertexBufferData, &landVertBuffer);


	//Set Primitive Topology
	d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//设定视口属性
	resetViewPort();

	//Effects.fx中的常量缓冲cbPerObject描述
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

	//Effects.fx中的常量缓冲cbPerFrame描述
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerFrame);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerFrameBuffer);

	//shadowmap.fx中的常量缓冲cbLightPerFrame描述
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbLightPerFrame);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbLightBuffer);

	//加载纹理文件
	hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"timg.jpg",
		NULL, NULL, &CubesTexture, NULL);
	hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"earth.jpg",
		NULL, NULL, &LandTexture, NULL);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.BorderColor[0] = 0;
	sampDesc.BorderColor[1] = 0;
	sampDesc.BorderColor[2] = 0;
	sampDesc.BorderColor[3] = 0;
	//sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = d3d11Device->CreateSamplerState(&sampDesc, &CubesTexSamplerState);	//立方体纹理采样方式
	hr = d3d11Device->CreateSamplerState(&sampDesc, &LandTexSamplerState);	//底面纹理采样方式

	D3D11_SAMPLER_DESC shadowsamplerDesc;
	shadowsamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  //都是线性插值(三种方式,点过滤,线性过滤,异性过滤)
	shadowsamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	shadowsamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	shadowsamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	shadowsamplerDesc.MipLODBias = 0.0f;
	shadowsamplerDesc.MaxAnisotropy = 1;
	shadowsamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	shadowsamplerDesc.BorderColor[0] = 0;
	shadowsamplerDesc.BorderColor[1] = 0;
	shadowsamplerDesc.BorderColor[2] = 0;
	shadowsamplerDesc.BorderColor[3] = 0;
	shadowsamplerDesc.MinLOD = 0;
	shadowsamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	shadowsamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowsamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowsamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = d3d11Device->CreateSamplerState(&shadowsamplerDesc, &ShadowTexSamplerState);	//立方体纹理采样方式

	//相机参数
	camPosition = XMVectorSet(0.0f, 3.0f, -8.0f, 0.0f);
	camTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//设置视图矩阵
	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);

	//设置投影矩阵
	camProjection = XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)Width / Height, 1.0f, 1000.0f);

	//光栅化阶段设置(线框/实心填充)
	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_SOLID;
	wfdesc.CullMode = D3D11_CULL_NONE;
	hr = d3d11Device->CreateRasterizerState(&wfdesc, &WireFrame);
	d3d11DevCon->RSSetState(WireFrame);

	return true;
}


void UpdateScene()
{
	XMVECTOR rotyaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR rotzaxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR rotxaxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	Rotation = XMMatrixRotationAxis(rotzaxis, rotz);
	Rotationx = XMMatrixRotationAxis(rotxaxis, rotx);
	Rotationy = XMMatrixRotationAxis(rotyaxis, roty);

	//Set cube1's world space using the transformations
	cube2World = Rotation * Rotationx * Rotationy;

	//点光源位置
	//XMVECTOR lightVector = XMVectorSet(0.0f, 0.0f, 0.0f, -10.0f);
	Scale = XMMatrixScaling(1.f, 1.f, 1.f);

	//Set cube2's world space matrix
	cube2World = cube2World * Scale;

	lightRotationy = XMMatrixRotationAxis(rotyaxis, rotlighty);
	light.pos.x = light_base.pos.x*lightRotationy._11 + light_base.pos.y*lightRotationy._21 + light_base.pos.z*lightRotationy._31 + lightRotationy._41;
	light.pos.y = light_base.pos.x*lightRotationy._12 + light_base.pos.y*lightRotationy._22 + light_base.pos.z*lightRotationy._32 + lightRotationy._42;
	light.pos.z = light_base.pos.x*lightRotationy._13 + light_base.pos.y*lightRotationy._23 + light_base.pos.z*lightRotationy._33 + lightRotationy._43;
	light.dir = { -light.pos.x, -light.pos.y, -light.pos.z };
}

//渲染ShadowMap,不存在颜色写，仅仅是深度写
bool renderShadowMap(){
	//清空原有值
	float bgColor[4] = { (0.0f, 0.0f, 0.0f, 1.0f) };
	d3d11DevCon->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMVECTOR lightPos = XMVectorSet(light.pos.x, light.pos.y, light.pos.z, 0.0f);
	lightWorld = XMMatrixIdentity();										//世界坐标
	lightView = XMMatrixLookAtLH(lightPos, camTarget, camUp);				//光源照相机观察变换矩阵
	lightProjection = XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)TextureWidth / TextureHeight, SCREEN_NEAR, SCREEN_FAR);
	XMMATRIX WVP_Light = lightWorld * lightView * lightProjection;			//光源坐标下的：模型*视图*投影矩阵
	cbLight.LIGHT_WVP = XMMatrixTranspose(WVP_Light);						//光源投影变换矩阵转置
	d3d11DevCon->UpdateSubresource(cbLightBuffer, 0, NULL, &cbLight, 0, 0);	//更新shadowmap.fx中的常量缓冲cbLightPerFrame
	d3d11DevCon->VSSetConstantBuffers(0, 1, &cbLightBuffer);

	//设置视口的属性  
	viewport.Width = (float)TextureWidth;
	viewport.Height = (float)TextureHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	d3d11DevCon->RSSetViewports(1, &viewport);

	//创建并设置顶点输入布局
	d3d11Device->CreateInputLayout(shadow_in_layout, ARRAYSIZE(shadow_in_layout), ShadowMap_VS_Buffer->GetBufferPointer(),
		ShadowMap_VS_Buffer->GetBufferSize(), &vertLayout);
	d3d11DevCon->IASetInputLayout(vertLayout);
	//设置当前的顶点着色器和像素着色器
	d3d11DevCon->VSSetShader(ShadowMap_VS, 0, 0);
	d3d11DevCon->PSSetShader(ShadowMap_PS, 0, 0);


	//设置渲染目标
	ID3D11RenderTargetView* renderTarget[1] = { 0 };
	d3d11DevCon->OMSetRenderTargets(1, renderTarget, mDepthStencilView);

	//设置顶点缓冲区
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	//渲染立方体
	d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);
	d3d11DevCon->DrawIndexed(sizeof(indices) / sizeof(DWORD), 0, 0);
	//渲染底面
	d3d11DevCon->IASetIndexBuffer(landIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	d3d11DevCon->IASetVertexBuffers(0, 1, &landVertBuffer, &stride, &offset);
	d3d11DevCon->DrawIndexed(sizeof(land_indices) / sizeof(DWORD), 0, 0);


	return true;
}

bool renderScene(){
	//后台缓冲区所有涂黑
	float bgColor[4] = { (0.0f, 0.0f, 0.0f, 0.0f) };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);					//清除后台缓冲
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);	//清除深度缓冲和模板缓冲，设置每帧初始值

	//投影相机变换矩阵
	WVP = cube2World * camView * camProjection; //模型*视图*投影矩阵
	cbPerObj.WVP = XMMatrixTranspose(WVP);      //变换矩阵转置
	cbPerObj.World = XMMatrixTranspose(XMMatrixTranslationFromVector(camPosition));	//相机坐标转置
	cbPerObj.lightPos = XMMatrixTranspose(XMMatrixTranslationFromVector(XMVectorSet(light.pos.x, light.pos.y, light.pos.z, 1.0f)));	//光源位置转置
	cbPerObj.ProjectorView = XMMatrixTranspose(lightView);		//光源视图矩阵转置
	cbPerObj.ProjectorProj = XMMatrixTranspose(lightProjection);	//光源投影矩阵转置
	d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);	//更新常量缓冲cbPerObj
	d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);

	constbuffPerFrame.light = light;

	d3d11DevCon->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &constbuffPerFrame, 0, 0);
	d3d11DevCon->PSSetConstantBuffers(1, 1, &cbPerFrameBuffer);

	//创建并设置顶点输入布局
	d3d11Device->CreateInputLayout(layout, ARRAYSIZE(layout), VS_Buffer->GetBufferPointer(),
		VS_Buffer->GetBufferSize(), &vertLayout);
	d3d11DevCon->IASetInputLayout(vertLayout);


	//设置渲染目标为后台缓冲
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	////设置当前的顶点着色器和像素着色器
	//d3d11DevCon->VSSetShader(VS, 0, 0);
	//d3d11DevCon->PSSetShader(PS, 0, 0);

	////正常场景（物体：立方体）绘制
	//d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	////设定顶点缓冲区
	//d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);
	//d3d11DevCon->PSSetShaderResources(0, 1, &CubesTexture);
	//d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);
	//d3d11DevCon->DrawIndexed(sizeof(indices) / sizeof(DWORD), 0, 0);

	////正常场景（底面）绘制
	//	d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//	d3d11DevCon->IASetVertexBuffers(0, 1, &landVertBuffer, &stride, &offset);
	//	d3d11DevCon->PSSetShaderResources(0, 1, &LandTexture);
	//	d3d11DevCon->PSSetSamplers(0, 1, &LandTexSamplerState);
	//	d3d11DevCon->DrawIndexed(sizeof(indices) / sizeof(DWORD), 0, 0);

	//阴影绘制（立方体）
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS_Shadow, 0, 0);
	d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);
	d3d11DevCon->PSSetShaderResources(0, 1, &CubesTexture);
	d3d11DevCon->PSSetShaderResources(1, 1, &mShaderResourceView);
	d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);
	d3d11DevCon->PSSetSamplers(1, 1, &ShadowTexSamplerState);
	d3d11DevCon->DrawIndexed(sizeof(indices) / sizeof(DWORD), 0, 0);
	
	//阴影绘制（底面）
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS_Shadow, 0, 0);
	d3d11DevCon->IASetIndexBuffer(landIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	d3d11DevCon->IASetVertexBuffers(0, 1, &landVertBuffer, &stride, &offset);
	d3d11DevCon->PSSetShaderResources(0, 1, &LandTexture);
	d3d11DevCon->PSSetShaderResources(1, 1, &mShaderResourceView);
	d3d11DevCon->PSSetSamplers(0, 1, &LandTexSamplerState);
	d3d11DevCon->PSSetSamplers(1, 1, &ShadowTexSamplerState);
	d3d11DevCon->DrawIndexed(sizeof(land_indices) / sizeof(DWORD), 0, 0);

	/*Present the backbuffer to the screen*/
	SwapChain->Present(0, 0);

	return true;
}

void DrawScene()
{
	//阴影贴图渲染
	if (!renderShadowMap()){
		MessageBox(NULL, L"生成阴影贴图失败!", NULL, MB_OK);
		return;
	}

	//渲染场景
	if (!renderScene()){
		MessageBox(NULL, L"场景渲染失败!", NULL, MB_OK);
		return;
	}
}



int messageloop(){    //The message loop

	MSG msg;    //Create a new message structure
	ZeroMemory(&msg, sizeof(MSG));    //clear message structure to NULL

	while (true)
	{
		//消息处理
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)    //“退出”消息
				break;    //跳出住循环

			TranslateMessage(&msg);    //Translate the message

			//Send the message to default windows procedure
			DispatchMessage(&msg);
		}
		else{
			DetectInput();
			UpdateScene();
			DrawScene();
		}


	}

	return (int)msg.wParam;        //return the message

}

LRESULT CALLBACK WndProc(HWND hwnd,    //Default windows procedure
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)    //Check message
	{

	case WM_KEYDOWN:    //For a key down
		//if escape key was pressed, display popup box
		if (wParam == VK_ESCAPE){
			if (MessageBox(0, L"确定退出？",
				L"确定?", MB_YESNO | MB_ICONQUESTION) == IDYES)

				//Release the windows allocated memory  
				DestroyWindow(hwnd);
		}
		return 0;
	case WM_DESTROY:    //if x button in top right was pressed
		PostQuitMessage(0);
		return 0;
	}
	//return the message for windows to handle it
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

void resetViewPort(){
	//重设设置视口的属性  
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Width = (float)TextureWidth;
	viewport.Height = (float)TextureHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	d3d11DevCon->RSSetViewports(1, &viewport);
}