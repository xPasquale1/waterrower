#pragma once

#include <iostream>

typedef unsigned char BYTE;
typedef unsigned int uint;

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
	TOO_MANY_WINDOWS
};
enum ErrCodeFlags{
	NO_ERR_FLAG = 0,
	NO_ERR_OUTPUT,
	ERR_ON_FATAL
};
//TODO ERR_ON_FATAL ausgeben können wenn der nutzer es so möchte
inline constexpr ErrCode ErrCheck(ErrCode code, const char* msg="\0", ErrCodeFlags flags=NO_ERR_FLAG){
	switch(code){
	case BAD_ALLOC:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[BAD_ALLOC ERROR] " << msg << std::endl;
		return BAD_ALLOC;
	case TEXTURE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[TEXTURE_NOT_FOUND ERROR] " << msg << std::endl;
		return TEXTURE_NOT_FOUND;
	case MODEL_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[MODEL_NOT_FOUND ERROR] " << msg << std::endl;
		return MODEL_NOT_FOUND;
	case FILE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[FILE_NOT_FOUND ERROR] " << msg << std::endl;
		return FILE_NOT_FOUND;
	case QUEUE_FULL:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[QUEUE_FULL ERROR] " << msg << std::endl;
		return QUEUE_FULL;
	case WINDOW_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[WINDOW_NOT_FOUND ERROR] " << msg << std::endl;
		return WINDOW_NOT_FOUND;
	case TOO_MANY_WINDOWS:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[TOO_MANY_WINDOWS ERROR] " << msg << std::endl;
		return TOO_MANY_WINDOWS;
	default: return SUCCESS;
	}
	return SUCCESS;
}

enum MOUSEBUTTON{
	MOUSE_LMB=1, MOUSE_RMB=2
};
struct Mouse{
	ivec2 pos = {};
	char button = 0;	//Bits: LMB, RMB, Rest ungenutzt
}; static Mouse mouse;

inline constexpr bool getButton(Mouse& mouse, MOUSEBUTTON button){return (mouse.button & button);}
inline constexpr void setButton(Mouse& mouse, MOUSEBUTTON button){mouse.button |= button;}
inline constexpr void resetButton(Mouse& mouse, MOUSEBUTTON button){mouse.button &= ~button;}
