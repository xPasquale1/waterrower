#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <d2d1.h>

#include "util.h"

//TODO namespace?

#define WINDOWFLAGSTYPE BYTE
enum WINDOWFLAG : WINDOWFLAGSTYPE{
	WINDOW_NONE = 0,
	WINDOW_CLOSE = 1,
	WINDOW_RESIZE = 2
};
//Hat viele Attribute die man auch über die win api abrufen könnte, aber diese extra zu speichern macht alles übersichtlicher
struct Window{
	HWND handle;									//Fensterhandle
	WORD windowWidth = 800;							//Fensterbreite
	WORD windowHeight = 800;						//Fensterhöhe
	DWORD* pixels;									//Pixelarray
	WORD pixelSize = 1;								//Größe der Pixel in Bildschirmpixeln
	WINDOWFLAG flags = WINDOW_NONE;					//Fensterflags
	ID2D1HwndRenderTarget* renderTarget = nullptr;	//Direct 2D Rendertarget
	std::string windowClassName;					//Ja, jedes Fenster hat seine eigene Klasse... TODO
};

#define APPLICATIONFLAGSTYPE BYTE
enum APPLICATIONFLAG : APPLICATIONFLAGSTYPE{
	APP_RUNNING=1,
	APP_HAS_DEVICE=2
};
#define MAX_WINDOW_COUNT 10
struct Application{
	APPLICATIONFLAG flags = APP_RUNNING;		//Applikationsflags
	ID2D1Factory* factory = nullptr;			//Direct2D Factory
}; static Application app;

inline bool getAppFlag(APPLICATIONFLAG flag){return(app.flags & flag);}
inline void setAppFlag(APPLICATIONFLAG flag){app.flags = (APPLICATIONFLAG)(app.flags | flag);}
inline void resetAppFlag(APPLICATIONFLAG flag){app.flags = (APPLICATIONFLAG)(app.flags & ~flag);}

inline ErrCode initApp(){
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &app.factory);
	if(hr){
		std::cerr << hr << std::endl;
		return APP_INIT;
	}
	return SUCCESS;
}

typedef LRESULT (*window_callback_function)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//Setzt bei Erfolg den Parameter window zu einem gültigen window, auf welches man dann zugreifen kann
//window muss ein nullptr sein/kein vorhandenes window!
ErrCode createWindow(HINSTANCE hInstance, LONG windowWidth, LONG windowHeight, LONG x, LONG y, WORD pixelSize, Window*& window, const char* name = "Window", window_callback_function callback = default_window_callback, HWND parentWindow = NULL){
	//Erstelle Fenster Klasse
	if(window != nullptr) return CREATE_WINDOW;

	window = new Window;

	WNDCLASS window_class = {};
	window_class.hInstance = hInstance;
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	std::string className = "Window-Class-" + std::rand();	//TODO meh...
	window_class.lpszClassName = className.c_str();
	window_class.lpfnWndProc = callback;

	window->windowClassName = className;
	//Registriere Fenster Klasse
	if(!RegisterClass(&window_class)){
		std::cerr << GetLastError() << std::endl;
		return CREATE_WINDOW;
	}

	RECT rect;
    rect.top = 0;
    rect.bottom = windowHeight;
    rect.left = 0;
    rect.right = windowWidth;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	uint w = rect.right - rect.left;
	uint h = rect.bottom - rect.top;

	//Erstelle das Fenster
	window->handle = CreateWindow(window_class.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, x, y, w, h, parentWindow, NULL, hInstance, NULL);
	if(window->handle == NULL){
		std::cerr << GetLastError() << std::endl;
		return CREATE_WINDOW;
	}

	SetWindowLongPtr(window->handle, GWLP_USERDATA, (LONG_PTR)window);	//TODO idk ob das so ok ist, win32 doku sagt nicht viel darüber aus...

	D2D1_RENDER_TARGET_PROPERTIES properties = {};
	properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.dpiX = 96;
	properties.dpiY = 96;
	properties.usage = D2D1_RENDER_TARGET_USAGE_NONE;
	properties.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties = {};
	hwndProperties.hwnd = window->handle;
	hwndProperties.pixelSize = D2D1::SizeU(windowWidth, windowHeight);
	hwndProperties.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
    HRESULT hr = app.factory->CreateHwndRenderTarget(properties, hwndProperties, &window->renderTarget);
    if(hr){
		std::cerr << hr << std::endl;
		return INIT_RENDER_TARGET;
    }

	WORD buffer_width = windowWidth/pixelSize;
	WORD buffer_height = windowHeight/pixelSize;
	window->pixelSize = pixelSize;
	window->pixels = new DWORD[buffer_width*buffer_height];
	window->windowWidth = windowWidth;
	window->windowHeight = windowHeight;
	return SUCCESS;
}

