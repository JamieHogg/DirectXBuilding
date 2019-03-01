
//Include and link appropriate libraries and headers//
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <xnamath.h>
#include <dinput.h>
#include <iostream>
#include <fstream>	//for outputting to file
#include <AntTweakBar.h>

//Global Declarations - Interfaces//
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;
ID3D11Buffer* squareIndexBuffer;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;
ID3D11Buffer* squareVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;
ID3D11Buffer* cbPerObjectBuffer;

ID3D11ShaderResourceView* CubesTexture;
ID3D11ShaderResourceView* GlassTexture;
ID3D11SamplerState* CubesTexSamplerState;
IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

DIMOUSESTATE mouseLastState;	//mouse state structure
LPDIRECTINPUT8 DirectInput;		//keyboard input structure

//Global Declarations - Others//
LPCTSTR WndClassName = L"firstwindow";
HWND hwnd = NULL;
HRESULT hr;

const int Width  = 900;		//window width and height
const int Height = 900;


float rotx = 0;		//used for adjusting cube
float rotz = 0;
float scaleX = 1.0f;
float scaleY = 1.0f;

XMMATRIX Rotationx;
XMMATRIX Rotationz;

XMMATRIX WVP;
XMMATRIX cube[1000];
XMMATRIX glass[10000];
XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;		//worldspace position
XMVECTOR camTarget;			//where the camera faces
XMVECTOR camUp;

XMVECTOR DeafultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);	//the forward and right direction in worldspace (used for camera rotation)
XMVECTOR DefaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR camForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);	//the forward and right directions of the camera
XMVECTOR camRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

float rot = 0.01f;
float camX = 0.0f;
float camY = 0.0f;
float camZ = -5.0f;

float moveLeftRight = 0.0f;		//used to move along the camera vectors when moving forward or right
float moveBackForward = 0.0f;
float moveUpDown = 0.0f;
float camYaw = 0.0f;	//rotation around x and y axis
float camPitch = 0.0f;

XMMATRIX Rotation;
XMMATRIX Scale;
XMMATRIX ScaleNew;
XMMATRIX Translation;

XMMATRIX camRotationMatrix;		//used to rotate camera
XMMATRIX grouncdWorld;	//worldspace of ground

double countsPerSecond = 0.0;
__int64 counterStart = 0;

int frameCount = 0;
int fps = 0;

__int64 frameTimeOld = 0;
double frameTime;

int cubeCount = 0;
int glassCount = 0;
int widthX = 9;
int widthZ = 5;
int height = 4;

bool windows = true;
bool roof = false;

int wallTexture = 1;

//Vertex Structure and Vertex Layout (Input Layout)//
struct Vertex	//Overloaded Vertex Structure
{
	Vertex() {}
	Vertex(float x, float y, float z,
		float u, float v, float nx, float ny, float nz)
		: pos(x, y, z), texCoord(u, v), normal(nx, ny, nz) {}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
};

//Function Prototypes//
bool InitializeDirect3d11App(HINSTANCE hInstance);
void CleanUp();
bool InitScene();
void UpdateScene(double time);
void ChangeTheScene();
void DrawScene();
void StartTimer();
double GetTime();
double GetFrameTime();
void WriteToFile(Vertex* vert, DWORD* indices, int vertexArraySize, int indexArraySize);

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width, int height,
	bool windowed);
int messageloop();

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);
void UpdateCamera();	//will be used for updating camera
int cubeCalculator();

LRESULT CALLBACK WndProc(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

//Create effects constant buffer's structure//
struct cbPerObject
{
	XMMATRIX  WVP;
};

cbPerObject cbPerObj;


D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
};
UINT numElements = ARRAYSIZE(layout);

