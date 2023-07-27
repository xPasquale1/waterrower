#pragma once

#include <iostream>

typedef unsigned char BYTE;
typedef unsigned int uint;

struct ivec2{
	int x;
	int y;
	bool lmb;
	bool rmb;
};

//Error-Codes
enum ErrCode{
	SUCCESS = 0,
	BAD_ALLOC,
	TEXTURE_NOT_FOUND,
	MODEL_NOT_FOUND,
	FILE_NOT_FOUND,
	QUEUE_FULL
};
enum ErrCodeFlags{
	NO_ERR_FLAG = 0,
	NO_ERR_OUTPUT,
	ERR_ON_FATAL
};
inline constexpr int ErrCheck(ErrCode code, const char* msg="\0", ErrCodeFlags flags=NO_ERR_FLAG){
	switch(code){
	case BAD_ALLOC:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[BAD_ALLOC ERROR]" << msg << std::endl;
		return ERR_ON_FATAL;
	case TEXTURE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[TEXTURE_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	case MODEL_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[MODEL_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	case FILE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[FILE_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	case QUEUE_FULL:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[QUEUE_FULL ERROR]" << msg << std::endl;
		return 0;
	default: return 0;
	}
	return 0;
}

struct Mouse{
	ivec2 pos;
	char button=0;	//Bits: LMB, RMB, Rest ungenutzt
}; static Mouse mouse;
enum MOUSEBUTTON{
	MOUSE_LMB=1, MOUSE_RMB=2
};
inline constexpr bool getButton(Mouse& mouse, MOUSEBUTTON button){return (mouse.button & button);}
inline constexpr void setButton(Mouse& mouse, MOUSEBUTTON button){mouse.button |= button;}
inline constexpr void resetButton(Mouse& mouse, MOUSEBUTTON button){mouse.button &= ~button;}
