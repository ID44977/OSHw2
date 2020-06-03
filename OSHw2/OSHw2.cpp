// OSHw2.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "OSHw2.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND hWind;
HDC  hWDC;
void InitThreadSemaphore(void);
void DrawPhilosopher(void);

HANDLE Mutex;												// 互斥信号量
HANDLE ChopStick[6];										// 筷子信号量
HANDLE hSemaphore[2];
BOOL ChopstickUsed[6];										// 筷子已被其它线程申请并保持
DWORD PhilosopherID[6];										// 哲学家线程标识符
HANDLE tPhilosopher[6];										// 哲学家线程句柄
RECT PRect[6] = { {350, 100, 380, 130}, {650, 100, 680, 130}, {800, 310, 830, 340},
{650, 520, 680, 550}, {350, 520, 380, 550}, {200, 310, 230, 340} };
HBRUSH hBrush[7];											// 红、绿、蓝、黄、白色刷子

POINT CPolygon[6][4] = { {{210, 177}, {290, 217}, {284, 226}, {204, 186}},
						{{510,  50}, {520,  50}, {520, 130}, {510, 130}},
						{{724, 217}, {804, 177}, {810, 185}, {730, 225}},
						{{730, 427}, {810, 467}, {804, 476}, {724, 436}},
						{{510, 520}, {520, 520}, {520, 600}, {510, 600}},
						{{204, 467}, {284, 427}, {290, 435}, {210, 475}} };

DWORD FAR PASCAL Philosopher(LPVOID);					// 哲学家线程
void Wait(HANDLE);											// 等待信号量
void Signal(HANDLE);											// 发送信号量
void DrawChopstick(HBRUSH, int, BOOL);						// 用对应颜色刷子画筷子
// 并设置/取消使用标记
void WaitTime(int);

//---------------------------------------------------------------------------
//  DWORD FAR PASCAL InitThreadSemaphore()
//
//  Description:
//     生成哲学家线程及各种信号量和变量初始化
//
//---------------------------------------------------------------------------
void InitThreadSemaphore()
{
	int i;

	hBrush[0] = CreateSolidBrush(RGB(255, 0, 0));							// 红色刷子
	hBrush[1] = CreateSolidBrush(RGB(0, 255, 0));							// 绿色刷子
	hBrush[2] = CreateSolidBrush(RGB(0, 0, 255));							// 蓝色刷子
	hBrush[3] = CreateSolidBrush(RGB(255, 255, 0));							// 黄色刷子
	hBrush[4] = CreateSolidBrush(RGB(0, 255, 255));							// 青色刷子
	hBrush[5] = CreateSolidBrush(RGB(255, 0, 255));							// 棕色刷子
	hBrush[6] = CreateSolidBrush(RGB(255, 255, 255));							// 白色刷子

	DrawPhilosopher();

	for (i = 0; i < 6; i++)	ChopStick[i] = CreateSemaphore(NULL, 1, 1, NULL);		// 生成筷子信号量
	Mutex = CreateSemaphore(NULL, 1, 1, NULL);			// 生成互斥信号量--画筷子或判断筷子是否被申请
	for (i = 0; i < 6; i++)	ChopstickUsed[i] = FALSE;			// 筷子未被其它线程申请并保持

	tPhilosopher[0] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家0线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"0",
		0, &PhilosopherID[0]);
	tPhilosopher[1] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家1线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"1",
		0, &PhilosopherID[1]);
	tPhilosopher[2] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家2线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"2",
		0, &PhilosopherID[2]);
	tPhilosopher[3] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家3线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"3",
		0, &PhilosopherID[3]);
	tPhilosopher[4] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家4线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"4",
		0, &PhilosopherID[4]);
	tPhilosopher[5] = CreateThread((LPSECURITY_ATTRIBUTES)NULL,	// 生成哲学家5线程
		0,
		(LPTHREAD_START_ROUTINE)Philosopher,
		(LPVOID)"5",
		0, &PhilosopherID[5]);
}

void DrawPhilosopher(void)
{
	int i;

	SelectObject(hWDC, hBrush[6]);
	for (i = 0; i < 6; i++) Polygon(hWDC, CPolygon[i], 4);

	for (i = 0; i < 6; i++) {
		SelectObject(hWDC, hBrush[i]);
		Ellipse(hWDC, PRect[i].left, PRect[i].top, PRect[i].right, PRect[i].bottom);
	}
}