int WINAPI WinMain(HINSTANCE hInstance,	//Main windows function
	HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine,
	int nShowCmd)
{

	if(!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
	{
		MessageBox(0, L"Window Initialization - Failed",
			L"Error", MB_OK);
		return 0;
	}

	if(!InitializeDirect3d11App(hInstance))	//Initialize Direct3D
	{
		MessageBox(0, L"Direct3D Initialization - Failed",
			L"Error", MB_OK);
		return 0;
	}

	if(!InitScene())	//Initialize our scene
	{
		MessageBox(0, L"Scene Initialization - Failed",
			L"Error", MB_OK);
		return 0;
	}

	if (!InitDirectInput(hInstance))
	{
		MessageBox(0, L"Direct Input Inititalisation - Failed", L"Error", MB_OK);
		return 0;
	}

	messageloop();

	CleanUp();    

	return 0;
}

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width, int height,
	bool windowed)
{
	typedef struct _WNDCLASS {
		UINT cbSize;
		UINT style;
		WNDPROC lpfnWndProc;
		int cbClsExtra;
		int cbWndExtra;
		HANDLE hInstance;
		HICON hIcon;
		HCURSOR hCursor;
		HBRUSH hbrBackground;
		LPCTSTR lpszMenuName;
		LPCTSTR lpszClassName;
	} WNDCLASS;

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WndClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class",	
			L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	hwnd = CreateWindowEx(
		NULL,
		WndClassName,
		L"Procedural Building Generation",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hwnd)
	{
		MessageBox(NULL, L"Error creating window",
			L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	//Describe our SwapChain Buffer
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
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hwnd; 
	swapChainDesc.Windowed = true;		//can use alt enter to change between fullscreen and windowed
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	//Create our SwapChain
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

	//Create our BackBuffer
	ID3D11Texture2D* BackBuffer;
	hr = SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void**)&BackBuffer );

	//Create our Render Target
	hr = d3d11Device->CreateRenderTargetView( BackBuffer, NULL, &renderTargetView );
	BackBuffer->Release();

	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width     = Width;
	depthStencilDesc.Height    = Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count   = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	//Create the Depth/Stencil View
	d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
	d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);

	//Set our Render Target
	d3d11DevCon->OMSetRenderTargets( 1, &renderTargetView, depthStencilView );

	TwInit(TW_DIRECT3D11, d3d11Device);
	TwWindowSize(Width, Height);
	TwBar* myBar;
	myBar = TwNewBar("UI");
	//ADD VARIABLES TO UI HERE, REMOVE THIS
	//TwAddVarRW(myBar, "Height", TW_TYPE_INT32, &height, "min = 3 max = 10");
	TwAddVarRW(myBar, "WidthX", TW_TYPE_INT32, &widthX, "min = 7 max = 11 step = 2");
	TwAddVarRW(myBar, "WidthZ", TW_TYPE_INT32, &widthZ, "min = 5 max = 11 step = 2");
	TwAddVarRW(myBar, "Wall Texture", TW_TYPE_INT32, &wallTexture, "min = 1 max = 3");
	TwAddVarRW(myBar, "Windows", TW_TYPE_BOOLCPP, &windows, "");
	TwAddVarRW(myBar, "Roof", TW_TYPE_BOOLCPP, &roof, "");

	return true;
}

void CleanUp()
{
	SwapChain->SetFullscreenState(false, NULL);
	PostMessage(hwnd, WM_DESTROY, 0, 0);

	//Release the COM Objects we created
	SwapChain->Release();
	d3d11Device->Release();
	d3d11DevCon->Release();
	renderTargetView->Release();
	squareVertBuffer->Release();
	squareIndexBuffer->Release();
	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();
	depthStencilView->Release();
	depthStencilBuffer->Release();
	cbPerObjectBuffer->Release();
	
	//Unacquire input
	DIKeyboard->Unacquire();
	DIMouse->Unacquire();
	DirectInput->Release();
}

