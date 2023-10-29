#include <iostream>
#include <winusb.h>
extern "C"{
	#include <hidsdi.h>
}
#include <setupapi.h>
#include <thread>
#include "window.h"
#include "usb.h"
#include "page.h"
#include "graphs.h"
#include "workout.h"
#include "simulation2D.h"

//#define NO_DEVICE

#define FPS 60
#define FPSMILLIS 1000/FPS

HINSTANCE ghInstance;
HANDLE hDevice = nullptr;
BYTE receiveBuffer[128];
BYTE sendBuffer[128];
RequestQueue queue;
Page main_page;
BYTE page_select = 0;
Font* default_font = nullptr;
//Workouts
Workout* workout = nullptr;
VirtualRowing2D* simulation2D = nullptr;

LRESULT CALLBACK main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ErrCode loadStartPage(BYTE*){setPageFlag(main_page, PAGE_LOAD); page_select = 0; return SUCCESS;}
ErrCode loadFreeTrainingPage(BYTE*){setPageFlag(main_page, PAGE_LOAD); page_select = 1; return SUCCESS;}
ErrCode loadStatistikPage(BYTE*){setPageFlag(main_page, PAGE_LOAD); page_select = 2; return SUCCESS;}
ErrCode loadWorkoutCreatePage(BYTE*){setPageFlag(main_page, PAGE_LOAD); page_select = 3; return SUCCESS;}
ErrCode loadOpenDevicePage(BYTE*){setPageFlag(main_page, PAGE_LOAD); page_select = 5; return SUCCESS;}
ErrCode loadWorkoutPage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	page_select = 4;
	initWorkout(*workout);
	return SUCCESS;
}
ErrCode switchToStartPage(HWND window);
ErrCode switchToFreeTrainingPage(HWND window);
ErrCode switchToStatistikPage(HWND window);
ErrCode switchToCreateWorkoutPage(HWND window);
ErrCode switchToWorkoutPage(HWND window);
ErrCode switchToOpenDevicePage(HWND window);
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
		ErrCheck(addRequest(queue, 10));
		ErrCheck(addRequest(queue, 4));
		ErrCheck(addRequest(queue, 5));
		//TODO kann einmalig mit ES_CONTINUOUS | ... aufgerufen werden und danach wieder ES_CONTINUOUS gel�scht werden
		if(!SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED)){
			std::cerr << "Konnte thread execution status nicht setzen!" << std::endl;
		}
	}
}

void renderFunc(){
	HWND main_window;
	if(ErrCheck(openWindow(ghInstance, 800, 800, 100, 50, 1, main_window, "waterrower", main_window_callback), "open main window") != SUCCESS){
		resetAppFlag(APP_RUNNING);
		return;
	};
	while(getAppFlag(APP_RUNNING)){
		SYSTEMTIME t1;
		GetSystemTime(&t1);
		clearWindows();
		updatePage(main_page, main_window);
		drawWindows();
		handleSignals(main_window);
		getMessages();
		SYSTEMTIME t2;
		GetSystemTime(&t2);
		unsigned long long millis = t2.wMilliseconds-t1.wMilliseconds+(t2.wMinute-t1.wMinute)*60+(t2.wHour-t1.wHour)*3600;
		if(millis < FPSMILLIS) Sleep(FPSMILLIS-millis);
	}
	return;
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	ghInstance = hInstance;
	if(ErrCheck(initApp(), "App �ffnen") != SUCCESS) return -1;

//	initCommunication(hDevice, sendBuffer, receiveBuffer);

	//TODO remove
	const char* boat_images[] = {"textures/boat.tex", "textures/boat.tex", "textures/boat.tex", "textures/boat.tex"};
	createVirtualRowing2D(simulation2D, boat_images, 4);

	default_font = new Font;
	ErrCheck(loadFont("fonts/ascii.tex", *default_font, {82, 83}), "font laden");
	default_font->font_size = 31;
	main_page.font = default_font;

	ErrCheck(loadStartPage(nullptr), "laden des Startbildschirms");
	setAppFlag(APP_RUNNING);

	std::thread render_thread(renderFunc);

	while(getAppFlag(APP_RUNNING)){
		if(getAppFlag(APP_HAS_DEVICE)){
			transmitRequests(queue, hDevice);

			int length = readPacket(hDevice, receiveBuffer, sizeof(receiveBuffer));
			if(length > 0){
				checkCode(receiveBuffer, length);
			}
		}
	}

	//TODO remove
	destroyVirtualRowing2D(simulation2D);

	//Threads abfangen
	render_thread.join();
	//Aufr�umen
	destroyPageNoFont(main_page);
	destroyFont(default_font);
	destroyWorkout(workout);
	if(getAppFlag(APP_HAS_DEVICE)){
		strcpy((char*)sendBuffer, "EXIT");
		sendPacket(hDevice, sendBuffer, sizeof("EXIT")-1);
		closeDevice(hDevice);
	}
	ErrCheck(closeApp(), "App schlie�en");
	return 0;
}

