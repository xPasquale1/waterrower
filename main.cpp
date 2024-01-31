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

ErrCode loadStartPage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 0;
	return SUCCESS;
}
ErrCode loadFreeTrainingPage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 1;
	return SUCCESS;
}
ErrCode loadStatistikPage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 2;
	return SUCCESS;
}
ErrCode loadWorkoutCreatePage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 3;
	return SUCCESS;
}
ErrCode loadOpenDevicePage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 5;
	return SUCCESS;
}
ErrCode loadWorkoutPage(BYTE*){
	setPageFlag(main_page, PAGE_LOAD);
	resetButton(mouse, MOUSE_LMB);
	page_select = 4;
	initWorkout(*workout);
	return SUCCESS;
}
ErrCode switchToStartPage(Window* window);
ErrCode switchToFreeTrainingPage(Window* window);
ErrCode switchToStatistikPage(Window* window);
ErrCode switchToCreateWorkoutPage(Window* window);
ErrCode switchToWorkoutPage(Window* window);
ErrCode switchToOpenDevicePage(Window* window);
ErrCode handleSignals(Window* window);
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
		//TODO kann einmalig mit ES_CONTINUOUS | ... aufgerufen werden und danach wieder ES_CONTINUOUS gelöscht werden
		if(!SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED)){
			std::cerr << "Konnte thread execution status nicht setzen!" << std::endl;
		}
	}
}

