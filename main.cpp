#include <iostream>
#include <winusb.h>
extern "C"{
	#include <hidsdi.h>
}
#include <setupapi.h>
#include <thread>
#include "window.h"
#include "usb.h"
#include "pages.h"

//HANDLE hDevice = open_device("\\\\.\\COM6", 19200);
BYTE receiveBuffer[128];
BYTE sendBuffer[128];
Page main_page;

SYSTEMTIME last_request_tp2 = {};
//Intervall in Millisekunden, sollte nicht < 26 sein
ErrCode loadStartPage();
ErrCode loadFreeTrainingPage();
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
		ErrCheck(addRequest(0));
		ErrCheck(addRequest(1));
		ErrCheck(addRequest(2));
		ErrCheck(addRequest(3));
	}
}

void displayDataPage(){
	main_page.menus[0]->labels[0].text = "Distanz: " + std::to_string(rowingData.dist);
	main_page.menus[0]->labels[1].text = "Zeit: " + std::to_string(rowingData.hrs) + ':' + std::to_string(rowingData.min) + ':' + std::to_string(rowingData.sec);
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	HWND main_window;
	ErrCheck(openWindow(hInstance, 800, 800, 2, main_window), "open main window");

//	init_communication(hDevice, sendBuffer, receiveBuffer);
	ErrCheck(loadStartPage(), "laden des Startbildschirms");

	while(app.window_count){
		ErrCheck(clearWindow(main_window), "clear window");

		updatePage(main_page, main_window);

		ErrCheck(drawWindow(main_window), "draw window");

//		refreshData();
//		transmitRequests(hDevice);

//		int length = readPacket(hDevice, receiveBuffer, 128);
//		if(length > 0){
//			checkCode(receiveBuffer, length);
//		}
		getMessages();	//TODO frägt alle windows ab, könnte evtl nicht nötig sein
	}

	//Aufräumen
	destroyPage(main_page);
//	strcpy((char*)sendBuffer, "EXIT");
//	sendPacket(hDevice, sendBuffer, sizeof("EXIT")-1);
//	CloseHandle(hDevice);
	return 0;
}

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:{
		ErrCheck(setWindowState(hwnd, WINDOW_SHOULD_CLOSE), "set close window state");
		break;
	}
	case WM_SIZE:{
		//TODO Fenster skalieren
        break;
	}
	case WM_LBUTTONDOWN:{
		if(!getButton(mouse, MOUSE_LMB)){

		};
		setButton(mouse, MOUSE_LMB);
		break;
	}
	case WM_LBUTTONUP:{
		resetButton(mouse, MOUSE_LMB);
		break;
	}
	case WM_RBUTTONDOWN:{
		if(getButton(mouse, MOUSE_RMB)){

		};
		setButton(mouse, MOUSE_RMB);
		break;
	}
	case WM_RBUTTONUP:{
		resetButton(mouse, MOUSE_RMB);
		break;
	}
	case WM_MOUSEMOVE:{
		for(WORD i=0; i < app.window_count; ++i){
			if(app.windows[i] == hwnd){
				mouse.pos.x = GET_X_LPARAM(lParam)/app.info[i].pixel_size;
				mouse.pos.y = GET_Y_LPARAM(lParam)/app.info[i].pixel_size;
			}
		}
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ErrCode loadStartPage(){
	destroyPage(main_page);
	Menu* main = new Menu;
	main_page.menus[0] = main;
	main_page.menu_count = 1;

	ivec2 pos = {100, 200};
	ivec2 size = {160, 40};
	main_page.menus[0]->buttons[0].pos = {pos.x, pos.y};
	main_page.menus[0]->buttons[0].size = {size.x, size.y};
	main_page.menus[0]->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	main_page.menus[0]->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	main_page.menus[0]->buttons[0].event = &loadFreeTrainingPage;
	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button texture laden");
	main_page.images[0] = buttonImage;
	main_page.image_count = 1;
	main_page.menus[0]->buttons[0].text = "START";
	main_page.menus[0]->buttons[0].image = buttonImage;
	main_page.menus[0]->button_count = 1;

	main_page.code = _default_page_function;

	return SUCCESS;
}

ErrCode loadFreeTrainingPage(){
	destroyPage(main_page);
	Menu* main = new Menu;
	main_page.menus[0] = main;
	main_page.menu_count = 1;

	ivec2 pos = {50, 350};
	ivec2 size = {160, 40};
	main_page.menus[0]->buttons[0].pos = {pos.x, pos.y};
	main_page.menus[0]->buttons[0].size = {size.x, size.y};
	main_page.menus[0]->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	main_page.menus[0]->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	main_page.menus[0]->buttons[0].event = &loadStartPage;
	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button texture laden");
	main_page.images[0] = buttonImage;
	main_page.image_count = 1;
	main_page.menus[0]->buttons[0].text = "BEENDEN";
	main_page.menus[0]->buttons[0].image = buttonImage;
	main_page.menus[0]->button_count = 1;

	main_page.menus[0]->labels[0].pos = {20, 20};
	main_page.menus[0]->labels[0].text_size = 4;
	main_page.menus[0]->labels[1].pos = {20, 80};
	main_page.menus[0]->labels[1].text_size = 4;
	main_page.menus[0]->labels[1].text = "test";
	main_page.menus[0]->label_count = 2;

	main_page.code = displayDataPage;

	return SUCCESS;
}
