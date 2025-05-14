#pragma once
#ifdef WIN32

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#include <functional>
#include <stdexcept>

#include "colors.hpp"
#include "matrix_concept.hpp"
#include "matrix_view.hpp"

namespace f9ay::test::windows {

class Windows {
private:
    int nCmdShow = 1;
    HWND hwnd;
    inline static Windows *singleton;
    std::function<void(HDC)> show_image;

public:
    Windows(const Windows &) = delete;
    Windows() {
        singleton = this;
        HINSTANCE hInstance = GetModuleHandle(NULL);
        WNDCLASS wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"MyWindowClass";
        RegisterClass(&wc);
        hwnd = CreateWindowEx(0, L"MyWindowClass", L"test show image",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                              hInstance, NULL);

        if (!hwnd) {
            throw std::runtime_error("Failed to create window");
        }
    }

    template <::f9ay::MATRIX_CONCEPT Matrix_Type>
        void show(Matrix_Type image) {
        show_image = [image](HDC hdc) -> void {
            using T = std::decay_t<decltype(image[0, 0])>;

            HDC mdc = CreateCompatibleDC(hdc);
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = image.col();
            bmi.bmiHeader.biHeight = -image.row();
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = sizeof(T) * 8;
            bmi.bmiHeader.biCompression = BI_RGB;
            // bits 指向 bitmap 像素的記憶體
            void *bits = nullptr;
            HBITMAP hBitmap = CreateDIBSection(mdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
            SelectObject(mdc, hBitmap);
            auto rawRowSize = (image.col() * sizeof(T));
            if (rawRowSize % 4 != 0 ) {
                rawRowSize = 4 * (rawRowSize / 4 + 1);
            }
            auto *pixels = static_cast<std::byte*>(bits);
            for (int i = 0; i < image.row(); i++) {
                for (int j = 0; j < image.col(); j++) {
                    if constexpr (std::is_same_v<T, colors::BGR> || std::is_same_v<T, colors::BGRA>) {
                        *reinterpret_cast<T*>(&pixels[i * rawRowSize + sizeof(T) * j]) = image[i][j];
                    }
                }
            }
            std::cout << "draw done" << std::endl;
            BitBlt(hdc, 0, 0, image.col(), image.row(), mdc, 0, 0, SRCCOPY);
            DeleteObject(hBitmap);
            DeleteDC(mdc);
        };
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {
        switch (msg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                if (singleton->show_image) {
                    singleton->show_image(hdc);
                }

                EndPaint(hwnd, &ps);
                break;
            }
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
};

}  // namespace f9ay::test::windows

#endif
