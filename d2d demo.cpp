// d2d demo.cpp : ����Ӧ�ó������ڵ㡣
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


#define MAX_LOADSTRING 100

#define SAFE_RELEASE(P) if(P) {P->Release(); P = NULL;}

#define VK(ch) (ch - 'A' + 65)

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������

double Px = 0, Py = 0;
double vx0 = 20, vy0 = 20, vx = 0, vy = 0;
double tpre, tnow;
bool IsDown[256];
DWORD g_t_now, g_t_pre;
HWND GhWnd;

ID2D1Factory			* pD2DFactory	= NULL;
ID2D1HwndRenderTarget	* pRenderTarget = NULL;
ID2D1SolidColorBrush	* pBlackBrush	= NULL;

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void DrawRectangle(HWND);
void Update();
void Render(HWND);

struct Force
{
	double x = 0, y = 0;
};

struct Rectangle
{
	double vx, vy, px, py, ax, ay, fx, fy, lenx, leny, mass;
	Rectangle()
	{
		vx = vy = px = py = ax = ay = fx = fy;
		mass = 0.5;
		lenx = leny = 50;
	}
	~Rectangle(){}
	void UpdateAcc()
	{
		ax = fx / mass;
		ay = fy / mass;
	}
	void AddForce(const Force f)
	{
		fx += f.x;
		fy += f.y;
		UpdateAcc();
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
		vx += ax * dt;
		vy += ay * dt;
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

struct Rectangle Rec1;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: �ڴ˷��ô��롣

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D2DDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D2DDEMO));

    MSG msg;

    // ����Ϣѭ��: 
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
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
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
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

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
	double px, py, lenx, leny;
	Rec.GetPosition(px, py);
	Rec.GetSize(lenx, leny);
	pRenderTarget->DrawRectangle(D2D1::RectF(px, py, px + lenx, py + leny), pBlackBrush);

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
	LastUpdateTime = CurrentTime;
}

void Render(HWND hWnd)
{
	DrawRectangle(hWnd, Rec1);
}



//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
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
            // �����˵�ѡ��: 
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
            //// TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
            //EndPaint(hWnd, &ps);

			//D2DRender(hWnd);

			Render(hWnd);
        }
        break;
	case WM_KEYDOWN:
		switch (wParam)
		{
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
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
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

// �����ڡ������Ϣ�������
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
