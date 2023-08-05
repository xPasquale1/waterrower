#pragma once

#include <iostream>
#include <math.h>

typedef unsigned char BYTE;
typedef unsigned long uint;

struct ivec2{
	int x;
	int y;
};

//Error-Codes
enum ErrCode{
	SUCCESS = 0,
	BAD_ALLOC,
	TEXTURE_NOT_FOUND,
	MODEL_NOT_FOUND,
	FILE_NOT_FOUND,
	QUEUE_FULL,
	WINDOW_NOT_FOUND,
	TOO_MANY_WINDOWS,
	REQUEST_NOT_FOUND,
	INVALID_WORKOUT,
	INVALID_USB_HANDLE,
	COMMSTATE_ERROR,
	TIMEOUT_SET_ERROR
};
enum ErrCodeFlags{
	ERR_NO_FLAG = 0,
	ERR_NO_OUTPUT,
	ERR_ON_FATAL
};
//TODO ERR_ON_FATAL ausgeben können wenn der nutzer es so möchte
inline ErrCode ErrCheck(ErrCode code, const char* msg="\0", ErrCodeFlags flags=ERR_NO_FLAG){
	switch(code){
	case BAD_ALLOC:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[BAD_ALLOC ERROR] " << msg << std::endl;
		return BAD_ALLOC;
	case TEXTURE_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[TEXTURE_NOT_FOUND ERROR] " << msg << std::endl;
		return TEXTURE_NOT_FOUND;
	case MODEL_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[MODEL_NOT_FOUND ERROR] " << msg << std::endl;
		return MODEL_NOT_FOUND;
	case FILE_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[FILE_NOT_FOUND ERROR] " << msg << std::endl;
		return FILE_NOT_FOUND;
	case QUEUE_FULL:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[QUEUE_FULL ERROR] " << msg << std::endl;
		return QUEUE_FULL;
	case WINDOW_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[WINDOW_NOT_FOUND ERROR] " << msg << std::endl;
		return WINDOW_NOT_FOUND;
	case TOO_MANY_WINDOWS:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[TOO_MANY_WINDOWS ERROR] " << msg << std::endl;
		return TOO_MANY_WINDOWS;
	case REQUEST_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[REQUEST_NOT_FOUND ERROR] " << msg << std::endl;
		return REQUEST_NOT_FOUND;
	case INVALID_WORKOUT:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[INVALID_WORKOUT ERROR] " << msg << std::endl;
		return INVALID_WORKOUT;
	case INVALID_USB_HANDLE:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[INVALID_USB_HANDLE ERROR] " << msg << std::endl;
		return INVALID_USB_HANDLE;
	case COMMSTATE_ERROR:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[COMMSTATE_ERROR ERROR] " << msg << std::endl;
		return COMMSTATE_ERROR;
	case TIMEOUT_SET_ERROR:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[TIMEOUT_SET_ERROR ERROR] " << msg << std::endl;
		return TIMEOUT_SET_ERROR;
	default: return SUCCESS;
	}
	return SUCCESS;
}

enum MOUSEBUTTON{
	MOUSE_LMB = 1,
	MOUSE_RMB = 2
};
struct Mouse{
	ivec2 pos = {};
	char button = 0;	//Bits: LMB, RMB, Rest ungenutzt
}; static Mouse mouse;

inline constexpr bool getButton(Mouse& mouse, MOUSEBUTTON button){return (mouse.button & button);}
inline constexpr void setButton(Mouse& mouse, MOUSEBUTTON button){mouse.button |= button;}
inline constexpr void resetButton(Mouse& mouse, MOUSEBUTTON button){mouse.button &= ~button;}

inline std::string float_to_string(float value, BYTE decimals=2){
	WORD precision = pow(10, decimals);	//TODO könnte constexpr sein...
	DWORD val = value * precision;
	return std::to_string(val/precision) + '.' + std::to_string(val%precision);
}

enum KEYBOARDBUTTON : unsigned long long{
	KEY_0 = 0ULL,
	KEY_1 = 1ULL << 0,
	KEY_3 = 1ULL << 1,
	KEY_4 = 1ULL << 2,
	KEY_5 = 1ULL << 3,
	KEY_6 = 1ULL << 4,
	KEY_7 = 1ULL << 5,
	KEY_8 = 1ULL << 6,
	KEY_9 = 1ULL << 7,
	KEY_A = 1ULL << 8,
	KEY_B = 1ULL << 9,
	KEY_C = 1ULL << 10,
	KEY_D = 1ULL << 11,
	KEY_E = 1ULL << 12,
	KEY_F = 1ULL << 13,
	KEY_G = 1ULL << 14,
	KEY_H = 1ULL << 15,
	KEY_I = 1ULL << 16,
	KEY_J = 1ULL << 17,
	KEY_K = 1ULL << 18,
	KEY_L = 1ULL << 19,
	KEY_M = 1ULL << 20,
	KEY_N = 1ULL << 21,
	KEY_O = 1ULL << 22,
	KEY_P = 1ULL << 23,
	KEY_Q = 1ULL << 24,
	KEY_R = 1ULL << 25,
	KEY_S = 1ULL << 26,
	KEY_T = 1ULL << 27,
	KEY_U = 1ULL << 28,
	KEY_V = 1ULL << 29,
	KEY_W = 1ULL << 30,
	KEY_X = 1ULL << 31,
	KEY_Y = 1ULL << 32,
	KEY_Z = 1ULL << 33,
	KEY_SHIFT = 1ULL << 34,
	KEY_SPACE = 1ULL << 35,
	KEY_CTRL = 1ULL << 36,
	KEY_ALT = 1ULL << 37,
	KEY_ESC = 1ULL << 38,
	KEY_TAB = 1ULL << 39,
	KEY_ENTER = 1ULL << 40,
	KEY_BACK = 1ULL << 41
};
struct Keyboard{
	unsigned long long buttons;	//Bits siehe enum oben
}; //static Keyboard keyboard;

inline constexpr bool getButton(Keyboard& keyboard, KEYBOARDBUTTON button){return keyboard.buttons & button;}
inline constexpr void setButton(Keyboard& keyboard, KEYBOARDBUTTON button){keyboard.buttons |= button;}
inline constexpr void resetButton(Keyboard& keyboard, KEYBOARDBUTTON button){keyboard.buttons &= ~button;}
