#include <windows.h>

#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HBITMAP hBitmap = nullptr;

constexpr unsigned short IDM_FILE_SAVE{1};
constexpr unsigned short IDM_FILE_QUIT{2};

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

  BITMAPINFOHEADER bmpInfoHeader{};
  bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmpInfoHeader.biWidth = screenWidth;
  bmpInfoHeader.biHeight =
      -screenHeight;  // Negative height for top-down orientation
  bmpInfoHeader.biPlanes = 1;
  bmpInfoHeader.biBitCount = 32;  // 32 bits per pixel (RGB + Alpha)
  bmpInfoHeader.biCompression = BI_RGB;

  hBitmap =
      CreateDIBSection(hdcScreen, reinterpret_cast<BITMAPINFO*>(&bmpInfoHeader),
                       DIB_RGB_COLORS, nullptr, nullptr, 0);

  // Select the bitmap into the compatible device context
  SelectObject(hdcMem, hBitmap);

  // Copy the screen data into the compatible device context
  BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, clientRect.left,
         clientRect.top, SRCCOPY);

  // Release the device contexts
  ReleaseDC(nullptr, hdcScreen);
  DeleteDC(hdcMem);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  WNDCLASSW wc{};
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = nullptr;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = L"Screen Capture";

  RegisterClassW(&wc);

  HWND hWnd = CreateWindowW(wc.lpszClassName, L"Screen Capture",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1280, 720,
                            nullptr, nullptr, hInstance, nullptr);
  if (!hWnd) {
    return FALSE;
  }

  RECT clientRect;
  GetClientRect(hWnd, &clientRect);
  InvalidateRect(hWnd, &clientRect, TRUE);

  CaptureScreen(clientRect);

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      HMENU hMenubar = CreateMenu();
      HMENU hMenu = CreateMenu();

      AppendMenuW(hMenu, MF_STRING, IDM_FILE_SAVE, L"&Save");
      AppendMenuW(hMenu, MF_STRING, IDM_FILE_QUIT, L"&Quit");

      AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hMenu, L"&File");
      SetMenu(hWnd, hMenubar);
      break;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDM_FILE_SAVE: {
          // Create a file dialog to choose the save location
          OPENFILENAME ofn{};
          WCHAR szFileName[MAX_PATH] = L"";

          ofn.lStructSize = sizeof(OPENFILENAME);
          ofn.hwndOwner = hWnd;
          ofn.lpstrFilter = L"PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0";
          ofn.lpstrFile = szFileName;
          ofn.nMaxFile = MAX_PATH;
          ofn.Flags = OFN_OVERWRITEPROMPT;

          if (GetSaveFileName(&ofn)) {
            int charCount = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1,
                                                nullptr, 0, nullptr, nullptr);
            char* pszFileName = new char[charCount];
            WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, pszFileName,
                                charCount, nullptr, nullptr);

            // Convert the captured bitmap to a compatible format (DIB)
            BITMAP bitmap{};
            GetObject(hBitmap, sizeof(bitmap), &bitmap);

            BITMAPINFOHEADER bmpInfoHeader{};
            bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmpInfoHeader.biWidth = bitmap.bmWidth;
            bmpInfoHeader.biHeight =
                -bitmap.bmHeight;  // Negative height for top-down orientation
            bmpInfoHeader.biPlanes = 1;
            bmpInfoHeader.biBitCount = 32;  // 32 bits per pixel (RGB + Alpha)
            bmpInfoHeader.biCompression = BI_RGB;

            // Calculate the size of the image data
            DWORD imageSize =
                ((bitmap.bmWidth * bmpInfoHeader.biBitCount + 31) / 32) * 4 *
                bitmap.bmHeight;

            // Allocate memory for the image data
            BYTE* imageData = new BYTE[imageSize];

            // Get the image data in the DIB format
            HDC hdcScreen = GetDC(NULL);
            GetDIBits(hdcScreen, hBitmap, 0, bitmap.bmHeight, imageData,
                      (BITMAPINFO*)&bmpInfoHeader, DIB_RGB_COLORS);
            ReleaseDC(NULL, hdcScreen);

            // Save the image data as a PNG file using STB image
            stbi_write_png(pszFileName, bitmap.bmWidth, bitmap.bmHeight, 4,
                           imageData, 0);  // No additional flags

            // Clean up resources
            delete[] pszFileName;
            delete[] imageData;
          }
          break;
        }
        case IDM_FILE_QUIT:
          SendMessageW(hWnd, WM_CLOSE, 0, 0);
          break;
      }
      break;
    }
    case WM_PAINT: {
      if (!hBitmap) {
        break;
      }

      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      HDC hdcMem = CreateCompatibleDC(hdc);
      SelectObject(hdcMem, hBitmap);
      BITMAP bitmap{};
      GetObject(hBitmap, sizeof(bitmap), &bitmap);

      // Calculate the destination rectangle size
      RECT clientRect;
      GetClientRect(hWnd, &clientRect);
      int destWidth = clientRect.right - clientRect.left;
      int destHeight = clientRect.bottom - clientRect.top;

      SetStretchBltMode(hdc, HALFTONE);

      // Draw the image with the decreased size
      StretchBlt(hdc, 0, 0, destWidth, destHeight, hdcMem, 0, 0, bitmap.bmWidth,
                 bitmap.bmHeight, SRCCOPY);

      DeleteDC(hdcMem);

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
