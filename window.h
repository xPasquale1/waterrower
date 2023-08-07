#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fstream>

#include "util.h"

#define WINDOWSTATETYPE BYTE
enum WINDOWSTATE : WINDOWSTATETYPE{
	WINDOW_CLOSE = 1,
	WINDOW_RESIZE = 2
};
struct WindowInfo{
	WORD window_width = 800;
	WORD window_height = 800;
	WORD pixel_size = 1;
	WINDOWSTATETYPE state;			//Zustand des Fensters (können mehrere sein)
};
#define MAX_WINDOW_COUNT 10
struct Application{
	HWND windows[MAX_WINDOW_COUNT];		//Fenster handles der app
	WindowInfo info[MAX_WINDOW_COUNT];	//Fensterinfos
	uint* pixels[MAX_WINDOW_COUNT];		//Zugehörige Pixeldaten der Fenster
	WORD window_count = 0;				//Anzahl der Fenster
	BITMAPINFO bitmapInfo = {};			//Bitmapinfo, gleich für alle Fenster!
}; static Application app;

inline ErrCode getWindow(HWND window, WORD& index){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			index = i;
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

inline ErrCode setWindowState(HWND window, WINDOWSTATE state){
	ErrCode code; WORD idx;
	code = getWindow(window, idx);
	if(code) return code;
	app.info[idx].state |= state;
	return SUCCESS;
}

inline ErrCode resetWindowState(HWND window, WINDOWSTATE state){
	ErrCode code; WORD idx;
	code = getWindow(window, idx);
	if(code) return code;
	app.info[idx].state &= ~state;
	return SUCCESS;
}

//TODO Fehler melden
inline bool getWindowState(HWND window, WINDOWSTATE state){
	WORD idx;
	getWindow(window, idx);
	return (app.info[idx].state & state);
}

//TODO Fehler melden
//Gibt den nächsten Zustand des Fensters zurück, Anwendung z.B. while(getNextWindowState())...
inline WINDOWSTATE getNextWindowState(HWND window){
	WORD idx = 0;
	getWindow(window, idx);
	WINDOWSTATETYPE state = app.info[idx].state & -app.info[idx].state;
	app.info[idx].state &= ~state;
	return (WINDOWSTATE)state;
}

inline ErrCode resizeWindow(HWND window, WORD width, WORD height, WORD pixel_size){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			delete[] app.pixels[i];
			app.info[i].window_width = width;
			app.info[i].window_height = height;
			app.info[i].pixel_size = pixel_size;
			app.pixels[i] = new uint[width/pixel_size*height/pixel_size];
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

typedef LRESULT (*window_callback_function)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//Setzt bei Erfolg den Parameter window zu einem gültigen window handle, auf welches man das zugreifen kann
ErrCode openWindow(HINSTANCE hInstance, LONG window_width, LONG window_height, WORD pixel_size, HWND& window, const char* name = "Window", window_callback_function callback = default_window_callback, HWND parentWindow = NULL){
	//Erstelle Fenster Klasse
	if(app.window_count >= MAX_WINDOW_COUNT) return TOO_MANY_WINDOWS;
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Window-Class";
	window_class.lpfnWndProc = callback;

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
	app.windows[app.window_count] = CreateWindow(window_class.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 100, 50, w, h, parentWindow, NULL, hInstance, NULL);
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
				app.info[j] = app.info[j+1];
			}
			app.window_count--;
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:{
		ErrCheck(closeWindow(hwnd), "schließe Fenster");
		break;
	}
	case WM_SIZE:{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		ErrCheck(resizeWindow(hwnd, width, height, 2));
        break;
	}
	case WM_LBUTTONDOWN:{
		setButton(mouse, MOUSE_LMB);
		break;
	}
	case WM_LBUTTONUP:{
		resetButton(mouse, MOUSE_LMB);
		break;
	}
	case WM_RBUTTONDOWN:{
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

inline void getMessages()noexcept{
	MSG msg;
	for(WORD i=0; i < app.window_count; ++i){
		HWND window = app.windows[i];
		while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
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
inline constexpr BYTE A(uint color){return BYTE(color>>24);}
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

inline void drawRectangle(WORD idx, uint x, uint y, uint dx, uint dy, uint color)noexcept{
	uint buffer_width = app.info[idx].window_width/app.info[idx].pixel_size;
	uint* pixels = app.pixels[idx];
	for(uint i=y; i < y+dy; ++i){
		for(uint j=x; j < x+dx; ++j){
			pixels[i*buffer_width+j] = color;
		}
	}
}

inline ErrCode drawLine(HWND window, WORD start_x, WORD start_y, WORD end_x, WORD end_y, uint color)noexcept{
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint* pixels = app.pixels[i];
		    int dx = end_x-start_x;
		    int dy = end_y-start_y;
		    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
		    float xinc = dx/(float)steps;
		    float yinc = dy/(float)steps;
		    float x = start_x;
		    float y = start_y;
		    for(int i = 0; i <= steps; ++i){
		    	pixels[(int)y*buffer_width+(int)x] = color;
		        x += xinc;
		        y += yinc;
		    }
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

//idx ist der window index
inline void drawLine(WORD idx, WORD start_x, WORD start_y, WORD end_x, WORD end_y, uint color)noexcept{
	uint buffer_width = app.info[idx].window_width/app.info[idx].pixel_size;
	uint* pixels = app.pixels[idx];
	int dx = end_x-start_x;
	int dy = end_y-start_y;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	float xinc = dx/(float)steps;
	float yinc = dy/(float)steps;
	float x = start_x;
	float y = start_y;
	for(int i = 0; i <= steps; ++i){
		pixels[(int)y*buffer_width+(int)x] = color;
		x += xinc;
		y += yinc;
	}
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
	int pos = file.tellg();
	image.data = new(std::nothrow) uint[image.width*image.height];
	if(!image.data) return BAD_ALLOC;
	file.close();
	file.open(name, std::ios::in | std::ios::binary);
	if(!file.is_open()) return FILE_NOT_FOUND;
	file.seekg(pos);
	char val[4];
	file.read(&val[0], 1);	//Überspringe letztes Leerzeichen
	for(uint i=0; i < image.width*image.height; ++i){
		file.read(&val[0], 1);
		file.read(&val[1], 1);
		file.read(&val[2], 1);
		file.read(&val[3], 1);
		image.data[i] = RGBA(val[0], val[1], val[2], val[3]);
	}
	file.close();
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
					uint color = image.data[ry*image.width+rx];
					if(A(color) > 0) pixels[y*buffer_width+x] = color;
				}
			}
			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}

struct Font{
	Image image;
	ivec2 char_size;		//Größe eines Symbols im Image
	WORD font_size = 12;	//Größe der Symbols in Pixel
	BYTE char_sizes[96];	//Größe der Symbole in x-Richtung
};

ErrCode loadFont(const char* path, Font& font, ivec2 char_size){
	ErrCode code;
	font.char_size = char_size;
	if((code = ErrCheck(loadImage(path, font.image), "font image laden")) != SUCCESS){
		return code;
	}
	//Lese max x von jedem Zeichen
	font.char_sizes[0] = font.char_size.x/2;	//Leerzeichen
	for(uint i=1; i < 96; ++i){
		uint x_max = 0;
		for(uint y=(i/16)*char_size.y; y < (i/16)*char_size.y+char_size.y; ++y){
			for(uint x=(i%16)*char_size.x; x < (i%16)*char_size.x+char_size.x; ++x){
				if(A(font.image.data[y*font.image.width+x]) > 0 && x > x_max){
					x_max = x;
				}
			}
		}
		font.char_sizes[i] = x_max - (i%16)*char_size.x + 10;
	}
	return SUCCESS;
}

//Gibts zurück wie viele Pixel der Text unter der gegebenen Font benötigt
WORD getStringFontSize(Font& font, std::string& text){
	float div = (float)font.char_size.y/font.font_size;
	WORD offset = 0;
	for(size_t i=0; i < text.size(); ++i){
		BYTE idx = (text[i]-32);
		offset += font.char_sizes[idx]/div;
	}
	return offset;
}

//Gibt zurück wie breit das Symbol war das gezeichnet wurde
uint drawFontChar(HWND window, Font& font, char symbol, uint start_x, uint start_y){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint idx = (symbol-32);
			float div = (float)font.char_size.y/font.font_size;
			uint end_x = start_x+font.char_sizes[idx]/div;
			uint end_y = start_y+font.font_size;
			uint x_offset = (idx%16)*font.char_size.x;
			uint y_offset = (idx/16)*font.char_size.y;
			uint buffer_width = app.info[i].window_width/app.info[i].pixel_size;
			uint* pixels = app.pixels[i];
			for(uint y=start_y; y < end_y; ++y){
				float scaled_y = (float)(y-start_y)/(end_y-start_y);
				for(uint x=start_x; x < end_x; ++x){
					uint ry = scaled_y*font.char_size.y;
					uint rx = (float)(x-start_x)/(end_x-start_x)*(font.char_sizes[idx]-1);
					uint color = font.image.data[(ry+y_offset)*font.image.width+rx+x_offset];
					if(A(color) > 0) pixels[y*buffer_width+x] = color;
				}
			}
			return end_x-start_x;
		}
	}
	return 0;
}

//Wie oben nur ist idx der Fenster index
WORD drawFontChar(WORD idx, Font& font, char symbol, uint start_x, uint start_y){
	uint val = (symbol-32);
	float div = (float)font.char_size.y/font.font_size;
	uint end_x = start_x+font.char_sizes[val]/div;
	uint end_y = start_y+font.font_size;
	uint x_offset = (val%16)*font.char_size.x;
	uint y_offset = (val/16)*font.char_size.y;
	uint buffer_width = app.info[idx].window_width/app.info[idx].pixel_size;
	uint* pixels = app.pixels[idx];
	for(uint y=start_y; y < end_y; ++y){
		float scaled_y = (float)(y-start_y)/(end_y-start_y);
		for(uint x=start_x; x < end_x; ++x){
			uint ry = scaled_y*font.char_size.y;
			uint rx = (float)(x-start_x)/(end_x-start_x)*(font.char_sizes[val]-1);
			uint color = font.image.data[(ry+y_offset)*font.image.width+rx+x_offset];
			if(A(color) > 0) pixels[y*buffer_width+x] = color;
		}
	}
	return end_x-start_x;
}

//Zerstört eine im Heap allokierte Font und alle weiteren allokierten Elemente
void destroyFont(Font*& font){
	destroyImage(font->image);
	delete font;
	font = nullptr;
}

ErrCode _defaultEvent(void){return SUCCESS;}
enum BUTTONSTATE{
	BUTTON_VISIBLE=1, BUTTON_CAN_HOVER=2, BUTTON_HOVER=4, BUTTON_PRESSED=8, BUTTON_TEXT_CENTER
};
struct Button{
	ErrCode (*event)(void) = _defaultEvent;	//Funktionspointer zu einer Funktion die gecallt werden soll wenn der Button gedrückt wird
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
	WORD textsize = 16;
};

void destroyButton(Button& button){
	delete[] button.image;	//nullptr check sollte nicht nötig sein
}

inline constexpr bool checkButtonState(Button& button, BUTTONSTATE state){return (button.state&state);}
//TODO kann bestimmt besser geschrieben werden... und ErrCheck aufs Event sollte mit einem BUTTONSTATE entschieden werden
inline void buttonsClicked(Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
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

inline void drawButtons(HWND window, Font& font, Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
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
			uint offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			WORD str_size = getStringFontSize(font, b.text);
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(window, font, b.text[i], b.pos.x+offset+b.size.x/2-str_size/2, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}else{
			uint offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(window, font, b.text[i], b.pos.x+offset, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}
	}
}

inline void updateButtons(HWND window, Font& font, Button* buttons, WORD button_count){
	buttonsClicked(buttons, button_count);
	drawButtons(window, font, buttons, button_count);
}

//Speichert und zeigt x Buttons und strings an
struct Label{
	std::string text;
	ivec2 pos = {0, 0};
	uint textcolor = RGBA(180, 180, 180);
	WORD text_size = 2;
};

enum MENUSTATE{
	MENU_OPEN=1, MENU_OPEN_TOGGLE=2
};
#define MAX_BUTTONS 10
#define MAX_STRINGS 20
#define MAX_IMAGES 5
struct Menu{
	Image* images[MAX_IMAGES];
	WORD image_count = 0;
	Button buttons[MAX_BUTTONS];
	WORD button_count = 0;
	BYTE state = MENU_OPEN;	//Bits: offen, toggle bit für offen, Rest ungenutzt
	ivec2 pos = {};			//TODO Position in Bildschirmpixelkoordinaten
	Label labels[MAX_STRINGS];
	WORD label_count = 0;
};

void destroyMenu(Menu& menu){
	for(WORD i=0; i < menu.image_count; ++i){
		delete[] menu.images[i];	//nullptr check sollte nicht nötig sein
	}
}

inline constexpr bool checkMenuState(Menu& menu, MENUSTATE state){return (menu.state&state);}
inline void updateMenu(HWND window, Menu& menu, Font& font){
	if(checkMenuState(menu, MENU_OPEN)){
		updateButtons(window, font, menu.buttons, menu.button_count);
		for(WORD i=0; i < menu.label_count; ++i){
			Label& label = menu.labels[i];
			uint offset = 0;
			for(size_t j=0; j < label.text.size(); ++j){
				WORD tmp = font.font_size;
				font.font_size = label.text_size;
				offset += drawFontChar(window, font, label.text[j], label.pos.x+offset, label.pos.y);
				font.font_size = tmp;
			}
		}
	}
}
