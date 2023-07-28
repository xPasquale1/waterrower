#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fstream>

#include "fonts/5x5font.h"
#include "util.h"

enum WINDOWSTATE{
	WINDOW_SHOULD_CLOSE = 1
};
struct WindowInfo{
	WORD window_width = 800;
	WORD window_height = 800;
	WORD pixel_size = 1;
	BYTE state = 0;				//Bit 0: wurde geschlossen
};
#define MAX_WINDOW_COUNT 20
struct Application{
	HWND windows[MAX_WINDOW_COUNT];		//Fenster handles der app
	WindowInfo info[MAX_WINDOW_COUNT];	//Fensterinfos
	uint* pixels[MAX_WINDOW_COUNT];		//Zugehörige pixeldaten der Fenster
	WORD window_count = 0;				//Anzahl der Fenster
	BITMAPINFO bitmapInfo = {};			//TODO eigentlich ist das für jedes Fenster gleich, von daher braucht man kein array
}; static Application app;

inline ErrCode setWindowState(HWND window, WINDOWSTATE state){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			app.info[i].state |= state;
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}
inline ErrCode resetWindowState(HWND window, WINDOWSTATE state){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			app.info[i].state &= ~state;
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}
//TODO schlecht gelöst, sollte auch Fehlercodes zurückgeben
inline bool getWindowState(HWND window, WINDOWSTATE state){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			return (app.info[i].state & state);
		}
		return false;
	}
	return true;
}

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//Setzt bei Erfolg den Parameter window zu einem gültigen window handle, auf welches man das zugreifen kann
ErrCode openWindow(HINSTANCE hInstance, LONG window_width, LONG window_height, WORD pixel_size, HWND& window, const char* name = "Window"){
	//Erstelle Fenster Klasse
	if(app.window_count >= MAX_WINDOW_COUNT) return TOO_MANY_WINDOWS;
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Window-Class";
	window_class.lpfnWndProc = window_callback;

	//Registriere Fenster Klasse
	RegisterClass(&window_class);

	RECT rect;
    rect.top = 0;
    rect.bottom = window_height;
    rect.left = 0;
    rect.right = window_width;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	uint w = rect.right - rect.left;
	uint h = rect.bottom - rect.top;

	//Erstelle das Fenster	//TODO x und y offset angeben können
	app.windows[app.window_count] = CreateWindow(window_class.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, w, h, NULL, NULL, hInstance, NULL);
	window = app.windows[app.window_count];

	//Bitmap-Info	//TODO muss eigentlich nicht immer neu beschrieben werden, da es ja nur eins gibt...
	app.bitmapInfo.bmiHeader.biSize = sizeof(app.bitmapInfo.bmiHeader);
	app.bitmapInfo.bmiHeader.biPlanes = 1;
	app.bitmapInfo.bmiHeader.biBitCount = 32;
	app.bitmapInfo.bmiHeader.biCompression = BI_RGB;

	WORD buffer_width = window_width/pixel_size;
	WORD buffer_height = window_height/pixel_size;
	app.info[app.window_count].pixel_size = pixel_size;
	app.pixels[app.window_count] = new uint[buffer_width*buffer_height];
	app.window_count++;
	return SUCCESS;
}

