// d2d demo.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "d2d demo.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <windows.h>
#include <time.h>
#include <WinUser.h>
#include <cmath>


#define MAX_LOADSTRING 100

#define SAFE_RELEASE(P) if(P) {P->Release(); P = NULL;}

#define VK(ch) (ch - 'A' + 65)

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

double Px = 0, Py = 0;
double vx0 = 50, vy0 = 50, vcx = 200, vcy = 200;
double tpre, tnow;
double AirK = 0.01;
bool IsDown[256];
DWORD g_t_now, g_t_pre;
HWND GhWnd;

ID2D1Factory			* pD2DFactory	= NULL;
ID2D1HwndRenderTarget	* pRenderTarget = NULL;
ID2D1SolidColorBrush	* pBlackBrush	= NULL;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void DrawRectangle(HWND);
void Update();
void Render(HWND);

struct PVector
{
	double x, y;
	PVector()
	{
		x = y = 0;
	}
	PVector(double _x, double _y)
	{
		x = _x;
		y = _y;
	}
	void add(PVector v)
	{
		x += v.x;
		y += v.y
	}
	void sub(PVector v)
	{
		x -= v.x;
		y -= v.y;
	}
	void mult(double t)
	{
		x *= t;
		y *= t;
	}
	void div(double t)
	{
		if (t == 0)
		{
			MessageBox(NULL, _T("Can't be divided by 0!"), _T("ERROR"), MB_OK);
			return;
		}
		x /= t;
		y /= t;
	}
	double mag()
	{
		return sqrt(x * x + y * y);
	}
	void normalize()
	{
		double t = mag();
		div(t);
	}
	void setmag(double k)
	{
		double t = mag();
		if (t != 0) mult(k / t);
	}
	void limit(double l)
	{
		double t = mag();
		if (t > l) setmag(l);
	}
	double heading2D()
	{
		return atan2(y, x);
	}
	double rotate(double theta)
	{
		double _x = x * cos(theta) - y * sin(theta), _y = x * sin(theta) + y * cos(theta);
		x = _x;
		y = _y;
	}
};

PVector operator + (PVector a, PVector b)
{
	return PVector{ a.x + b.x, a.y + b.y };
}
PVector operator - (PVector a, PVector b)
{
	return PVector{ a.x - b.x, a.y - b.y };
}
PVector operator * (PVector v, double k)
{
	return PVector{ v.x * k, v.y * k };
}
PVector operator / (PVector v, double k)
{
	if (k == 0) return v;
	return PVector{ v.x / k, v.y / k };
}

struct Rectg
{
	PVector location, velocity, acceleration;
	double mass;
	Rectg() {};
	Rectg(double x, double y)
	{
		location.add(PVector{ x, y });
	}
	void update(double dt)
	{
		velocity.add(acceleration * dt);
		location.add(velocity * dt);
	}
};

struct Rectangle
{
	double vx, vy, px, py, ax, ay, lenx, leny, mass;
	double fx, fy, AirForcex, AirForcey;
	Rectangle()
	{
		vx = vy = px = py = ax = ay = fx = fy;
		mass = 0.5;
		lenx = leny = 50;
		AirForcex = AirForcey = 0;
	}
	~Rectangle(){}
	void UpdateAcc()
	{
		ax = fx / mass;
		ay = fy / mass;
	}
	void AddForce(double x, double y)
	{
		fx += x;
		fy += y;
		UpdateAcc();
	}
	void UpdatePosition(double dt)
	{
		px += vx * dt;
		py += vy * dt;
		double NewAirForcex, NewAirForcey;
		vx += ax * dt - AirForce;
		vy += ay * dt - AirForce;
	}
	void GetPosition(double &x, double &y)
	{
		x = px;
		y = py;
	}
	void GetSize(double &lx, double &ly)
	{
		lx = lenx;
		ly = leny;
	}
};

struct CameraType
{
	double vx, vy, px, py;
	CameraType()
	{
		vx = vy = px = py = 0;
	}
	~CameraType() {}
	void GetPosition(double &x, double &y)
	{
		x = px;
		y = py;
	}
	void AddVelocity(double x, double y)
	{
		vx += x;
		vy += y;
	}
	void update(double dt)
	{
		px += dt * vx;
		py += dt * vy;
	}
};

struct CameraType Camera;
struct Rectangle Rec1;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D2DDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D2DDEMO));

    MSG msg;

    // 主消息循环: 
	g_t_pre = GetTickCount();
	tpre = clock();
	memset(IsDown, 0, sizeof IsDown);
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
			
        }
		else
		{
			Update();
			Render(GhWnd);
			//D2D_Update(hWnd);
			//D2D_Render(hWnd);
		}
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D2DDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_D2DDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   GhWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void CreateD2DResource(HWND hWnd)
{
	if (!pRenderTarget)
	{
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
		if (FAILED(hr))
		{
			MessageBoxA(hWnd, "Create D2D factory failed!", "Error", 0);
			return;
		}

		// Obtain the size of the drawing area
		RECT rc;
		GetClientRect(hWnd, &rc);

		// Create a Direct2D render target
		hr = pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(
				hWnd,
				D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)
			),
			&pRenderTarget
		);
		if (FAILED(hr))
		{
			MessageBoxA(hWnd, "Create render target failed!", "Error", 0);
			return;
		}

		// Create a brush
		hr = pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&pBlackBrush
		);
		if (FAILED(hr))
		{
			MessageBoxA(hWnd, "Create brush failed!", "Error", 0);
			return;
		}
	}
}