bool InitScene()
{
	//Compile Shaders from shader file
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
	hr = D3DX11CompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_4_0", 0, 0, 0, &PS_Buffer, 0, 0);

	//Create the Shader Objects
	hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

	//Set Vertex and Pixel Shaders
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS, 0, 0);

	//Create the vertex buffer
	Vertex v[] =
	{
		// Front Face
		Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f),	//vertices make up each point and the indices show the order to be drawn
		Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f),
		Vertex( 1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f),
		Vertex( 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f),

		 //Back Face
		Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
		Vertex( 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f),
		Vertex( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
		Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),

		 //Top Face
		Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f),
		Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
		Vertex( 1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
		Vertex( 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f),

		// Bottom Face
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f),
		Vertex( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f),
		Vertex( 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f),
		Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f),

		// Left Face
		Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f),

		// Right Face
		Vertex( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f),
		Vertex( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		Vertex( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f),
		Vertex( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f)
	};

	Vertex stairs[] =
	{
		Vertex(-1, 0, -1, 0.335, 0.858571, 0, 0, -0.57735),
		Vertex(1, 0, -1, 0.668333, 0.858571, 0, 0, -0.57735),
		Vertex(1, -1, -1, 0.668333, 1, 0, 0, -0.57735),
		Vertex(-1, 0, -1, 0.335, 0.858571, 0, 0, -0.57735),
		Vertex(1, -1, -1, 0.668333, 1, 0, 0, -0.57735),
		Vertex(-1, -1, -1, 0.335, 1, 0, 0, -0.57735),
		Vertex(-1, 0, -1, 0.335, 0.858571, -0.57735, 0, -0),
		Vertex(-1, -1, -1, 0.335, 1, -0.57735, 0, -0),
		Vertex(-1, -1, 1, 0.003333, 1, -0.57735, 0, -0),
		Vertex(-1, -1, 1, 0.003333, 1, -0.57735, 0, -0),
		Vertex(-1, 0, -0, 0.188333, 0.858571, -0.57735, 0, -0),
		Vertex(-1, 0, -1, 0.335, 0.858571, -0.57735, 0, -0),
		Vertex(-1, -1, 1, 0.003333, 1, -0.57735, 0, -0),
		Vertex(-1, 1, 1, 0.003333, 0.719286, -0.57735, 0, -0),
		Vertex(-1, 0, -0, 0.188333, 0.858571, -0.57735, 0, -0),
		Vertex(-1, 1, 1, 0.003333, 0.719286, -0.57735, 0, -0),
		Vertex(-1, 1, -0, 0.188333, 0.717143, -0.57735, 0, -0),
		Vertex(-1, 0, -0, 0.188333, 0.858571, -0.57735, 0, -0),
		Vertex(-1, -1, 1, 0.668333, 0.43, 0, -0.57735, -0),
		Vertex(-1, -1, -1, 0.668333, 0.714286, 0, -0.57735, -0),
		Vertex(1, -1, -1, 0.335, 0.714286, 0, -0.57735, -0),
		Vertex(-1, -1, 1, 0.668333, 0.43, 0, -0.57735, -0),
		Vertex(1, -1, -1, 0.335, 0.714286, 0, -0.57735, -0),
		Vertex(1, -1, 1, 0.335, 0.43, 0, -0.57735, -0),
		Vertex(1, 1, 1, 0.996667, 0.719286, 0.57735, 0, -0),
		Vertex(1, -1, 1, 0.996667, 1, 0.57735, 0, -0),
		Vertex(1, -1, -1, 0.668333, 1, 0.57735, 0, -0),
		Vertex(1, -1, -1, 0.668333, 1, 0.57735, 0, -0),
		Vertex(1, 0, -0, 0.833333, 0.858571, 0.57735, 0, -0),
		Vertex(1, 1, 1, 0.996667, 0.719286, 0.57735, 0, -0),
		Vertex(1, 0, -0, 0.833333, 0.858571, 0.57735, 0, -0),
		Vertex(1, 1, -0, 0.833333, 0.717143, 0.57735, 0, -0),
		Vertex(1, 1, 1, 0.996667, 0.719286, 0.57735, 0, -0),
		Vertex(1, 0, -1, 0.668333, 0.858571, 0.57735, 0, -0),
		Vertex(1, 0, -0, 0.833333, 0.858571, 0.57735, 0, -0),
		Vertex(1, -1, -1, 0.668333, 1, 0.57735, 0, -0),
		Vertex(-1, 1, 1, 0.668333, 0.143571, 0, 0, 0.57735),
		Vertex(-1, -1, 1, 0.668333, 0.43, 0, 0, 0.57735),
		Vertex(1, -1, 1, 0.335, 0.43, 0, 0, 0.57735),
		Vertex(-1, 1, 1, 0.668333, 0.143571, 0, 0, 0.57735),
		Vertex(1, -1, 1, 0.335, 0.43, 0, 0, 0.57735),
		Vertex(1, 1, 1, 0.335, 0.143571, 0, 0, 0.57735),
		Vertex(1, 1, -0, 0.335, 0.00142902, 0, 0.57735, -0),
		Vertex(-1, 1, -0, 0.668333, 0.00142902, 0, 0.57735, -0),
		Vertex(-1, 1, 1, 0.668333, 0.143571, 0, 0.57735, -0),
		Vertex(1, 1, -0, 0.335, 0.00142902, 0, 0.57735, -0),
		Vertex(-1, 1, 1, 0.668333, 0.143571, 0, 0.57735, -0),
		Vertex(1, 1, 1, 0.335, 0.143571, 0, 0.57735, -0),
		Vertex(1, 1, -0, 0.335, 0.144286, 0, 0, -0.57735),
		Vertex(1, 0, -0, 0.335, 0.287143, 0, 0, -0.57735),
		Vertex(-1, 0, -0, 0.668333, 0.287143, 0, 0, -0.57735),
		Vertex(1, 1, -0, 0.335, 0.144286, 0, 0, -0.57735),
		Vertex(-1, 0, -0, 0.668333, 0.287143, 0, 0, -0.57735),
		Vertex(-1, 1, -0, 0.668333, 0.144286, 0, 0, -0.57735),
		Vertex(-1, 0, -1, 0.668333, 0.43, 0, 0.57735, -0),
		Vertex(-1, 0, -0, 0.668333, 0.287143, 0, 0.57735, -0),
		Vertex(1, 0, -0, 0.335, 0.287143, 0, 0.57735, -0),
		Vertex(-1, 0, -1, 0.668333, 0.43, 0, 0.57735, -0),
		Vertex(1, 0, -0, 0.335, 0.287143, 0, 0.57735, -0),
		Vertex(1, 0, -1, 0.335, 0.43, 0, 0.57735, -0)
	};

	DWORD indices[] = {
		// Front Face
		0,  1,  2,	//points to location of vertexes above
		0,  2,  3,

		// Back Face
		4,  5,  6,
		4,  6,  7,

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
	
	int vertexSize = sizeof(v) / sizeof(v[0]);
	int indexSize = sizeof(indices) / sizeof(indices[0]);

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory( &indexBufferDesc, sizeof(indexBufferDesc) );

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 12 * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	// creates indices
	iinitData.pSysMem = indices;

	d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &squareIndexBuffer);

	d3d11DevCon->IASetIndexBuffer( squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);


	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory( &vertexBufferDesc, sizeof(vertexBufferDesc) );

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof( Vertex ) * 24;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData; 

	ZeroMemory( &vertexBufferData, sizeof(vertexBufferData) );
	vertexBufferData.pSysMem = v;
	hr = d3d11Device->CreateBuffer( &vertexBufferDesc, &vertexBufferData, &squareVertBuffer);

	//Set the vertex buffer
	UINT stride = sizeof( Vertex );
	UINT offset = 0;
	d3d11DevCon->IASetVertexBuffers( 0, 1, &squareVertBuffer, &stride, &offset );

	//Create the Input Layout
	hr = d3d11Device->CreateInputLayout( layout, numElements, VS_Buffer->GetBufferPointer(), 
		VS_Buffer->GetBufferSize(), &vertLayout );

	//Set the Input Layout
	d3d11DevCon->IASetInputLayout( vertLayout );

	//Set Primitive Topology
	d3d11DevCon->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	//Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//Set the Viewport
	d3d11DevCon->RSSetViewports(1, &viewport);

	//Create the buffer to send to the cbuffer in effect file
	D3D11_BUFFER_DESC cbbd;	
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

	//Camera information
	//Initial camera position
	camPosition = XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f);
	camTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Set the View matrix
	camView = XMMatrixLookAtLH( camPosition, camTarget, camUp );

	//Set the Projection matrix
	camProjection = XMMatrixPerspectiveFovLH( 0.4f*3.14f, Width/Height, 1.0f, 1000.0f);

	hr = D3DX11CreateShaderResourceViewFromFile( d3d11Device, L"wood_1.png",
		NULL, NULL, &CubesTexture, NULL );

	hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"glass_1.png",
		NULL, NULL, &GlassTexture, NULL);

	// Describe the Sample State
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
	//Create the Sample State
	hr = d3d11Device->CreateSamplerState( &sampDesc, &CubesTexSamplerState );


	return true;
}

