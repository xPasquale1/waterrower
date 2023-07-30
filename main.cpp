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

#define NO_DEVICE

LRESULT CALLBACK basic_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifndef NO_DEVICE
HANDLE hDevice = open_device("\\\\.\\COM6", 19200);
#endif
//TODO Ein paar Dinge hier sollten bei Zeit mal vom Stack runter, nicht das hier Böses passiert
BYTE receiveBuffer[128];
BYTE sendBuffer[128];
RequestQueue queue;
Page main_page;
BYTE page_switch = 0;	//0 kein wechsel, 1 wechsel zur startseite, 2 free training seite
Font* default_font = nullptr;

ErrCode loadStartPage(){page_switch = 1; return SUCCESS;};
ErrCode loadFreeTrainingPage(){page_switch = 2; return SUCCESS;};
ErrCode switchToStartPage();
ErrCode switchToFreeTrainingPage();
ErrCode switchPage();
void refreshData(RequestQueue& queue, WORD interval=250){
	static SYSTEMTIME last_request_tp2 = {};
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
		ErrCheck(addRequest(queue, 0));
		ErrCheck(addRequest(queue, 1));
		ErrCheck(addRequest(queue, 2));
		ErrCheck(addRequest(queue, 3));
		ErrCheck(addRequest(queue, 4));
		ErrCheck(addRequest(queue, 5));
		ErrCheck(addRequest(queue, 6));
		ErrCheck(addRequest(queue, 7));
		ErrCheck(addRequest(queue, 8));
	}
}

void displayDataPage(){
#ifndef NO_DEVICE
		refreshData(queue, 250);
#endif
	main_page.menus[0]->labels[0].text = "Distanz: " + std::to_string(rowingData.dist) + 'm';
	main_page.menus[0]->labels[1].text = "Geschwindigkeit: " + std::to_string(rowingData.ms_total) + "m/s";
	float avg_ms = 0;
	uint total_sec = rowingData.sec+rowingData.min*60+rowingData.hrs*3600;
	if(total_sec > 0){
		avg_ms = (float)rowingData.dist/total_sec;
	}
	main_page.menus[0]->labels[2].text = "Durchschnittlich: " + float_to_string(avg_ms) + "m/s";
	main_page.menus[0]->labels[3].text = "Zeit: " + std::to_string(rowingData.hrs) + ':' + std::to_string(rowingData.min) + ':' + std::to_string(rowingData.sec);
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	HWND main_window;
	ErrCheck(openWindow(hInstance, 800, 800, 2, main_window, "waterrower", basic_window_callback), "open main window");

#ifndef NO_DEVICE
	init_communication(hDevice, sendBuffer, receiveBuffer);
#endif

	default_font = new Font;
	ErrCheck(loadFont("fonts/ascii.tex", *default_font, {82, 83}), "font laden");
	default_font->font_size = 31;
	main_page.font = default_font;

	ErrCheck(loadStartPage(), "laden des Startbildschirms");

	while(app.window_count){
		ErrCheck(clearWindow(main_window), "clear window");

		updatePage(main_page, main_window);

		switchPage();

		ErrCheck(drawWindow(main_window), "draw window");

#ifndef NO_DEVICE
		transmitRequests(queue, hDevice);

		int length = readPacket(hDevice, receiveBuffer, sizeof(receiveBuffer));
		if(length > 0){
			checkCode(receiveBuffer, length);
		}
#endif
		getMessages();	//TODO frägt alle windows ab, könnte evtl nicht nötig sein
	}

	//Aufräumen
	destroyPageNoFont(main_page);
	destroyFont(default_font);
	strcpy((char*)sendBuffer, "EXIT");
#ifndef NO_DEVICE
	sendPacket(hDevice, sendBuffer, sizeof("EXIT")-1);
	CloseHandle(hDevice);
#endif
	return 0;
}

LRESULT CALLBACK basic_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:{
		ErrCheck(setWindowState(hwnd, WINDOW_SHOULD_CLOSE), "set close window state");
		break;
	}
	case WM_SIZE:{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		ErrCheck(resizeWindow(hwnd, width, height, 2));
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
		if(!getButton(mouse, MOUSE_RMB)){

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

ErrCode switchPage(){
	switch(page_switch){
	case 1:
		switchToStartPage();
		page_switch = 0;
		break;
	case 2:
		switchToFreeTrainingPage();
		page_switch = 0;
		break;
	}
	return SUCCESS;
}

ErrCode switchToStartPage(){
	destroyPageNoFont(main_page);

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button texture laden");
	main_page.images[0] = buttonImage;
	main_page.image_count = 1;

	Menu* menu1 = new Menu;
	ivec2 pos = {100, 200};
	ivec2 size = {160, 40};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadFreeTrainingPage;
	menu1->buttons[0].text = "START";
	menu1->buttons[0].image = buttonImage;
	menu1->button_count = 1;
	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

#ifndef NO_DEVICE
	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);
#endif

	return SUCCESS;
}

ErrCode switchToFreeTrainingPage(){
	destroyPageNoFont(main_page);

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button texture laden");
	main_page.images[0] = buttonImage;
	main_page.image_count = 1;

	Menu* menu1 = new Menu;
	ivec2 pos = {50, 350};
	ivec2 size = {160, 40};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "BEENDEN";
	menu1->buttons[0].image = buttonImage;
	menu1->button_count = 1;

	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = 31;
	menu1->labels[1].pos = {20, 56};
	menu1->labels[1].text_size = 31;
	menu1->labels[2].pos = {20, 92};
	menu1->labels[2].text_size = 31;
	menu1->labels[3].pos = {20, 128};
	menu1->labels[3].text_size = 31;
	menu1->label_count = 4;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = displayDataPage;

	return SUCCESS;
}