ErrCode destroyWindow(Window*& window){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	if(!UnregisterClassA(window->windowClassName.c_str(), NULL)){
		std::cerr << GetLastError() << std::endl;
		return GENERIC_ERROR;
	}
	DestroyWindow(window->handle);
	delete[] window->pixels;
	delete window;
	window = nullptr;
	return SUCCESS;
}

inline ErrCode destroyApp(){
	app.factory->Release();
	return SUCCESS;
}

inline ErrCode setWindowFlag(Window* window, WINDOWFLAG state){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	window->flags = (WINDOWFLAG)(window->flags | state);
	return SUCCESS;
}
inline ErrCode resetWindowFlag(Window* window, WINDOWFLAG state){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	window->flags = (WINDOWFLAG)(window->flags & ~state);
	return SUCCESS;
}
inline bool getWindowFlag(Window* window, WINDOWFLAG state){
	if(window == nullptr) return false;
	return (window->flags & state);
}

//TODO Sollte ERRCODE zurückgeben und WINDOWFLAG als Referenzparameter übergeben bekommen
//Gibt den nächsten Zustand des Fensters zurück und löscht diesen anschließend, Anwendung z.B. while(state = getNextWindowState() != WINDOW_NONE)...
inline WINDOWFLAG getNextWindowState(Window* window){
	if(window == nullptr) return WINDOW_NONE;
	WINDOWFLAG flag = (WINDOWFLAG)(window->flags & -window->flags);
	window->flags = (WINDOWFLAG)(window->flags & ~flag);
	return flag;
}

inline ErrCode resizeWindow(Window* window, WORD width, WORD height, WORD pixel_size){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	window->windowWidth = width;
	window->windowHeight = height;
	window->pixelSize = pixel_size;
	delete[] window->pixels;
	window->pixels = new DWORD[width/pixel_size*height/pixel_size];
	window->renderTarget->Resize({width, height});
	return SUCCESS;
}