LRESULT CALLBACK main_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:{
		ErrCheck(setWindowFlag(hwnd, WINDOW_CLOSE), "setze close Fensterstatus");
		break;
	}
	case WM_SIZE:{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if(!width || !height) break;
		ErrCheck(setWindowFlag(hwnd, WINDOW_RESIZE), "setze resize Fensterstatus");
		ErrCheck(resizeWindow(hwnd, width, height, 1), "Fenster skalieren");
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
	case WM_KEYDOWN:{
		switch(wParam){
		case 0:
			break;
		}
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ErrCode handleSignals(HWND window){
	for(int i=0; i < app.window_count; ++i){
		HWND windowIter = app.windows[i];
		WINDOWFLAGS state;
		while((state = getNextWindowState(windowIter))){
			switch(state){
			case WINDOW_CLOSE:
				ErrCheck(closeWindow(windowIter), "Fenster schlie�en");
				if(app.window_count == 0) resetAppFlag(APP_RUNNING);
				--i;
				goto whileend;
				break;
			case WINDOW_RESIZE:
				setPageFlag(main_page, PAGE_LOAD);
				break;
			default:
				break;
			}
		}
		whileend:;
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
			case 3:
				switchToCreateWorkoutPage(window);
				break;
			case 4:
				switchToWorkoutPage(window);
				break;
			case 5:
				switchToOpenDevicePage(window);
				break;
			}
		}
	}
	return SUCCESS;
}

ErrCode switchToStartPage(HWND window){
	destroyPageNoFont(main_page);
	destroyWorkout(workout);

	for(WORD i=1; i < app.window_count; ++i){
		ErrCheck(setWindowFlag(app.windows[i], WINDOW_CLOSE));
	}

	//TODO meh...
	initRowingData(rowingData);

	WORD idx = 0;
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
	Image* disabledButtonImage = new Image;
	ErrCheck(loadImage("textures/button_disabled.tex", *disabledButtonImage), "ausgeschalteter button image laden");
	menu1->images[1] = disabledButtonImage;
	menu1->image_count = 2;

	ivec2 pos = {(int)(windowInfo.window_width/windowInfo.pixel_size*0.125), (int)(windowInfo.window_height/windowInfo.pixel_size*0.125)};
	ivec2 size = {(int)(windowInfo.window_height/windowInfo.pixel_size*0.1)*4, (int)(windowInfo.window_height/windowInfo.pixel_size*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadFreeTrainingPage;
	menu1->buttons[0].text = "Freies Training";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].disabled_image = disabledButtonImage;
	menu1->buttons[0].textsize = size.y/2;
	if(!getAppFlag(APP_HAS_DEVICE)) setButtonFlag(menu1->buttons[0], BUTTON_DISABLED);
	menu1->buttons[1].pos = {pos.x, pos.y+size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125)};
	menu1->buttons[1].size = {size.x, size.y};
	menu1->buttons[1].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[1].pos.y-size.y*0.05)};
	menu1->buttons[1].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[1].event = loadStatistikPage;
	menu1->buttons[1].text = "Statistiken";
	menu1->buttons[1].image = buttonImage;
	menu1->buttons[1].disabled_image = disabledButtonImage;
	menu1->buttons[1].textsize = size.y/2;
	menu1->buttons[2].pos = {pos.x, pos.y+size.y*2+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125*2)};
	menu1->buttons[2].size = {size.x, size.y};
	menu1->buttons[2].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[2].pos.y-size.y*0.05)};
	menu1->buttons[2].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[2].event = loadWorkoutCreatePage;
	menu1->buttons[2].text = "Workouts";
	menu1->buttons[2].image = buttonImage;
	menu1->buttons[2].disabled_image = disabledButtonImage;
	menu1->buttons[2].textsize = size.y/2;
	if(!getAppFlag(APP_HAS_DEVICE)) setButtonFlag(menu1->buttons[2], BUTTON_DISABLED);
	menu1->buttons[3].pos = {pos.x, pos.y+size.y*3+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125*3)};
	menu1->buttons[3].size = {size.x, size.y};
	menu1->buttons[3].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[3].pos.y-size.y*0.05)};
	menu1->buttons[3].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[3].event = loadOpenDevicePage;
	menu1->buttons[3].text = "Geraete";
	menu1->buttons[3].image = buttonImage;
	menu1->buttons[3].disabled_image = disabledButtonImage;
	menu1->buttons[3].textsize = size.y/2;
	menu1->button_count = 4;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

	return SUCCESS;
}

