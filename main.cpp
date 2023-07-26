#include <iostream>
#include <winusb.h>
extern "C"{
	#include <hidsdi.h>
}
#include <setupapi.h>
#include <thread>
#include "window.h"
#include "usb.h"
#include "font.h"

HANDLE hDevice = open_device("\\\\.\\COM3", 9600);
BYTE receiveBuffer[128];
BYTE sendBuffer[128];
static bool running = true;

SYSTEMTIME last_request_tp2 = {};
//Intervall in Millisekunden, sollte nicht < 26 sein
void refreshData(WORD interval=250){
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	DWORD newmilli = systemTime.wMilliseconds;
	newmilli += systemTime.wSecond*1000;
	newmilli += systemTime.wMinute*60000;
	newmilli += systemTime.wHour*3600000;
	DWORD currentmilli = last_request_tp2.wMilliseconds;
	currentmilli += last_request_tp2.wSecond*1000;
	currentmilli += last_request_tp2.wMinute*60000;
	currentmilli += last_request_tp2.wHour*3600000;
	if(newmilli - currentmilli > interval){
		last_request_tp2 = systemTime;
		addRequest(0);
		addRequest(1);
		addRequest(2);
		addRequest(3);
	}
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	init_window(hInstance);

	init_communication(hDevice, sendBuffer, receiveBuffer);

	while(running){
		getMessages();
		clear_window();
		int font_size = 5;
		int fs2 = font_size*font_size;
		draw_int(10, 10, font_size, rowingData.dist, RGBA(255, 255, 255));
		draw_int(10, 10+fs2+5, font_size, rowingData.hrs, RGBA(255, 255, 255));
		draw_character(10+fs2+5, 10+fs2+5, font_size, ':', RGBA(255, 255, 255));
		draw_int(10+fs2*2+5, 10+fs2+5, font_size, rowingData.min, RGBA(255, 255, 255));
		draw_character(10+fs2*3+5, 10+fs2+5, font_size, ':', RGBA(255, 255, 255));
		draw_int(10+fs2*4+5, 10+fs2+5, font_size, rowingData.sec, RGBA(255, 255, 255));
		draw();

		refreshData();
		transmitRequests(hDevice);

		int length = readPacket(hDevice, receiveBuffer, 128);
		if(length > 0){
			checkCode(receiveBuffer, length);
		}
	}

	//Aufräumen
	strcpy((char*)sendBuffer, "EXIT");
	sendPacket(hDevice, sendBuffer, sizeof("EXIT")-1);
	CloseHandle(hDevice);
	return 0;
}

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY: {
		running = false;
		break;
	}
	case WM_SIZE:{
		window_width = LOWORD(lParam);
		window_height = HIWORD(lParam);
		buffer_width = window_width/pixel_size;
		buffer_height = window_height/pixel_size;
        delete[] memory;
        memory = new uint[buffer_width*buffer_height];
        break;
	}
	case WM_LBUTTONDOWN:{
		if(!mouse.lmb){
			addRequest(0);
			addRequest(1);
			addRequest(2);
			addRequest(3);
		};
		mouse.lmb = true;
		break;
	}
	case WM_LBUTTONUP:{
		mouse.lmb = false;
		break;
	}
	case WM_RBUTTONDOWN:{
		if(!mouse.rmb){
			addRequest(2);
		};
		mouse.rmb = true;
		break;
	}
	case WM_RBUTTONUP:{
		mouse.rmb = false;
		break;
	}
	case WM_MOUSEMOVE:{
		mouse.pos.x = GET_X_LPARAM(lParam)/pixel_size;
		mouse.pos.y = GET_Y_LPARAM(lParam)/pixel_size;
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
