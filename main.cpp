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
#include "graphs.h"

#define NO_DEVICE

#ifndef NO_DEVICE
HANDLE hDevice = open_device("\\\\.\\COM6", 19200);
#endif
//TODO Ein paar Dinge hier sollten bei Zeit mal vom Stack runter, nicht das hier Böses passiert
BYTE receiveBuffer[128];
BYTE sendBuffer[128];
RequestQueue queue;
Page main_page;
BYTE page_select = 0;	//0 Startseite, 1 free training Seite
Font* default_font = nullptr;

LRESULT CALLBACK main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ErrCode loadStartPage(){setPageFlag(main_page, PAGE_LOAD); page_select = 0; return SUCCESS;};
ErrCode loadFreeTrainingPage(){setPageFlag(main_page, PAGE_LOAD); page_select = 1; return SUCCESS;};
ErrCode loadStatistikPage(){setPageFlag(main_page, PAGE_LOAD); page_select = 2; return SUCCESS;};
ErrCode switchToStartPage(HWND window);
ErrCode switchToFreeTrainingPage(HWND window);
ErrCode switchToStatistikPage(HWND window);
ErrCode handleSignals(HWND window);
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

void displayDataPage(HWND window){
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

void displayStatistics(HWND window){
	DataPoint dps[] = {{1, 2}, {2, 3}, {4, 4}, {5, 3}, {6, 3}}; WORD dps_count = 5;
	ErrCheck(drawGraph(window, 50, 50, 250, 250, dps, dps_count));
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	HWND main_window;
	ErrCheck(openWindow(hInstance, 800, 800, 2, main_window, "waterrower", main_window_callback), "open main window");

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

		ErrCheck(drawWindow(main_window), "draw window");

#ifndef NO_DEVICE
		transmitRequests(queue, hDevice);

		int length = readPacket(hDevice, receiveBuffer, sizeof(receiveBuffer));
		if(length > 0){
			checkCode(receiveBuffer, length);
		}
#endif
		handleSignals(main_window);
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

LRESULT CALLBACK main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:{
		ErrCheck(setWindowState(hwnd, WINDOW_CLOSE), "setze close Fensterstatus");
		break;
	}
	case WM_SIZE:{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		ErrCheck(setWindowState(hwnd, WINDOW_RESIZE), "setzte resize Fensterstatus");
		ErrCheck(resizeWindow(hwnd, width, height, 2), "Fenster skalieren");
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

ErrCode handleSignals(HWND window){
	getMessages();
	for(WORD i=0; i < app.window_count; ++i){
		HWND windowIter = app.windows[i];
		WINDOWSTATE state;
		while((state = getNextWindowState(windowIter))){
			switch(state){
			case WINDOW_CLOSE:
				closeWindow(windowIter);
				break;
			case WINDOW_RESIZE:
				setPageFlag(main_page, PAGE_LOAD);
				break;
			}
		}
	}
	PAGEFLAGSTYPE flag;
	while((flag = getNextPageFlag(main_page))){
		switch(flag){
		case PAGE_LOAD:
			switch(page_select){
			case 0:
				switchToStartPage(window);
				break;
			case 1:
				switchToFreeTrainingPage(window);
				break;
			case 2:
				switchToStatistikPage(window);
				break;
			}
			break;
		}
	}
	return SUCCESS;
}

ErrCode switchToStartPage(HWND window){
	destroyPageNoFont(main_page);

	WORD idx;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/waterrower.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(windowInfo.window_width/windowInfo.pixel_size*0.125), (int)(windowInfo.window_height/windowInfo.pixel_size*0.125)};
	ivec2 size = {(int)(windowInfo.window_height/windowInfo.pixel_size*0.1)*4, (int)(windowInfo.window_height/windowInfo.pixel_size*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadFreeTrainingPage;
	menu1->buttons[0].text = "Freies Training";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->buttons[1].pos = {pos.x, pos.y+size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125)};
	menu1->buttons[1].size = {size.x, size.y};
	menu1->buttons[1].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[1].pos.y-size.y*0.05)};
	menu1->buttons[1].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[1].event = loadStatistikPage;
	menu1->buttons[1].text = "Statistiken";
	menu1->buttons[1].image = buttonImage;
	menu1->buttons[1].textsize = size.y/2;
	menu1->button_count = 2;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

#ifndef NO_DEVICE
	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);
#endif

	return SUCCESS;
}

ErrCode switchToFreeTrainingPage(HWND window){
	destroyPageNoFont(main_page);

	WORD idx;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(windowInfo.window_width/windowInfo.pixel_size*0.125), windowInfo.window_height/windowInfo.pixel_size-(int)(windowInfo.window_height/windowInfo.pixel_size*0.2)};
	ivec2 size = {(int)(windowInfo.window_height/windowInfo.pixel_size*0.1)*4, (int)(windowInfo.window_height/windowInfo.pixel_size*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Beenden";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
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

ErrCode switchToStatistikPage(HWND window){
	destroyPageNoFont(main_page);

	WORD idx;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(windowInfo.window_width/windowInfo.pixel_size*0.125), windowInfo.window_height/windowInfo.pixel_size-(int)(windowInfo.window_height/windowInfo.pixel_size*0.2)};
	ivec2 size = {(int)(windowInfo.window_height/windowInfo.pixel_size*0.1)*4, (int)(windowInfo.window_height/windowInfo.pixel_size*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Startbildschirm";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->button_count = 1;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = displayStatistics;

	return SUCCESS;
}