void displayDataPage(HWND window){
#ifndef NO_DEVICE
		refreshData(queue);
#endif
	main_page.menus[0]->labels[0].text = "Distanz: " + intToString(rowingData.dist.upper, 0) + '.' + intToString(rowingData.dist.lower, 0) + 'm';
	main_page.menus[0]->labels[1].text = "Geschwindigkeit: " + intToString(rowingData.ms_total) + "m/s";
	main_page.menus[0]->labels[2].text = "Durchschnittlich: " + intToString(rowingData.ms_avg) + "m/s";
	main_page.menus[0]->labels[3].text = "Zeit: " + intToString(rowingData.time.hrs, 0) + ':' + intToString(rowingData.time.min, 0) + ':' + intToString(rowingData.time.sec, 0);
	if(app.window_count > 1){
		WORD offset = 0;
		WORD window_idx;
		WORD fontSize = default_font->font_size;
		default_font->font_size = 48;
		if(ErrCheck(getWindow(*(HWND*)main_page.data, window_idx)) != SUCCESS) return;
		for(size_t i=0; i < main_page.menus[0]->labels[0].text.size(); ++i){
			offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[0].text[i], 10+offset, 10);
		}
		offset = 0;
		for(size_t i=0; i < main_page.menus[0]->labels[2].text.size(); ++i){
			offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[2].text[i], 10+offset, default_font->font_size+20);
		}
		offset = 0;
		for(size_t i=0; i < main_page.menus[0]->labels[3].text.size(); ++i){
			offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[3].text[i], 10+offset, default_font->font_size*2+30);
		}
		default_font->font_size = fontSize;
	}
}
ErrCode endFreeTraining(BYTE*){
	if(rowingData.dist.upper){
		Statistic s1;
		s1.distance = rowingData.dist.upper;
		GetSystemTime(&s1.time);
		Statistic s2;
		readStatistics(&s2, 1);
		if(s2.time.wYear == s1.time.wYear && s2.time.wMonth == s1.time.wMonth && s2.time.wDay == s1.time.wDay){
			s1.distance += s2.distance;
			saveStatistic(s1, true);
		}else{
			saveStatistic(s1, false);
		}
	}
	ErrCheck(loadStartPage(nullptr), "freies Training zu Startseite");
	return SUCCESS;
}
ErrCode switchToFreeTrainingPage(HWND window){
	destroyPageNoFont(main_page);

#ifndef NO_DEVICE
	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);