//TODO overlay Fenster sollten hier irgendwie auch noch rein kommen dürfen (globale variable oder besser page data)
void renderFunc(){
	Window* main_window = nullptr;
	if(ErrCheck(createWindow(ghInstance, 800, 800, 100, 50, 1, main_window, "waterrower"), "open main window") != SUCCESS){
		resetAppFlag(APP_RUNNING);
		return;
	};
	while(getAppFlag(APP_RUNNING)){
		SYSTEMTIME t1;
		GetSystemTime(&t1);

		clearWindow(main_window);
		updatePage(main_page, main_window);
		drawWindow(main_window);

		getMessages(main_window);
		handleSignals(main_window);

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
	//Aufräumen
	destroyPageNoFont(main_page);
	destroyFont(default_font);
	destroyWorkout(workout);
	if(getAppFlag(APP_HAS_DEVICE)){
		strcpy((char*)sendBuffer, "EXIT");
		sendPacket(hDevice, sendBuffer, sizeof("EXIT")-1);
		closeDevice(hDevice);
	}
	ErrCheck(destroyApp(), "App schließen");
	return 0;
}

ErrCode handleSignals(Window* window){
	WINDOWFLAG state;
	while((state = getNextWindowState(window))){
		switch(state){
		case WINDOW_CLOSE:
			ErrCheck(destroyWindow(window), "Fenster schließen");
			resetAppFlag(APP_RUNNING);
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

ErrCode switchToStartPage(Window* window){
	destroyPageNoFont(main_page);
	destroyWorkout(workout);

	//TODO meh...
	initRowingData(rowingData);

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/waterrower.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	Image* disabledButtonImage = new Image;
	ErrCheck(loadImage("textures/button_disabled.tex", *disabledButtonImage), "ausgeschalteter button image laden");
	menu1->images[1] = disabledButtonImage;
	menu1->image_count = 2;

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), (int)(window->windowHeight/window->pixelSize*0.125)};
	ivec2 size = {(int)(window->windowWidth/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
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
	menu1->buttons[1].pos = {pos.x, pos.y+size.y+(int)(window->windowHeight/window->pixelSize*0.0125)};
	menu1->buttons[1].size = {size.x, size.y};
	menu1->buttons[1].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[1].pos.y-size.y*0.05)};
	menu1->buttons[1].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[1].event = loadStatistikPage;
	menu1->buttons[1].text = "Statistiken";
	menu1->buttons[1].image = buttonImage;
	menu1->buttons[1].disabled_image = disabledButtonImage;
	menu1->buttons[1].textsize = size.y/2;
	menu1->buttons[2].pos = {pos.x, pos.y+size.y*2+(int)(window->windowHeight/window->pixelSize*0.0125*2)};
	menu1->buttons[2].size = {size.x, size.y};
	menu1->buttons[2].repos = {(int)(pos.x-size.x*0.05), (int)(menu1->buttons[2].pos.y-size.y*0.05)};
	menu1->buttons[2].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[2].event = loadWorkoutCreatePage;
	menu1->buttons[2].text = "Workouts";
	menu1->buttons[2].image = buttonImage;
	menu1->buttons[2].disabled_image = disabledButtonImage;
	menu1->buttons[2].textsize = size.y/2;
	if(!getAppFlag(APP_HAS_DEVICE)) setButtonFlag(menu1->buttons[2], BUTTON_DISABLED);
	menu1->buttons[3].pos = {pos.x, pos.y+size.y*3+(int)(window->windowHeight/window->pixelSize*0.0125*3)};
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

void displayDataPage(Window* window){
	refreshData(queue);
	main_page.menus[0]->labels[0].text = "Distanz: " + intToString(rowingData.dist.upper, 0) + '.' + intToString(rowingData.dist.lower, 0) + 'm';
	main_page.menus[0]->labels[1].text = "Geschwindigkeit: " + intToString(rowingData.ms_total) + "m/s";
	main_page.menus[0]->labels[2].text = "Durchschnittlich: " + intToString(rowingData.ms_avg) + "m/s";
	main_page.menus[0]->labels[3].text = "Zeit: " + intToString(rowingData.time.hrs, 0) + ':' + intToString(rowingData.time.min, 0) + ':' + intToString(rowingData.time.sec, 0);
	
	Window* statWindow = (Window*)main_page.data;
	if(statWindow == nullptr) return;
	WORD offset = 0;
	WORD window_idx;
	WORD fontSize = default_font->font_size;
	default_font->font_size = 48;
	for(size_t i=0; i < main_page.menus[0]->labels[0].text.size(); ++i){
		offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[0].text[i], 10+offset, 10);
	}
	offset = 0;
	for(size_t i=0; i < main_page.menus[0]->labels[2].text.size(); ++i){
		offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[2].text[i], 10+offset, default_font->font_size+20);
	}
	offset = 0;
	for(size_t i=0; i < main_page.menus[0]->labels[3].text.size(); ++i){
		offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[3].text[i], 10+offset, default_font->font_size*2+30);
	}
	default_font->font_size = fontSize;
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
ErrCode switchToFreeTrainingPage(Window* window){
	destroyPageNoFont(main_page);

	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);

	Window* overlay_window = nullptr;
	if(ErrCheck(createWindow(ghInstance, 800, 800, 0, 0, 1, overlay_window, "waterrower", default_window_callback, window->handle), "Overlayfenster öffnen") != SUCCESS){
		resetAppFlag(APP_RUNNING);
		return GENERIC_ERROR;		//Fehler wird oben schon ausgegeben
	};
	SetWindowLong(overlay_window->handle, GWL_STYLE, WS_VISIBLE);
	SetWindowLong(overlay_window->handle, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(overlay_window->handle, RGB(0, 0, 0), 0, LWA_COLORKEY);
	SetWindowPos(overlay_window->handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	allocPageData(main_page, overlay_window, sizeof(overlay_window));

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), window->windowHeight/window->pixelSize-(int)(window->windowHeight/window->pixelSize*0.2)};
	ivec2 size = {(int)(window->windowHeight/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = endFreeTraining;
	menu1->buttons[0].text = "Beenden";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->button_count = 1;

	WORD w = window->windowWidth/window->pixelSize;
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

void displayStatistics(Window* window){
	Statistic s[10] = {};
	DataPoint dps[10];
	ErrCheck(readStatistics(s, 10));
	for(int i=0; i < 10; ++i){
		dps[i].x = i;
		dps[i].y = s[9-i].distance;
	}
	WORD width = window->windowWidth/window->pixelSize;
	WORD height = window->windowHeight/window->pixelSize;
	ErrCheck(drawGraph(window, *default_font, width/2-width*0.625/2, height*0.0625, width*0.625, height*0.625, dps, 10));
	main_page.menus[0]->labels[0].pos = {(int)(width/2-width*0.625/2), (int)(height*0.0625+height*0.625+20)};
	main_page.menus[0]->labels[0].text = "Wasservolumen: " + intToString(rowingData.volume) + 'L';
}
ErrCode switchToStatistikPage(Window* window){
	destroyPageNoFont(main_page);

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), window->windowHeight/window->pixelSize-(int)(window->windowHeight/window->pixelSize*0.2)};
	ivec2 size = {(int)(window->windowHeight/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Startbildschirm";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->button_count = 1;

	WORD w = window->windowWidth/window->pixelSize;
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
ErrCode switchToCreateWorkoutPage(Window* window){
	destroyPageNoFont(main_page);
	destroyWorkout(workout);
	createWorkout(workout);

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), window->windowHeight/window->pixelSize-(int)(window->windowHeight/window->pixelSize*0.2)};
	ivec2 size = {(int)(window->windowHeight/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Startbildschirm";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	pos.y -= size.y+(int)(window->windowHeight/window->pixelSize*0.0125);
	menu1->buttons[1].pos = {pos.x, pos.y};
	menu1->buttons[1].size = {size.x, size.y};
	menu1->buttons[1].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[1].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[1].event = loadWorkoutPage;
	menu1->buttons[1].text = "Starte Workout";
	menu1->buttons[1].image = buttonImage;
	menu1->buttons[1].textsize = size.y/2;
	menu1->button_count = 7;

	WORD w = window->windowWidth/window->pixelSize;
	WORD label_text_size = w*0.038;
	pos = {(int)(window->windowWidth/window->pixelSize*0.125), (int)(window->windowHeight/window->pixelSize*0.125)};
	menu1->labels[0].pos = {pos.x, pos.y};
	menu1->labels[0].text = "Dauer: " + std::to_string(workout->duration/60) + "min";
	menu1->labels[0].text_size = label_text_size;
	menu1->labels[1].pos = {pos.x, pos.y+size.y+(int)(window->windowHeight/window->pixelSize*0.0125)};
	menu1->labels[1].text = "Zielgeschw.: " + intToString(workout->intensity) + "m/s";
	menu1->labels[1].text_size = label_text_size;
	menu1->label_count = 2;
	pos.x += (int)(window->windowWidth/window->pixelSize*0.375);
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
	pos.x = (int)(window->windowWidth/window->pixelSize*0.125);
	pos.y += size.y+(int)(window->windowHeight/window->pixelSize*0.0125);
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

void runWorkout(Window* window){
	refreshData(queue);
	// if(!getWorkoutFlag(*workout, WORKOUT_RUNNING)) return;
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
		
		Window* statWindow = (Window*)main_page.data;
		if(!getWorkoutFlag(*workout, WORKOUT_OVERLAY) || statWindow == nullptr) return;
		WORD offset = 0;
		WORD window_idx;
		WORD fontSize = default_font->font_size;
		default_font->font_size = 48;
		if(statWindow == nullptr) return;
		for(size_t i=0; i < main_page.menus[0]->labels[0].text.size(); ++i){
			offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[0].text[i], 10+offset, 10);
		}
		offset = 0;
		for(size_t i=0; i < main_page.menus[0]->labels[1].text.size(); ++i){
			offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[1].text[i], 10+offset, default_font->font_size+20);
		}
		offset = 0;
		for(size_t i=0; i < main_page.menus[0]->labels[3].text.size(); ++i){
			offset += drawFontChar(statWindow, *default_font, main_page.menus[0]->labels[3].text[i], 10+offset, default_font->font_size*2+30);
		}
		default_font->font_size = fontSize;

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
ErrCode switchToWorkoutPage(Window* window){
	destroyPageNoFont(main_page);

	workout->distance = 0;
	resetWorkoutFlag(*workout, WORKOUT_RUNNING);

	//TODO Sollte eine Option sein
	setWorkoutFlag(*workout, WORKOUT_INTENSITY);
	if(getWorkoutFlag(*workout, WORKOUT_SIMULATION)) initVirtualRowing2D(*simulation2D, workout->intensity);

	//TODO öffne ein Overlayfenster (nach Wunsch) auf welchem die Ruderdaten angezeigt werden
	Window* overlay_window = nullptr;
	if(ErrCheck(createWindow(ghInstance, 800, 800, 0, 0, 1, overlay_window, "waterrower", default_window_callback, window->handle), "Overlayfenster öffnen") != SUCCESS){
		resetAppFlag(APP_RUNNING);
		return GENERIC_ERROR;		//Fehler wird oben schon ausgegeben
	};
	SetWindowLong(overlay_window->handle, GWL_STYLE, WS_VISIBLE);
	SetWindowLong(overlay_window->handle, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(overlay_window->handle, RGB(0, 0, 0), 0, LWA_COLORKEY);
	SetWindowPos(overlay_window->handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	setWorkoutFlag(*workout, WORKOUT_OVERLAY);
	allocPageData(main_page, overlay_window, sizeof(overlay_window));

	strcpy((char*)sendBuffer, "RESET");
	sendPacket(hDevice, sendBuffer, sizeof("RESET")-1);

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), window->windowHeight/window->pixelSize-(int)(window->windowHeight/window->pixelSize*0.2)};
	ivec2 size = {(int)(window->windowHeight/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Startbildschirm";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;
	menu1->button_count = 1;

	WORD w = window->windowWidth/window->pixelSize;
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
		if(ErrCheck(initCommunication(hDevice, sendBuffer, receiveBuffer, 500)) != SUCCESS){
			resetAppFlag(APP_HAS_DEVICE);
			//TODO Fehler dem User melden
		}
		loadStartPage(nullptr);
	}
	return SUCCESS;
}
ErrCode switchToOpenDevicePage(Window* window){
	destroyPageNoFont(main_page);

	resetAppFlag(APP_HAS_DEVICE);
	closeDevice(hDevice);

	Image* backgroundImage = new Image;
	ErrCheck(loadImage("textures/basic_dark.tex", *backgroundImage), "Hintergrund image laden");
	main_page.images[0] = backgroundImage;
	main_page.imageInfo[0].pos = {0, 0};
	main_page.imageInfo[0].size = {window->windowWidth/window->pixelSize, window->windowHeight/window->pixelSize};
	main_page.image_count = 1;

	Menu* menu1 = new Menu;

	Image* buttonImage = new Image;
	ErrCheck(loadImage("textures/button.tex", *buttonImage), "button image laden");
	menu1->images[0] = buttonImage;
	menu1->image_count = 1;

	//TODO testet nur Port 1-10 für Geräte
	BYTE deviceNumbers[10];
	BYTE device_count = 0;
	for(WORD i=1; i <= 10; ++i){
		std::string port = "\\\\.\\COM" + std::to_string(i);
		if(openDevice(hDevice, port.c_str(), 19200) == SUCCESS){
			deviceNumbers[device_count++] = i;
			closeDevice(hDevice);
		}
	}

	ivec2 pos = {(int)(window->windowWidth/window->pixelSize*0.125), window->windowHeight/window->pixelSize-(int)(window->windowHeight/window->pixelSize*0.2)};
	ivec2 size = {(int)(window->windowHeight/window->pixelSize*0.1)*4, (int)(window->windowHeight/window->pixelSize*0.1)};
	menu1->buttons[0].pos = {pos.x, pos.y};
	menu1->buttons[0].size = {size.x, size.y};
	menu1->buttons[0].repos = {(int)(pos.x-size.x*0.05), (int)(pos.y-size.y*0.05)};
	menu1->buttons[0].resize = {(int)(size.x+size.x*0.1), (int)(size.y+size.y*0.1)};
	menu1->buttons[0].event = loadStartPage;
	menu1->buttons[0].text = "Startbildschirm";
	menu1->buttons[0].image = buttonImage;
	menu1->buttons[0].textsize = size.y/2;

	pos.y = (int)(window->windowHeight/window->pixelSize*0.125);
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
		pos.y += size.y+(int)(window->windowHeight/window->pixelSize*0.0125);
	}
	menu1->button_count = 1+device_count;

	WORD w = window->windowWidth/window->pixelSize;
	WORD label_text_size = w*0.0775;
	menu1->labels[0].pos = {20, 20};
	menu1->labels[0].text_size = label_text_size;
	menu1->label_count = 1;

	main_page.menus[0] = menu1;
	main_page.menu_count = 1;

	main_page.code = _default_page_function;

	return SUCCESS;
}