//TODO anstatt solch eine komplexe Funktion in createWindow rein zu geben, könnte man seine eigene schreiben mit Window* und uMsg,... als Parameter
//und diese default funktion ruft diese dann optional nur auf
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);	//TODO das ist ein Fehler, wie melden aber?
	switch(uMsg){
		case WM_DESTROY:{
			ErrCheck(setWindowFlag(window, WINDOW_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			ErrCheck(setWindowFlag(window, WINDOW_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(window, width, height, 1), "Fenster skalieren");
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
			mouse.pos.x = GET_X_LPARAM(lParam)/window->pixelSize;
			mouse.pos.y = GET_Y_LPARAM(lParam)/window->pixelSize;
			break;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

inline void getMessages(Window* window)noexcept{
	MSG msg;
	while(PeekMessage(&msg, window->handle, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline constexpr uint RGBA(BYTE r, BYTE g, BYTE b, BYTE a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr BYTE A(uint color){return BYTE(color>>24);}
inline constexpr BYTE R(uint color){return BYTE(color>>16);}
inline constexpr BYTE G(uint color){return BYTE(color>>8);}
inline constexpr BYTE B(uint color){return BYTE(color);}

inline ErrCode clearWindow(Window* window)noexcept{
	if(window == nullptr) return WINDOW_NOT_FOUND;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	for(WORD y=0; y < buffer_height; ++y){
		for(WORD x=0; x < buffer_width; ++x){
			window->pixels[y*buffer_width+x] = RGBA(0, 0, 0);
		}
	}
	return SUCCESS;
}

//TODO bitmap kann man bestimmt auch im Window speichern und auf dieser dann rummalen
inline ErrCode drawWindow(Window* window)noexcept{
	if(window == nullptr) return WINDOW_NOT_FOUND;
	if(window->windowWidth == 0 || window->windowHeight == 0) return GENERIC_ERROR;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	ID2D1Bitmap* bitmap;
	D2D1_BITMAP_PROPERTIES properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.dpiX = 96;
	properties.dpiY = 96;
	window->renderTarget->BeginDraw();
	HRESULT hr = window->renderTarget->CreateBitmap({buffer_width, buffer_height}, window->pixels, buffer_width*4, properties, &bitmap);
	if(hr){
		std::cerr << hr << std::endl;
		exit(-2);
	}
	window->renderTarget->DrawBitmap(bitmap, D2D1::RectF(0, 0, window->windowWidth, window->windowHeight), 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, buffer_width, buffer_height));
	bitmap->Release();
	window->renderTarget->EndDraw();
	return SUCCESS;
}

inline ErrCode drawRectangle(Window* window, uint x, uint y, uint dx, uint dy, uint color)noexcept{
	if(window == nullptr) return WINDOW_NOT_FOUND;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	for(WORD i=y; i < y+dy; ++i){
		for(WORD j=x; j < x+dx; ++j){
			window->pixels[i*buffer_width+j] = color;
		}
	}
	return SUCCESS;
}

inline ErrCode drawLine(Window* window, WORD start_x, WORD start_y, WORD end_x, WORD end_y, uint color)noexcept{
	if(window == nullptr) return WINDOW_NOT_FOUND;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	int dx = end_x-start_x;
	int dy = end_y-start_y;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	float xinc = dx/(float)steps;
	float yinc = dy/(float)steps;
	float x = start_x;
	float y = start_y;
	for(int i = 0; i <= steps; ++i){
		window->pixels[(int)y*buffer_width+(int)x] = color;
		x += xinc;
		y += yinc;
	 }
	return SUCCESS;
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
	file.read(&val[0], 1);	//überspringe letztes Leerzeichen
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
	image.data = nullptr;
}

//x und y von 0 - 1
inline uint getImage(Image& image, float x, float y){
	uint ry = y*image.height;
	uint rx = x*(image.width-1);
	return image.data[ry*image.width+rx];
}

//Kopiert das gesamte Image in den angegebenen Bereich von start_x bis end_x und start_y bis end_y
//TODO Kopiere nicht das gesamte Image, sondern auch das sollte man angeben können
ErrCode copyImageToWindow(Window* window, Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	for(int y=start_y; y < end_y; ++y){
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			WORD rx = (float)(x-start_x)/(end_x-start_x)*(image.width-1);
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
		}
	}
	return SUCCESS;
}

//Funktion testet ob jeder pixel im gültigen Fensterbereich liegt! idx ist der window index
ErrCode copyImageToWindowSave(Window* window, Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y){
	if(window == nullptr) return WINDOW_NOT_FOUND;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	for(int y=start_y; y < end_y; ++y){
		if(y < 0 || y >= (int)buffer_height) continue;
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			if(x < 0 || x >= (int)buffer_width) continue;
			WORD rx = (float)(x-start_x)/(end_x-start_x)*(image.width-1);
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
		}
	}
	return SUCCESS;
}

struct Font{
	Image image;
	ivec2 char_size;		//Größe eines Symbols im Image
	WORD font_size = 12;	//Größe der Symbole in Pixel
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
//TODO Errors? übergebe symbol größe als Referenz Parameter
uint drawFontChar(Window* window, Font& font, char symbol, uint start_x, uint start_y){
	if(window == nullptr) return 0;
	uint idx = (symbol-32);
	float div = (float)font.char_size.y/font.font_size;
	uint end_x = start_x+font.char_sizes[idx]/div;
	uint end_y = start_y+font.font_size;
	uint x_offset = (idx%16)*font.char_size.x;
	uint y_offset = (idx/16)*font.char_size.y;
	uint buffer_width = window->windowWidth/window->pixelSize;
	for(uint y=start_y; y < end_y; ++y){
		float scaled_y = (float)(y-start_y)/(end_y-start_y);
		for(uint x=start_x; x < end_x; ++x){
			uint ry = scaled_y*font.char_size.y;
			uint rx = (float)(x-start_x)/(end_x-start_x)*(font.char_sizes[idx]-1);
			uint color = font.image.data[(ry+y_offset)*font.image.width+rx+x_offset];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
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

ErrCode _defaultEvent(BYTE*){return SUCCESS;}
enum BUTTONFLAGS{
	BUTTON_VISIBLE=1,
	BUTTON_CAN_HOVER=2,
	BUTTON_HOVER=4,
	BUTTON_PRESSED=8,
	BUTTON_TEXT_CENTER=16,
	BUTTON_DISABLED=32
};
struct Button{
	ErrCode (*event)(BYTE*) = _defaultEvent;	//Funktionspointer zu einer Funktion die gecallt werden soll wenn der Button gedrückt wird
	std::string text;
	Image* image = nullptr;
	Image* disabled_image = nullptr;
	ivec2 pos = {0, 0};
	ivec2 repos = {0, 0};
	ivec2 size = {50, 10};
	ivec2 resize = {55, 11};
	BYTE flags = BUTTON_VISIBLE | BUTTON_CAN_HOVER | BUTTON_TEXT_CENTER;
	uint color = RGBA(120, 120, 120);
	uint hover_color = RGBA(120, 120, 255);
	uint textcolor = RGBA(180, 180, 180);
	uint disabled_color = RGBA(90, 90, 90);
	WORD textsize = 16;
	BYTE* data = nullptr;
};

void destroyButton(Button& button){
	destroyImage(*button.image);
	delete[] button.data;
}

inline constexpr void setButtonFlag(Button& button, BUTTONFLAGS flag){button.flags |= flag;}
inline constexpr void resetButtonFlag(Button& button, BUTTONFLAGS flag){button.flags &= ~flag;}
inline constexpr bool getButtonFlag(Button& button, BUTTONFLAGS flag){return (button.flags & flag);}
//TODO kann bestimmt besser geschrieben werden... und ErrCheck aufs Event sollte mit einem BUTTONSTATE entschieden werden
inline void buttonsClicked(Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTON_VISIBLE) || getButtonFlag(b, BUTTON_DISABLED)) continue;
		ivec2 delta = {mouse.pos.x - b.pos.x, mouse.pos.y - b.pos.y};
		if(delta.x >= 0 && delta.x <= b.size.x && delta.y >= 0 && delta.y <= b.size.y){
			if(getButtonFlag(b, BUTTON_CAN_HOVER)) b.flags |= BUTTON_HOVER;
			if(getButton(mouse, MOUSE_LMB) && !getButtonFlag(b, BUTTON_PRESSED)){
				ErrCheck(b.event(b.data));
				b.flags |= BUTTON_PRESSED;
			}
			else if(!getButton(mouse, MOUSE_LMB)) b.flags &= ~BUTTON_PRESSED;
		}else if(getButtonFlag(b, BUTTON_CAN_HOVER)){
			b.flags &= ~BUTTON_HOVER;
		}
	}
}

inline void drawButtons(Window* window, Font& font, Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTON_VISIBLE)) continue;
		if(getButtonFlag(b, BUTTON_DISABLED)){
			if(b.disabled_image == nullptr)
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.disabled_color);
			else
				copyImageToWindow(window, *b.disabled_image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}else if(b.image == nullptr){
			if(getButtonFlag(b, BUTTON_CAN_HOVER) && getButtonFlag(b, BUTTON_HOVER))
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.hover_color);
			else
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.color);
		}else{
			if(getButtonFlag(b, BUTTON_CAN_HOVER) && getButtonFlag(b, BUTTON_HOVER))
				copyImageToWindow(window, *b.image, b.repos.x, b.repos.y, b.repos.x+b.resize.x, b.repos.y+b.resize.y);
			else
				copyImageToWindow(window, *b.image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}
		if(getButtonFlag(b, BUTTON_TEXT_CENTER)){
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

inline void updateButtons(Window* window, Font& font, Button* buttons, WORD button_count){
	buttonsClicked(buttons, button_count);
	drawButtons(window, font, buttons, button_count);
}

struct Label{
	std::string text;
	ivec2 pos = {0, 0};
	uint textcolor = RGBA(180, 180, 180);
	WORD text_size = 2;
};

enum MENUFLAGS{
	MENU_OPEN=1,
	MENU_OPEN_TOGGLE=2
};
#define MAX_BUTTONS 10
#define MAX_STRINGS 20
#define MAX_IMAGES 5
struct Menu{
	Image* images[MAX_IMAGES];	//Sind für die Buttons
	BYTE image_count = 0;
	Button buttons[MAX_BUTTONS];
	BYTE button_count = 0;
	BYTE flags = MENU_OPEN;		//Bits: offen, toggle bit für offen, Rest ungenutzt
	ivec2 pos = {};				//TODO Position in Bildschirmpixelkoordinaten
	Label labels[MAX_STRINGS];
	BYTE label_count = 0;
};

void destroyMenu(Menu& menu){
	for(WORD i=0; i < menu.image_count; ++i){
		destroyImage(*menu.images[i]);
	}
}

inline constexpr bool getMenuFlag(Menu& menu, MENUFLAGS state){return (menu.flags&state);}

inline void updateMenu(Window* window, Menu& menu, Font& font){
	if(getMenuFlag(menu, MENU_OPEN)){
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