#endif

	//TODO �ffne ein Overlayfenster (nach Wunsch) auf welchem die Ruderdaten angezeigt werden
	if(app.window_count <= 1){
		HWND overlay_window;
		if(ErrCheck(openWindow(ghInstance, 800, 800, 0, 0, 1, overlay_window, "waterrower", main_window_callback, window), "Overlayfenster �ffnen") != SUCCESS){
			resetAppFlag(APP_RUNNING);
			return GENERIC_ERROR;		//Fehler wird oben schon ausgegeben
		};
		SetWindowLong(overlay_window, GWL_STYLE, WS_VISIBLE);
		SetWindowLong(overlay_window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT);
		SetLayeredWindowAttributes(overlay_window, RGB(0, 0, 0), 0, LWA_COLORKEY);
		SetWindowPos(overlay_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		allocPageData(main_page, &overlay_window, sizeof(overlay_window));
	}

	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

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
	menu1->buttons[0].event = endFreeTraining;
	menu1->buttons[0].text = "Beenden";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->button_count = 1;

	WORD w = windowInfo.window_width/windowInfo.pixel_size;
	WORD label_text_size = w*0.0775;
	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = label_text_size;
	menu1->labels[1].pos = {20, 20+label_text_size+(WORD)(label_text_size*0.16)};
	menu1->labels[1].text_size = label_text_size;
	menu1->labels[2].pos = {20, 20+label_text_size*2+(WORD)(label_text_size*0.16*2)};
	menu1->labels[2].text_size = label_text_size;
	menu1->labels[3].pos = {20, 20+label_text_size*3+(WORD)(label_text_size*0.16*3)};
	menu1->labels[3].text_size = label_text_size;
	menu1->label_count = 4;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = displayDataPage;

	return SUCCESS;
}

void displayStatistics(HWND window){
	Statistic s[10] = {};
	DataPoint dps[10];
	ErrCheck(readStatistics(s, 10));
	for(int i=0; i < 10; ++i){
		dps[i].x = i;
		dps[i].y = s[9-i].distance;
	}
	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WORD width = app.info[idx].window_width/app.info[idx].pixel_size;
	WORD height = app.info[idx].window_height/app.info[idx].pixel_size;
	ErrCheck(drawGraph(window, *default_font, width/2-width*0.625/2, height*0.0625, width*0.625, height*0.625, dps, 10));
	main_page.menus[0]->labels[0].pos = {(int)(width/2-width*0.625/2), (int)(height*0.0625+height*0.625+20)};
	main_page.menus[0]->labels[0].text = "Wasservolumen: " + intToString(rowingData.volume) + 'L';
}
ErrCode switchToStatistikPage(HWND window){
	destroyPageNoFont(main_page);

	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

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

	WORD w = windowInfo.window_width/windowInfo.pixel_size;
	WORD label_text_size = w*0.0775;
	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = label_text_size;
	menu1->label_count = 1;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	ErrCheck(addRequest(queue, 9));

	main_page.code = displayStatistics;

	return SUCCESS;
}