ErrCode closeWindow(HWND window){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			DestroyWindow(app.windows[i]);
			delete[] app.pixels[i];
			for(WORD j=i; j < app.window_count-1; ++j){
				app.windows[j] = app.windows[j+1];
				app.pixels[j] = app.pixels[j+1];
				app.info[j] = app.info[j+1];	//TODO hier wird einiges kopiert aber das passiert ja eh nicht oft...
			}
			app.window_count--;
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

inline void getMessages()noexcept{
	MSG msg;
	for(WORD i=0; i < app.window_count; ++i){
		if(getWindowState(app.windows[i], WINDOW_SHOULD_CLOSE)){
			closeWindow(app.windows[i]);
			break;
		}
		while(PeekMessage(&msg, app.windows[i], 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

inline ErrCode clearWindow(HWND window)noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint buffer_height = app.info[i].window_height/app.info[i].pixel_size;
			uint* pixels = app.pixels[i];
			for(uint y=0; y < buffer_height; ++y){
				for(uint x=0; x < buffer_width; ++x){
					pixels[y*buffer_width+x] = 0;
				}
			}
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

inline void clearWindows()noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
		uint buffer_height = app.info[i].window_height/app.info[i].pixel_size;
		uint* pixels = app.pixels[i];
		for(uint y=0; y < buffer_height; ++y){
			for(uint x=0; x < buffer_width; ++x){
				pixels[y*buffer_width+x] = 0;
			}
		}
	}
}

inline constexpr uint RGBA(BYTE r, BYTE g, BYTE b, BYTE a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr BYTE R(uint color){return BYTE(color>>16);}
inline constexpr BYTE G(uint color){return BYTE(color>>8);}
inline constexpr BYTE B(uint color){return BYTE(color);}

inline ErrCode drawWindow(HWND window)noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint buffer_height = app.info[i].window_height/app.info[i].pixel_size;
			HDC hdc = GetDC(app.windows[i]);
			app.bitmapInfo.bmiHeader.biWidth = buffer_width;
			app.bitmapInfo.bmiHeader.biHeight = -buffer_height;
			StretchDIBits(hdc, 0, 0, app.info[i].window_width, app.info[i].window_height, 0, 0, buffer_width, buffer_height, app.pixels[i], &app.bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
			ReleaseDC(app.windows[i], hdc);
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

inline void drawWindows()noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
		uint buffer_height = app.info[i].window_height/app.info[i].pixel_size;
		HDC hdc = GetDC(app.windows[i]);
		app.bitmapInfo.bmiHeader.biWidth = buffer_width;
		app.bitmapInfo.bmiHeader.biHeight = -buffer_height;
		StretchDIBits(hdc, 0, 0, app.info[i].window_width, app.info[i].window_height, 0, 0, buffer_width, buffer_height, app.pixels[i], &app.bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(app.windows[i], hdc);
	}
}

inline ErrCode drawRectangle(HWND window, uint x, uint y, uint dx, uint dy, uint color)noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint* pixels = app.pixels[i];
			for(uint i=y; i < y+dy; ++i){
				for(uint j=x; j < x+dx; ++j){
					pixels[i*buffer_width+j] = color;
				}
			}
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

//Geht nur von 0-9!!!
//TODO alle Funktionen hier sind nur auf die 5x5 font abgestimmt, sollte man verallgemeinern
inline void draw_number(HWND window, uint x, uint y, uint size, BYTE number, uint color){
	short idx = (number+48)*5;
	for(uint i=0; i < 5; ++i){
		for(uint j=0; j < 5; ++j){
			if((font5x5[idx]<<j)&0b10000) drawRectangle(window, x+j*size, y+i*size, size, size, color);
		}
		++idx;
	}
}

static int _rec_offset = 0;
int _number_recurse(HWND window, uint x, uint y, uint size, int num, uint color, int iter){
	if(num > 0){
		++_rec_offset;
		_number_recurse(window, x, y, size, num/10, color, iter+1);
		draw_number(window, x+(_rec_offset-iter-1)*5*size, y, size, num%10, color);
	}
	return _rec_offset;
}

void drawCharacter(HWND window, uint x, uint y, uint size, BYTE character, uint color){
	short idx = character*5;
	for(uint i=0; i < 5; ++i){
		for(uint j=0; j < 5; ++j){
			if((font5x5[idx]<<j)&0b10000) drawRectangle(window, x+j*size, y+i*size, size, size, color);
		}
		++idx;
	}
}

//Gibt die Anzahl der gezeichneten Zeichen zurück
int drawInt(HWND window, uint x, uint y, uint size, int num, uint color){
	if(num == 0){
		draw_number(window, x, y, size, 0, color);
		return 1;
	}
	_rec_offset = 0;
	if(num < 0){
		_rec_offset = 1;
		num *= -1;
		drawCharacter(window, x, y, size, '-', color);
	}
	return _number_recurse(window, x, y, size, num, color, 0);
}

struct Image{
	uint* data = nullptr;
	WORD width = 0;		//x-Dimension
	WORD height = 0;	//y-Dimension
};

ErrCode loadImage(const char* name, Image& image){
	std::fstream file; file.open(name, std::ios::in);
	if(!file.is_open()) return FILE_NOT_FOUND;
	//Lese Breite und Höhe
	std::string word;
	file >> word;
	image.width = std::atoi(word.c_str());
	file >> word;
	image.height = std::atoi(word.c_str());
	image.data = new(std::nothrow) uint[image.width*image.height];
	if(!image.data) return BAD_ALLOC;
	for(uint i=0; i < image.width*image.height; ++i){
		file >> word;
		BYTE r = std::atoi(word.c_str());
		file >> word;
		BYTE g = std::atoi(word.c_str());
		file >> word;
		BYTE b = std::atoi(word.c_str());
		file >> word;
		BYTE a = std::atoi(word.c_str());
		image.data[i] = RGBA(r, g, b, a);
	}
	return SUCCESS;
}

void destroyImage(Image& image){
	delete[] image.data;
}

//x und y von 0 - 1
inline uint getImage(Image& image, float x, float y){
	uint ry = y*image.height;
	uint rx = x*(image.width-1);
	return image.data[ry*image.width+rx];
}

//Kopiert das gesamte Image in den angegebenen Bereich von start_x bis end_x und start_y bis end_y
//TODO Kopiere nicht das gesamte Image, sondern auch das sollte man angeben können
ErrCode copyImageToWindow(HWND window, Image& image, int start_x, int start_y, int end_x, int end_y){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint* pixels = app.pixels[i];
			for(int y=start_y; y < end_y; ++y){
				float scaled_y = (float)(y-start_y)/(end_y-start_y);
				for(int x=start_x; x < end_x; ++x){
					uint ry = scaled_y*image.height;
					uint rx = (float)(x-start_x)/(end_x-start_x)*(image.width-1);
					pixels[y*buffer_width+x] = image.data[ry*image.width+rx];
				}
			}
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

ErrCode _defaultEvent(void){return SUCCESS;}
enum BUTTONSTATE{
	BUTTON_VISIBLE=1, BUTTON_CAN_HOVER=2, BUTTON_HOVER=4, BUTTON_PRESSED=8, BUTTON_TEXT_CENTER
};
struct Button{
	~Button(){}
	ErrCode (*event)(void) = &_defaultEvent;	//Funktionspointer zu einer Funktion die gecallt werden soll wenn der Button gedrückt wird
	std::string text;
	Image* image = nullptr;
	ivec2 pos = {0, 0};
	ivec2 repos = {0, 0};
	ivec2 size = {50, 10};
	ivec2 resize = {55, 11};
	BYTE state = BUTTON_VISIBLE | BUTTON_CAN_HOVER | BUTTON_TEXT_CENTER;
	uint color = RGBA(120, 120, 120);
	uint hover_color = RGBA(120, 120, 255);
	uint textcolor = RGBA(180, 180, 180);
	uint textsize=2;
};
inline constexpr bool checkButtonState(Button& button, BUTTONSTATE state){return (button.state&state);}
//TODO kann bestimmt besser geschrieben werden...
inline void buttonsClicked(Button* buttons, uint button_count, Mouse& mouse){
	for(uint i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!checkButtonState(b, BUTTON_VISIBLE)) continue;
		ivec2 delta = {mouse.pos.x - b.pos.x, mouse.pos.y - b.pos.y};
		if(delta.x >= 0 && delta.x <= b.size.x && delta.y >= 0 && delta.y <= b.size.y){
			if(checkButtonState(b, BUTTON_CAN_HOVER)) b.state |= BUTTON_HOVER;
			if(getButton(mouse, MOUSE_LMB) && !checkButtonState(b, BUTTON_PRESSED)){
				ErrCheck(b.event());
				b.state |= BUTTON_PRESSED;
			}
			else if(!getButton(mouse, MOUSE_LMB)) b.state &= ~BUTTON_PRESSED;
		}else if(checkButtonState(b, BUTTON_CAN_HOVER)){
			b.state &= ~BUTTON_HOVER;
		}
	}
}

inline void drawButtons(HWND window, Button* buttons, uint button_count){
	for(uint i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!checkButtonState(b, BUTTON_VISIBLE)) continue;
		if(b.image == nullptr){
			if(checkButtonState(b, BUTTON_CAN_HOVER) && checkButtonState(b, BUTTON_HOVER))
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.hover_color);
			else
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.color);
		}else{
			if(checkButtonState(b, BUTTON_CAN_HOVER) && checkButtonState(b, BUTTON_HOVER)){
				copyImageToWindow(window, *b.image, b.repos.x, b.repos.y, b.repos.x+b.resize.x, b.repos.y+b.resize.y);
			}else
				copyImageToWindow(window, *b.image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}
		if(checkButtonState(b, BUTTON_TEXT_CENTER)){
			int x_offset = b.size.x/2-((b.text.size())*10+b.text.size()-1+1)/2;
			for(size_t i=0; i < b.text.size(); ++i){
				drawCharacter(window, x_offset+b.pos.x+i*10+i+1, b.pos.y+b.size.y/2-b.textsize*5/2, 2, b.text[i], b.textcolor);
			}
		}else{
			for(size_t i=0; i < b.text.size(); ++i){
				drawCharacter(window, b.pos.x+i*10+i+1, b.pos.y+b.size.y/2-b.textsize*5/2, 2, b.text[i], b.textcolor);
			}
		}
	}
}

inline void updateButtons(HWND window, Button* buttons, uint button_count, Mouse& mouse){
	buttonsClicked(buttons, button_count, mouse);
	drawButtons(window, buttons, button_count);
}

enum MENUSTATE{
	MENU_OPEN=1, MENU_OPEN_TOGGLE=2
};
#define MAX_BUTTONS 10
#define MAX_STRINGS 30
//Speichert und zeigt x Buttons und strings an
struct Label{
	std::string text;
	ivec2 pos = {0, 0};
	uint textcolor = RGBA(180, 180, 180);
	WORD text_size = 2;
};
struct Menu{
	Button buttons[MAX_BUTTONS];
	WORD button_count=0;
	BYTE state = MENU_OPEN;	//Bits: offen, toggle bit für offen, Rest ungenutzt
	ivec2 pos = {};			//TODO Position in Bildschirmpixelkoordinaten, buttons bekommen zusätzlich diesen offset
	Label labels[MAX_STRINGS];
	WORD label_count = 0;
};
inline constexpr bool checkMenuState(Menu& menu, MENUSTATE state){return (menu.state&state);}
inline void updateMenu(HWND window, Menu& menu, Mouse& mouse){
	if(checkMenuState(menu, MENU_OPEN)){
		updateButtons(window, menu.buttons, menu.button_count, mouse);
		for(WORD i=0; i < menu.label_count; ++i){
			Label& label = menu.labels[i];
			for(size_t j=0; j < label.text.size(); ++j){
				drawCharacter(window, label.pos.x+label.text_size*5*j, label.pos.y, label.text_size, label.text[j], label.textcolor);
			}
		}
	}
}