//---------------------------------------------------------------------------
//  DWORD FAR PASCAL Philosopher1()
//
//  Description:
//     哲学家线程----线程同步及预防死锁
//
//---------------------------------------------------------------------------
DWORD FAR PASCAL Philosopher(LPVOID ThreadParaStr)
{
	int Loop, ThreadID;
	int R1, R2;

	ThreadID = atoi((char *)ThreadParaStr);

	Loop = 0;
	while (TRUE) {
		R1 = ThreadID;
		R2 = (ThreadID + 1) % 6;
		/*
				// 摒弃‘环路等待’条件
				if (ThreadID == 0) {
					R1 = (ThreadID+1) % 6;
					R2 = ThreadID;
				}
		*/
		
				// 摒弃‘请求和保持’条件
				Wait(Mutex);
				if ((ChopstickUsed[R1]) || (ChopstickUsed[R2])) {
					Signal(Mutex);
					goto LoopAgain;
				}
				Signal(Mutex);
		hSemaphore[0] = ChopStick[R1];
		hSemaphore[1] = ChopStick[R2];
		WaitForMultipleObjects(2, hSemaphore, FALSE, INFINITE);

		//Wait(ChopStick[R1]);									// 拿到左手筷子
		DrawChopstick(hBrush[ThreadID], R1, TRUE);			// 将筷子画上颜色并设置使用标志

/*
		// 摒弃‘不剥夺’条件
		Wait(Mutex);
		if (ChopstickUsed[R2]) {
			Signal(Mutex);
			goto ReleaseChopstick;
		}
		Signal(Mutex);
*/

		//Wait(ChopStick[R2]);									// 拿到右手筷子
		DrawChopstick(hBrush[ThreadID], R2, TRUE);			// 将筷子画上颜色并设置使用标志


		WaitTime(1000 * ((Loop*ThreadID % 5) + 1));				// 吃饭

		DrawChopstick(hBrush[6], R2, FALSE);					// 去掉筷子颜色并取消筷子已使用标志
		Signal(ChopStick[R2]);

	ReleaseChopstick:
		DrawChopstick(hBrush[6], R1, FALSE);					// 去掉筷子颜色并取消筷子已使用标志
		Signal(ChopStick[R1]);

	LoopAgain:
		WaitTime(2000 * (Loop % 7 + 1));							// 思考
		if (Loop++ > 10000) Loop = 0;
	}
	return(0);
}

//---------------------------------------------------------------------------
//  DWORD FAR PASCAL Philosopher4()
//
//  Description:
//     画筷子线程
//
//---------------------------------------------------------------------------
void DrawChopstick(HBRUSH hChopstickBrush, int ChopstickID, BOOL ChopstickUsedID)
{
	Wait(Mutex);
	if (ChopstickUsedID) {
		ChopstickUsed[ChopstickID] = TRUE;						// 设置筷子已使用标志
		SelectObject(hWDC, hChopstickBrush);
		Polygon(hWDC, CPolygon[ChopstickID], 4);
	}
	else {
		ChopstickUsed[ChopstickID] = FALSE;						// 取消筷子已使用标志
		SelectObject(hWDC, hChopstickBrush);
		Polygon(hWDC, CPolygon[ChopstickID], 4);
	}
	Signal(Mutex);
}


//---------------------------------------------------------------------------
//  void WaitTime()
//
//  Description:
//     延迟器(单位ms)
//
//---------------------------------------------------------------------------
void WaitTime(int msTime)
{
	int i;
	double Area, PI, r;

	PI = 3.14159;
	r = 10.12345;

	for (i = 0; i < 500 * msTime; i++) Area = PI * r * r;
}

void Wait(HANDLE hSemaphore)
{
	WaitForSingleObject(hSemaphore, INFINITE);							// 等待信号量
}

void Signal(HANDLE hSemaphore) {
	ReleaseSemaphore(hSemaphore, 1, NULL);								// 发送信号量
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OSHW2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OSHW2));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSHW2));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_OSHW2);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
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

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		hWind = hWnd;
		hWDC = GetDC(hWnd);
		break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
			case IDM_EAT:
				InitThreadSemaphore();
				break;
			case IDM_EXIT:
				ReleaseDC(hWnd, hWDC);
				DestroyWindow(hWnd);
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
			RECT rt;
			GetClientRect(hWnd, &rt);
			DrawPhilosopher();
            EndPaint(hWnd, &ps);
			break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
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