ErrCode incWorkoutTime(BYTE*){
	workout->duration += 60;
	main_page.menus[0]->labels[0].text = "Dauer: " + std::to_string(workout->duration/60) + "min";
	return SUCCESS;
}
ErrCode decWorkoutTime(BYTE*){
	if(workout->duration > 60) workout->duration -= 60;
	main_page.menus[0]->labels[0].text = "Dauer: " + std::to_string(workout->duration/60) + "min";
	return SUCCESS;
}
ErrCode incIntensityTime(BYTE*){
	workout->intensity += 10;
	setWorkoutFlag(*workout, WORKOUT_INTENSITY);
	main_page.menus[0]->labels[1].text = "Zielgeschw.: " + intToString(workout->intensity) + "m/s";
	return SUCCESS;
}
ErrCode decIntensityTime(BYTE*){
	if(workout->intensity >= 10) workout->intensity -= 10;
	if(workout->intensity == 0) resetWorkoutFlag(*workout, WORKOUT_INTENSITY);
	main_page.menus[0]->labels[1].text = "Zielgeschw.: " + intToString(workout->intensity) + "m/s";
	return SUCCESS;
}
ErrCode toggleSimulation(BYTE*){
	workout->flags ^= WORKOUT_SIMULATION;
	if(getWorkoutFlag(*workout, WORKOUT_SIMULATION)) main_page.menus[0]->buttons[6].text = "Simulation";
	else main_page.menus[0]->buttons[6].text = "Keine Simulation";
	return SUCCESS;
}
ErrCode switchToCreateWorkoutPage(HWND window){
	destroyPageNoFont(main_page);
	destroyWorkout(workout);
	createWorkout(workout);

	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

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
	pos.y -= size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125);
	menu1->buttons[1].pos = {pos.x, pos.y};
	menu1->buttons[1].size = {size.x, size.y};
	menu1->buttons[1].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[1].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[1].event = loadWorkoutPage;
	menu1->buttons[1].text = "Starte Workout";
	menu1->buttons[1].image = buttonImage;
	menu1->buttons[1].textsize = size.y/2;
	menu1->button_count = 7;

	WORD w = windowInfo.window_width/windowInfo.pixel_size;
	WORD label_text_size = w*0.038;
	pos = {(int)(windowInfo.window_width/windowInfo.pixel_size*0.125), (int)(windowInfo.window_height/windowInfo.pixel_size*0.125)};
	menu1->labels[0].pos = {pos.x, pos.y};
	menu1->labels[0].text = "Dauer: " + std::to_string(workout->duration/60) + "min";
	menu1->labels[0].text_size = label_text_size;
	menu1->labels[1].pos = {pos.x, pos.y+size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125)};
	menu1->labels[1].text = "Zielgeschw.: " + intToString(workout->intensity) + "m/s";
	menu1->labels[1].text_size = label_text_size;
	menu1->label_count = 2;
	pos.x += (int)(windowInfo.window_width/windowInfo.pixel_size*0.375);
	menu1->buttons[2].pos = {pos.x, pos.y};
	menu1->buttons[2].size = {size.y, size.y};
	menu1->buttons[2].repos = {(int)(pos.x-size.y*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[2].resize = {(int)(size.y+size.y*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[2].event = incWorkoutTime;
	menu1->buttons[2].text = "+";
	menu1->buttons[2].image = buttonImage;
	menu1->buttons[2].textsize = size.y/2;
	pos.x += size.y;
	menu1->buttons[3].pos = {pos.x, pos.y};
	menu1->buttons[3].size = {size.y, size.y};
	menu1->buttons[3].repos = {(int)(pos.x-size.y*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[3].resize = {(int)(size.y+size.y*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[3].event = decWorkoutTime;
	menu1->buttons[3].text = "-";
	menu1->buttons[3].image = buttonImage;
	menu1->buttons[3].textsize = size.y/2;

	pos.x -= size.y;
	pos.y += size.y;
	menu1->buttons[4].pos = {pos.x, pos.y};
	menu1->buttons[4].size = {size.y, size.y};
	menu1->buttons[4].repos = {(int)(pos.x-size.y*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[4].resize = {(int)(size.y+size.y*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[4].event = incIntensityTime;
	menu1->buttons[4].text = "+";
	menu1->buttons[4].image = buttonImage;
	menu1->buttons[4].textsize = size.y/2;
	pos.x += size.y;
	menu1->buttons[5].pos = {pos.x, pos.y};
	menu1->buttons[5].size = {size.y, size.y};
	menu1->buttons[5].repos = {(int)(pos.x-size.y*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[5].resize = {(int)(size.y+size.y*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[5].event = decIntensityTime;
	menu1->buttons[5].text = "-";
	menu1->buttons[5].image = buttonImage;
	menu1->buttons[5].textsize = size.y/2;

	//Optionsbuttons
	pos.x = (int)(windowInfo.window_width/windowInfo.pixel_size*0.125);
	pos.y += size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125);
	menu1->buttons[6].pos = {pos.x, pos.y};
	menu1->buttons[6].size = {size.x, size.y};
	menu1->buttons[6].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[6].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[6].event = toggleSimulation;
	menu1->buttons[6].text = "Keine Simulation";
	menu1->buttons[6].image = buttonImage;
	menu1->buttons[6].textsize = size.y/2;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

	return SUCCESS;
}

void runWorkout(HWND window){
#ifndef NO_DEVICE
		refreshData(queue);
//		if(!getWorkoutFlag(*workout, WORKOUT_RUNNING)) return;
#endif
	if(!updateWorkout(*workout, rowingData.dist.upper - workout->distance)){
		main_page.menus[0]->labels[0].text = "Distanz: " + intToString(rowingData.dist.upper, 0) + '.' + intToString(rowingData.dist.lower, 0) + 'm';
		main_page.menus[0]->labels[2].text = "Zielgeschwindigkeit: " + intToString(workout->intensity) + "m/s";
		main_page.menus[0]->labels[1].text = "Durchschnittlich: " + intToString(rowingData.ms_avg) + "m/s";
		if(getWorkoutFlag(*workout, WORKOUT_INTENSITY)){
			std::string intens;
			if((workout->intensity-workout->intensity*0.02) > rowingData.ms_avg) intens = "+";
			else if((workout->intensity+workout->intensity*0.02) < rowingData.ms_avg) intens = "-";
			else intens = "";
			main_page.menus[0]->labels[1].text = "Durchschnittlich: " + intToString(rowingData.ms_avg) + "m/s " + intens;
		}
		WORD hrs = workout->duration/3600;
		WORD min = (workout->duration/60)%60;
		WORD sec = workout->duration%60;
		main_page.menus[0]->labels[3].text = "Zeit: " + intToString(hrs, 0) + ':' + intToString(min, 0) + ':' + intToString(sec, 0);

		if(getWorkoutFlag(*workout, WORKOUT_SIMULATION)) updateVirtualRowing2D(*simulation2D, window, *default_font, rowingData.ms_avg);
		if(getWorkoutFlag(*workout, WORKOUT_OVERLAY) && app.window_count > 1){
			WORD offset = 0;
			WORD window_idx;
			WORD fontSize = default_font->font_size;
			default_font->font_size = 48;
			if(ErrCheck(getWindow(*(HWND*)main_page.data, window_idx)) != SUCCESS) return;
			for(size_t i=0; i < main_page.menus[0]->labels[0].text.size(); ++i){
				offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[0].text[i], 10+offset, 10);
			}
			offset = 0;
			for(size_t i=0; i < main_page.menus[0]->labels[1].text.size(); ++i){
				offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[1].text[i], 10+offset, default_font->font_size+20);
			}
			offset = 0;
			for(size_t i=0; i < main_page.menus[0]->labels[3].text.size(); ++i){
				offset += drawFontChar(window_idx, *default_font, main_page.menus[0]->labels[3].text[i], 10+offset, default_font->font_size*2+30);
			}
			default_font->font_size = fontSize;
		}

	}else{
		if(getWorkoutFlag(*workout, WORKOUT_DONE)){
			Statistic s1;
			s1.distance = workout->distance;
			GetSystemTime(&s1.time);
			Statistic s2;
			readStatistics(&s2, 1);
			if(s2.time.wYear == s1.time.wYear && s2.time.wMonth == s1.time.wMonth && s2.time.wDay == s1.time.wDay){
				s1.distance += s2.distance;
				saveStatistic(s1, true);
			}else{
				saveStatistic(s1, false);
			}
			setWorkoutFlag(*workout, WORKOUT_DONE);
		}
	}
}
ErrCode switchToWorkoutPage(HWND window){
	destroyPageNoFont(main_page);

	workout->distance = 0;
	resetWorkoutFlag(*workout, WORKOUT_RUNNING);

	//TODO Sollte eine Option sein
	setWorkoutFlag(*workout, WORKOUT_INTENSITY);
	if(getWorkoutFlag(*workout, WORKOUT_SIMULATION)) initVirtualRowing2D(*simulation2D, workout->intensity);

	//TODO �ffne ein Overlayfenster (nach Wunsch) auf welchem die Ruderdaten angezeigt werden
	if(app.window_count <= 1){
		HWND overlay_window;
		if(ErrCheck(openWindow(ghInstance, 800, 800, 0, 0, 1, overlay_window, "waterrower", main_window_callback, window), "Overlayfenster �ffnen") != SUCCESS){
			resetAppFlag(APP_RUNNING);
			return GENERIC_ERROR;		//Fehler wird oben schon ausgegeben
		};
		SetWindowLong(overlay_window, GWL_STYLE, WS_VISIBLE);
		SetWindowLong(overlay_window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT);
		SetLayeredWindowAttributes(overlay_window, RGB(0, 0, 0), 0, LWA_COLORKEY);
		SetWindowPos(overlay_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		setWorkoutFlag(*workout, WORKOUT_OVERLAY);
		allocPageData(main_page, &overlay_window, sizeof(overlay_window));
	}

#ifndef NO_DEVICE
	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);
#endif

	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

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

	WORD w = windowInfo.window_width/windowInfo.pixel_size;
	WORD label_text_size = w*0.068;
	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = label_text_size;
	menu1->labels[1].pos = {20, 20+label_text_size+(WORD)(label_text_size*0.16)};
	menu1->labels[1].text_size = label_text_size;
	menu1->labels[2].pos = {20, 20+label_text_size*2+(WORD)(label_text_size*0.16*2)};
	menu1->labels[2].text_size = label_text_size;
	menu1->labels[3].pos = {20, 20+label_text_size*3+(WORD)(label_text_size*0.16*3)};
	menu1->labels[3].text_size = label_text_size;
	menu1->label_count = 4;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = runWorkout;

	return SUCCESS;
}

ErrCode selectDevice(BYTE* data){
	std::string port = "\\\\.\\COM" + std::to_string(*data);
	if(ErrCheck(openDevice(hDevice, port.c_str(), 19200)) == SUCCESS){
		setAppFlag(APP_HAS_DEVICE);
		loadStartPage(nullptr);
	}
	return SUCCESS;
}
ErrCode switchToOpenDevicePage(HWND window){
	destroyPageNoFont(main_page);

	resetAppFlag(APP_HAS_DEVICE);
	closeDevice(hDevice);

	WORD idx = 0;
	ErrCheck(getWindow(window, idx));
	WindowInfo& windowInfo = app.info[idx];

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {windowInfo.window_width/windowInfo.pixel_size, windowInfo.window_height/windowInfo.pixel_size};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	//TODO testet nur Port 1-10 f�r Ger�te
	BYTE deviceNumbers[10];
	BYTE device_count = 0;
	for(WORD i=1; i <= 10; ++i){
		std::string port = "\\\\.\\COM" + std::to_string(i);
		if(openDevice(hDevice, port.c_str(), 19200) == SUCCESS){
			deviceNumbers[device_count++] = i;
			closeDevice(hDevice);
		}
	}

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

	pos.y = (int)(windowInfo.window_height/windowInfo.pixel_size*0.125);
	for(BYTE i=1; i <= device_count; ++i){
		menu1->buttons[i].pos = {pos.x, pos.y};
		menu1->buttons[i].size = {size.x, size.y};
		menu1->buttons[i].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
		menu1->buttons[i].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
		menu1->buttons[i].event = selectDevice;
		menu1->buttons[i].text = "Port: " + std::to_string((int)deviceNumbers[i-1]);
		menu1->buttons[i].data = new BYTE;
		*menu1->buttons[i].data = deviceNumbers[i-1];
		menu1->buttons[i].image = buttonImage;
		menu1->buttons[i].textsize = size.y/2;
		pos.y += size.y+(int)(windowInfo.window_height/windowInfo.pixel_size*0.0125);
	}
	menu1->button_count = 1+device_count;

	WORD w = windowInfo.window_width/windowInfo.pixel_size;
	WORD label_text_size = w*0.0775;
	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = label_text_size;
	menu1->label_count = 1;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

	return SUCCESS;
}