void UpdateScene(double time)
{
	//Keep the cubes rotating
	rot += 0.0f * time;
	if(rot > 6.26f)
		rot = 0.0f;

	//Reset cube1World
	grouncdWorld = XMMatrixIdentity();
	//Scale = XMMatrixScaling(500.0f, 10.0f, 500.0f);
	ScaleNew = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	Translation = XMMatrixTranslation(0.0f, 10.0f, 0.0f);

	//Define cube1's world space matrix
	XMVECTOR rotyaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR rotzaxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR rotxaxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	Rotation = XMMatrixRotationAxis(rotyaxis, rot);	//constantly update the rotaion of the cube, according to input data
	Rotationx = XMMatrixRotationAxis(rotxaxis, rotx);
	Rotationz = XMMatrixRotationAxis(rotzaxis, rotz);
	
	ChangeTheScene();
}

void ChangeTheScene()
{
	cubeCount = 0;
	glassCount = 0;
	for (int x = 0; x < widthX; x += 1)
	{
		for (int z = 0; z < widthZ; z += 1)
		{
			for (int y = 0; y < height; y += 1)
			{
				// Door
				if ((x == trunc(widthX / 2)) && (z == 0) && (y < 2))
				{

				}
				// Left Windows
				else if (windows && x == 0 && (y > 0 && y < height - 1) && (z > 1 && z < widthZ - 2))
				{
					Translation = XMMatrixTranslation(x, y, z);

					glass[glassCount] = XMMatrixIdentity();
					glass[glassCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					glassCount++;
				}
				// Right Windows
				else if (windows && x == widthX - 1 && (y > 0 && y < height - 1) && (z > 1 && z < widthZ - 2))
				{
					Translation = XMMatrixTranslation(x, y, z);

					glass[glassCount] = XMMatrixIdentity();
					glass[glassCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					glassCount++;
				}
				// Back Windows
				else if (windows && (y > 0 && y < height - 1) && (x > 1 && x < widthX - 2) && (z == widthZ - 1))
				{
					Translation = XMMatrixTranslation(x, y, z);

					glass[glassCount] = XMMatrixIdentity();
					glass[glassCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					glassCount++;
				}
				// Front Windows
				else if (windows && (y > 0 && y < height - 1) && (x > 1 && x < widthX - 2) && (x != (trunc(widthX / 2) + 1) && x != (trunc(widthX / 2) - 1) && z == 0))
				{
					Translation = XMMatrixTranslation(x, y, z);

					glass[glassCount] = XMMatrixIdentity();
					glass[glassCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					glassCount++;
				}
				// General Walls
				else if (x == 0 || (x == (widthX - 1)) || (z == 0) || (z == (widthZ - 1)))
				{
					Translation = XMMatrixTranslation(x, y, z);

					cube[cubeCount] = XMMatrixIdentity();
					cube[cubeCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					cubeCount++;
				}
				//roof
				if (roof)
				{
					Translation = XMMatrixTranslation(x, height, z);

					cube[cubeCount] = XMMatrixIdentity();
					cube[cubeCount] = (ScaleNew * Translation * Rotation * Rotationx * Rotationz);
					cubeCount++;
				}
			}
		}
	}

	/*
	switch (wallTexture)
	{
	case 1:
		hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"wood_1.png",
			NULL, NULL, &CubesTexture, NULL);
		break;
	case 2:
		hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"wood_2.png",
			NULL, NULL, &CubesTexture, NULL);
		break;
	case 3:
		hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"wood_3.png",
			NULL, NULL, &CubesTexture, NULL);
		break;
	}
	*/
}

void DrawScene()
{
	//Clear our backbuffer
	float bgColor[4] = {(0.0f, 0.0f, 0.0f, 0.0f)};
	d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

	//Refresh the Depth/Stencil view
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	for (int n = 0; n < cubeCount; n++)
	{
		//Set the WVP matrix and send it to the constant buffer in effect file
		WVP = cube[n] * camView * camProjection;
		cbPerObj.WVP = XMMatrixTranspose(WVP);
		d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
		d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
		d3d11DevCon->PSSetShaderResources(0, 1, &CubesTexture);
		d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

		////Draw the first cube
		d3d11DevCon->DrawIndexed(36, 0, 0);
	}

	for (int n = 0; n < glassCount; n++)
	{
		//Set the WVP matrix and send it to the constant buffer in effect file
		WVP = glass[n] * camView * camProjection;
		cbPerObj.WVP = XMMatrixTranspose(WVP);
		d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
		d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
		d3d11DevCon->PSSetShaderResources(0, 1, &GlassTexture);
		d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

		////Draw the first cube
		d3d11DevCon->DrawIndexed(36, 0, 0);
	}

	TwDraw();
	//Present the backbuffer to the screen
	SwapChain->Present(0, 0);
}

void StartTimer()
{
	LARGE_INTEGER frequencyCount;
	QueryPerformanceFrequency(&frequencyCount);

	countsPerSecond = double(frequencyCount.QuadPart);
}

double GetTime()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	return double(currentTime.QuadPart - counterStart) / countsPerSecond;
}

double GetFrameTime()
{
	LARGE_INTEGER currentTime;
	__int64 tickCount;
	QueryPerformanceCounter(&currentTime);

	tickCount = currentTime.QuadPart - frameTimeOld;
	frameTimeOld = currentTime.QuadPart;

	if (tickCount < 0.0f)
	{
		tickCount = 0.0f;
	}

	return float(tickCount) / countsPerSecond;
}

void WriteToFile(Vertex* vert, DWORD* indices, int vertexArraySize, int indexArraySize)
{
	std::ofstream objFile;
	objFile.open("cube.obj");

	objFile << "o cube" << "\n";
	objFile << "mtllib cube.mtl" << "\n";

	for (int n = 0; n < vertexArraySize; n++)
	{
		objFile << "v ";
		objFile << vert[n].pos.x << " ";
		objFile << vert[n].pos.y << " ";
		objFile << vert[n].pos.z << "\n";
	}

	for (int n = 0; n < vertexArraySize; n++)
	{
		objFile << "vt ";
		objFile << vert[n].texCoord.x << " ";
		objFile << vert[n].texCoord.y << "\n";
	}

	for (int n = 0; n < 24; n++)
	{
		objFile << "vn ";
		objFile << vert[n].normal.x << " ";
		objFile << vert[n].normal.y << " ";
		objFile << vert[n].normal.z << " " << "\n";
	}

	objFile << "g cube" << "\n";
	objFile << "usemtl cube" << "\n";

	for (int n = 0; n < indexArraySize; n += 3)
	{
		objFile << "f ";
		objFile << indices[n] + 1 << "/" << indices[n] + 1 << "/" << indices[n] + 1 << " ";
		objFile << indices[n + 1] + 1 << "/" << indices[n + 1] + 1 << "/" << indices[n + 1] + 1 << " ";
		objFile << indices[n + 2] + 1 << "/" << indices[n + 2] + 1 << "/" << indices[n + 2] + 1 << "\n";
	}

	objFile.close();

	std::ofstream mtlFile;
	mtlFile.open("cube.mtl");

	mtlFile << "newmtl cube" << "\n";
	mtlFile << "map_Ka change_my_mind.png" << "\n";
	mtlFile << "map_Kd change_my_mind.png" << "\n";


	mtlFile.close();
}

bool InitDirectInput(HINSTANCE hInstance)
{
	//Create Direct input object
	hr = DirectInput8Create(
		hInstance,	//handle to the instance of the application
		DIRECTINPUT_VERSION,	//version of direct input we want to use
		IID_IDirectInput8,		//identifier to the interface of direct input we want to use
		(void**)&DirectInput,		//returned pointer to our direct input object
		NULL);	//used for COM aggregation

	hr = DirectInput->CreateDevice(GUID_SysKeyboard, &DIKeyboard, NULL);
	hr = DirectInput->CreateDevice(GUID_SysMouse, &DIMouse, NULL);

	hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);	//type of input data
	hr = DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	hr = DIMouse->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

void DetectInput(double time)
{
	DIMOUSESTATE mouseCurrState;

	BYTE keyboardState[256];	//array of all keys

	DIKeyboard->Acquire();	//takes back control of the device from other applications running
	/*
	if (GetAsyncKeyState('Z'))
	{
		DIMouse->Acquire();
	}
	*/
	DIMouse->Acquire();

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);
	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	if (keyboardState[DIK_ESCAPE] & 0x80)	//check when keys are pressed
	{
		PostMessage(hwnd, WM_DESTROY, 0, 0);
	}

	float speed = 5.0f * time;

	if (keyboardState[DIK_LEFT] & 0x80)	//use arrow keys for cube, wasd for camera
	{
		rotz -= 1.0f * time;
	}
	if (keyboardState[DIK_RIGHT] & 0x80)
	{
		rotz += 1.0f * time;
	}
	if (keyboardState[DIK_UP] & 0x80)
	{
		rotx += 1.0f * time;
	}
	if (keyboardState[DIK_DOWN] & 0x80)
	{
		rotx -= 1.0f * time;
	}
	if (keyboardState[DIK_A] & 0x80)
	{
		moveLeftRight -= speed;
	}
	if (keyboardState[DIK_D] & 0x80)
	{
		moveLeftRight += speed;
	}
	if (keyboardState[DIK_W] & 0x80)
	{
		moveBackForward += speed;
	}
	if (keyboardState[DIK_S] & 0x80)
	{
		moveBackForward -= speed;
	}
	if (keyboardState[DIK_SPACE] & 0x80)
	{
		moveUpDown += speed;
	}
	if (keyboardState[DIK_LSHIFT] & 0x80)
	{
		moveUpDown -= speed;
	}
	if (!GetAsyncKeyState('Z'))
	{
		if ((mouseCurrState.lX != mouseLastState.lX) || (mouseCurrState.lY != mouseLastState.lY))		//checks when mouse has been moved
		{
			camYaw += mouseLastState.lX * 0.001f;
			camPitch += mouseLastState.lY * 0.001f;

			scaleX -= (mouseCurrState.lX * 0.001f);
			scaleY -= (mouseCurrState.lY * 0.001f);
		}
	}

	if (rotx > 6.28)
	{
		rotx -= 6.28;
	}
	else if (rotx < 0)
	{
		rotx = 6.28 + rotx;
	}
	if (rotz > 6.28)
	{
		rotz -= 6.28;
	}
	else if (rotz < 0)
	{
		rotz = 6.28 + rotz;
	}

	mouseLastState = mouseCurrState;
	UpdateCamera();
	return;
}

void UpdateCamera()		//called every frame in DetectInput
{
	camRotationMatrix = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);	//pass in the current rotation
	camTarget = XMVector3TransformCoord(DeafultForward, camRotationMatrix);		//update camera target
	camTarget = XMVector3Normalize(camTarget);	//normalise vector

	XMMATRIX RotateYTempMatrix;
	RotateYTempMatrix = XMMatrixRotationY(camYaw);

	camRight = XMVector3TransformCoord(DefaultRight, RotateYTempMatrix);
	camUp = XMVector3TransformCoord(camUp, RotateYTempMatrix);
	camForward = XMVector3TransformCoord(DeafultForward, RotateYTempMatrix);

	camPosition += moveLeftRight * camRight;
	camPosition += moveBackForward * camForward;
	camPosition += moveUpDown*camUp;

	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;
	moveUpDown = 0.0f;

	camTarget = camPosition + camTarget;
	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);
}

int cubeCalculator()
{
	// calculates the number of cubes need to make up the walls
	int result;	
	result = ((widthX * 2) + (widthZ * 2) - 4) * height;

	return result;
}

int messageloop(){
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while(true)
	{
		BOOL PeekMessageL( 
			LPMSG lpMsg,
			HWND hWnd,
			UINT wMsgFilterMin,
			UINT wMsgFilterMax,
			UINT wRemoveMsg
			);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);	
			DispatchMessage(&msg);
		}
		else{
			// run game code    
			frameCount++;
			if (GetTime() > 1.0f)
			{
				fps = frameCount;
				frameCount = 0;
				StartTimer();
			}

			frameTime = GetFrameTime();
			DetectInput(frameTime);

			UpdateScene(frameTime);
			DrawScene();
		}
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch( msg )
	{
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE ){
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	if (TwEventWin(hwnd, msg, wParam, lParam))
	{
		return 0;
	}

	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

LRESULT CALLBACK MessageProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (TwEventWin(wnd, msg, wParam, lParam))
	{
		return 0;
	}
}