void DrawRectangle(HWND hWnd, struct Rectangle &Rec)
{
	CreateD2DResource(hWnd);
	pRenderTarget->BeginDraw();
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	//RECT rc;
	//GetClientRect(hWnd, &rc);
	//Px = rc.left;
	//Py = rc.top;
	double px, py, lenx, leny, cx, cy;
	Rec.GetPosition(px, py);
	Rec.GetSize(lenx, leny);
	Camera.GetPosition(cx, cy);
	pRenderTarget->DrawRectangle(D2D1::RectF(px - cx, py - cy, px + lenx - cx, py + leny - cy), pBlackBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr))
	{
		//MessageBox(NULL, "Draw failed!", "Error", 0);
		return;
	}
}

void Cleanup()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pBlackBrush);
	SAFE_RELEASE(pD2DFactory);
}


void Update()
{
	static int never = 1;
	static DWORD LastUpdateTime;
	DWORD CurrentTime = GetTickCount();
	if (never)
	{
		LastUpdateTime = CurrentTime;
		never = 0;
	}
	double dt = ((double)CurrentTime - LastUpdateTime) / 1000;
	Rec1.UpdatePosition(dt);
	Camera.update(dt);
	LastUpdateTime = CurrentTime;
}

void Render(HWND hWnd)
{
	DrawRectangle(hWnd, Rec1);
}



//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static double lastwasd;
	static WPARAM lastkey = 0;
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            //PAINTSTRUCT ps;
            //HDC hdc = BeginPaint(hWnd, &ps);
            //// TODO: 在此处添加使用 hdc 的任何绘图代码...
            //EndPaint(hWnd, &ps);

			//D2DRender(hWnd);

			Render(hWnd);
        }
        break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		// control rec1
		case VK('W'):
			if (!IsDown[VK('W')])
			{
				IsDown[VK('W')] = true;
				Rec1.AddForce(0, -vy0);
			}
			break;
		case VK('S'):
			if (!IsDown[VK('S')])
			{
				IsDown[VK('S')] = true;
				Rec1.AddForce(0, vy0);
			}
			break;
		case VK('A'):
			if (!IsDown[VK('A')])
			{
				IsDown[VK('A')] = true;
				Rec1.AddForce(-vx0, 0);
			}
			break;
		case VK('D'):
			if (!IsDown[VK('D')])
			{
				IsDown[VK('D')] = true;
				Rec1.AddForce(vx0, 0);
			}
			break;
		// control camera
		case VK('I'):
			if (!IsDown[VK('I')])
			{
				IsDown[VK('I')] = true;
				Camera.AddVelocity(0, -vcy);
			}
			break;
		case VK('K'):
			if (!IsDown[VK('K')])
			{
				IsDown[VK('K')] = true;
				Camera.AddVelocity(0, vcy);
			}
			break;
		case VK('J'):
			if (!IsDown[VK('J')])
			{
				IsDown[VK('J')] = true;
				Camera.AddVelocity(-vcx, 0);
			}
			break;
		case VK('L'):
			if (!IsDown[VK('L')])
			{
				IsDown[VK('L')] = true;
				Camera.AddVelocity(vcx, 0);
			}
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		// control rec1
		case VK('W'):
			Rec1.AddForce(0, vy0);
			IsDown[VK('W')] = false;
			break;
		case VK('S'):
			Rec1.AddForce(0, -vy0);
			IsDown[VK('S')] = false;
			break;
		case VK('A'):
			Rec1.AddForce(vx0, 0);
			IsDown[VK('A')] = false;
			break;
		case VK('D'):
			Rec1.AddForce(-vx0, 0);
			IsDown[VK('D')] = false;
			break;

		// contral camera
		case VK('I'):
			Camera.AddVelocity(0, vcy);
			IsDown[VK('I')] = false;
			break;
		case VK('K'):
			Camera.AddVelocity(0, -vcy);
			IsDown[VK('K')] = false;
			break;
		case VK('J'):
			Camera.AddVelocity(vcx, 0);
			IsDown[VK('J')] = false;
			break;
		case VK('L'):
			Camera.AddVelocity(-vcx, 0);
			IsDown[VK('L')] = false;
			break;
		}
    case WM_DESTROY:
		Cleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
		
    }
	Update();
	Render(hWnd);
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
