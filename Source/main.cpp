#include <windows.h>

constexpr int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                      // current instance
WCHAR szTitle[MAX_LOADSTRING];        // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HBITMAP hBitmap = nullptr;

void CaptureScreen(RECT clientRect) {
  // Get the screen dimensions
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  // Create a device context for the entire screen
  HDC hdcScreen = GetDC(nullptr);

  // Create a compatible device context to store the captured image
  HDC hdcMem = CreateCompatibleDC(hdcScreen);

  // Create a compatible bitmap to hold the captured image
  if (hBitmap) {
    DeleteObject(hBitmap);
  }

  hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);

  // Select the bitmap into the compatible device context
  SelectObject(hdcMem, hBitmap);

  int windowWidth = clientRect.right - clientRect.left;
  int windowHeight = clientRect.bottom - clientRect.top;

  // Set the stretching mode to improve image quality
  SetStretchBltMode(hdcMem, HALFTONE);

  // Copy the screen data into the compatible device context and scale it down
  StretchBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcScreen, 0, 0,
             screenWidth, screenHeight, SRCCOPY);

  // Release the device contexts
  ReleaseDC(nullptr, hdcScreen);
  DeleteDC(hdcMem);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Initialize global strings
  wcscpy_s(szTitle, MAX_LOADSTRING, L"Capture Screen");
  wcscpy_s(szWindowClass, MAX_LOADSTRING, L"Capture Screen");

  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, 0, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = nullptr;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = nullptr;

  return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  hInst = hInstance;  // Store instance handle in our global variable

  HWND hWnd =
      CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                    0, 1280, 720, nullptr, nullptr, hInstance, nullptr);
  if (!hWnd) {
    return FALSE;
  }

  RECT clientRect;
  GetClientRect(hWnd, &clientRect);
  InvalidateRect(hWnd, &clientRect, TRUE);

  CaptureScreen(clientRect);

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
    case WM_SIZE: {
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      if (hBitmap != nullptr) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);
        BITMAP bitmap{};
        GetObject(hBitmap, sizeof(bitmap), &bitmap);
        BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0,
               SRCCOPY);
        DeleteDC(hdcMem);
      }

      EndPaint(hWnd, &ps);
      break;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}
