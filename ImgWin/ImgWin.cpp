// ImgWin.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ImgWin.h"
#include "ImgCapture.h"
#include "ImgProcessHelper.h"
#include <Commctrl.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
ImgCapture* ImgObj = NULL;
unsigned short* array = NULL;
int img_rows = 0, img_cols = 0;
bool displayflag = false;
int GainValue = 30;
bool Lock_Image = false;

cv::Mat image;
cv::Mat image3c;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Gain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID OnPaint(HDC hdc);
void SaveImage(std::string FileName);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

    // TODO: Place code here.
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // TODO: Place code here.
    ImgObj = new ImgCapture();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_IMGWIN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IMGWIN));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplusToken);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMGWIN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_IMGWIN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

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
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        if (ImgObj->init() != 0)
            MessageBox(hWnd, L"Error", L"Camera cannot be initialised", MB_OK);

        // Retrieve image dimensions
        img_rows = ImgObj->get_height();
        img_cols = ImgObj->get_width();

        ImgObj->SetGain(40);

        // Start the timer
        SetTimer(hWnd, IDT_TIMER, 40, NULL);
        displayflag = true;

        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_SAVE:
                Lock_Image = true;
                SaveImage("OutImg.png");
                Lock_Image = false;
                break;
            case IDM_GAIN:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOGGAIN), hWnd, Gain);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_TIMER:
        if (!Lock_Image)
        {
            ImgObj->shoot(1);
            array = ImgObj->get_img();

            image.create(img_rows, img_cols, CV_8U);
            //image3c.create(img_rows, img_cols, CV_8UC3);
            if (array != NULL)
            {
                double pixel = 0.0;

                // Acquire image into Mat
                for (int ht = 0; ht < img_rows; ht++)
                    for (int wd = 0; wd < img_cols; wd++)
                    {
                        pixel = static_cast<double>(array[ht * img_cols + wd]);
                        pixel = pixel / 1022. * 255.0; // conversion to be 2 bytes per pixel

                        image.at<uchar>(ht, wd) = static_cast<unsigned char>(pixel);

                    }

                cv::Mat src2;
                // Flip the image vertically
                cv::flip(image, src2, 0);

                // Convert into 3 channels but still in grayscale to fit the colour palette for Bitmap
                cv::cvtColor(src2, image3c, cv::COLOR_GRAY2BGR, 3);

                //cv::imwrite("testout.png", image3c);
                // Force redraw
                InvalidateRect(hWnd, NULL, false);
            }
        }

        
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            if (displayflag)
                OnPaint(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        if(ImgObj->close() != 0)
            MessageBox(hWnd, L"Error", L"Camera cannot be released", MB_OK);
        
        KillTimer(hWnd, IDT_TIMER);
        PostQuitMessage(0);
        
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void SaveImage(std::string FileName)
{
    ImgProcessHelper ImgProcessObj;

    cv::Mat Tgt;
    // Flip the image vertically
    cv::flip(image3c, Tgt, 0);

    ImgProcessObj.SaveImg(FileName, Tgt);
    
}

INT_PTR CALLBACK Gain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hTrackBar = GetDlgItem(hDlg, IDC_SLIDER1);
    
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SendMessage(hTrackBar, TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(hTrackBar, TBM_SETRANGEMAX, FALSE, 100);
        SendMessage(hTrackBar, TBM_SETPOS, TRUE, GainValue);
        SendMessage(hTrackBar, TBM_SETTICFREQ, 5, 0);
        return (INT_PTR)TRUE;
    case WM_HSCROLL:
        if (LOWORD(wParam) == TB_THUMBPOSITION)
        {
            // Get the selected position
            LRESULT pos = SendMessage(hTrackBar, TBM_GETPOS, 0, 0);

            if (pos > 0 && pos <= 100)
                GainValue = static_cast<int>(pos);
                
            //MessageBox(hDlg, L"Trackbar change trigger", L"Debug", MB_OK);
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            // Call class method to set gain
            if (LOWORD(wParam) == IDOK)
                ImgObj->SetGain(GainValue);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for about box.
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

/////////////////////////////////////
// Painting routines for the window
VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    
    // Send image to the client area
    // Define paramters for header
    BITMAPINFOHEADER bmheader;
    bmheader.biSize = sizeof(bmheader);
    bmheader.biWidth = img_cols;
    bmheader.biHeight = img_rows;
    bmheader.biPlanes = 1;
    bmheader.biBitCount = 24;
    bmheader.biCompression = BI_RGB;
    bmheader.biSizeImage = 0;
    bmheader.biXPelsPerMeter = 0;
    bmheader.biYPelsPerMeter = 0;
    bmheader.biClrUsed = 0;
    bmheader.biClrImportant = 0;

    RGBQUAD ClrPal;
    ClrPal.rgbGreen = ClrPal.rgbRed = ClrPal.rgbBlue = 255;
    ClrPal.rgbReserved = 0;

    BITMAPINFO bmInfo;
    bmInfo.bmiHeader = bmheader;
    bmInfo.bmiColors[0] = ClrPal;

    Bitmap MyBMImg(&bmInfo, image3c.data);

    graphics.DrawImage(&MyBMImg, 0, 0);

   
}

