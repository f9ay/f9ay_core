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
        singleton = this;
    }

    template <::f9ay::MATRIX_CONCEPT Matrix_Type>
    void show(Matrix_Type image) {
        show_image = [image](HDC hdc) -> void {
            for (int i = 0; i < image.row(); i++) {
                for (int j = 0; j < image.col(); j++) {
                    if constexpr (std::is_same_v<
                                      std::decay_t<decltype(image[i, j])>,
                                      colors::BGR>) {
                        auto [b, g, r] = image[i][j];
                        const COLORREF colorRef = RGB(r, g, b);
                        SetPixel(hdc, i, j, colorRef);
                    } else if (std::is_same_v<
                                   std::decay_t<decltype(image[i, j])>,
                                   f9ay::colors::BGRA>) {
                        // alpha 通道忽略
                        auto [b, g, r, a] = image[i][j];
                        const COLORREF colorRef = RGB(b, g, a);
                        SetPixel(hdc, i, j, colorRef);
                    }
                }
            }
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
