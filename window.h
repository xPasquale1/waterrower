#pragma once

#include <windows.h>
#include <windowsx.h>

#include "util.h"

static uint window_width = 800;		//Größe des Fensters
static uint window_height = 800;
static uint pixel_size = 2;
static uint buffer_width = window_width/pixel_size;
static uint buffer_height = window_height/pixel_size;

static HWND window;
static uint* memory = nullptr;	//Pointer zum pixel-array
static BITMAPINFO bitmapInfo = {};

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void init_window(HINSTANCE hInstance){
	//Erstelle Fenster Klasse
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Window-Class";
	window_class.lpfnWndProc = window_callback;

	//Registriere Fenster Klasse
	RegisterClass(&window_class);

	//Erstelle das Fenster
	window = CreateWindow(window_class.lpszClassName, "Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, NULL, NULL, hInstance, NULL);

	//Bitmap-Info
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	memory = new uint[buffer_width*buffer_height];
}

void close_window(){
	DestroyWindow(window);
	delete[] memory;
}

inline void getMessages()noexcept{
	MSG msg;
	while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline void clear_window()noexcept{
	for(uint y=0; y < buffer_height; ++y){
	    for(uint x=0; x < buffer_width; ++x){
	    	memory[y*buffer_width+x] = 0;
	    }
	}
}

inline constexpr uint RGBA(BYTE r, BYTE g, BYTE b, BYTE a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr BYTE R(uint color){return BYTE(color>>16);}
inline constexpr BYTE G(uint color){return BYTE(color>>8);}
inline constexpr BYTE B(uint color){return BYTE(color);}

inline void draw()noexcept{
	HDC hdc = GetDC(window);
	bitmapInfo.bmiHeader.biWidth = buffer_width;
	bitmapInfo.bmiHeader.biHeight = -buffer_height;
	StretchDIBits(hdc, 0, 0, window_width, window_height, 0, 0, buffer_width, buffer_height, memory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window, hdc);
}

inline void draw_rectangle(uint x, uint y, uint dx, uint dy, uint color)noexcept{
	for(uint i=y; i < y+dy; ++i){
		for(uint j=x; j < x+dx; ++j){
			memory[i*buffer_width+j] = color;
		}
	}
}
