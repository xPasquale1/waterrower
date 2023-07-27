#pragma once

#include <chrono>

typedef unsigned char uchar;
typedef unsigned int uint;

struct ivec2{
	int x;
	int y;
	bool lmb;
	bool rmb;
};

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

class Timer{
    using clock = std::chrono::system_clock;
    clock::time_point m_time_point;
    uchar m_avg_idx = 0;
    float m_avg[8] = {0};
public:
    Timer() : m_time_point(clock::now()){}
    ~Timer(){}
    void start(void){
        m_time_point = clock::now();
    }
    float measure_s(void) const {
        const std::chrono::duration<float> difference = clock::now() - m_time_point;
        return difference.count();
    }
    float measure_ms(void) const {
        const std::chrono::duration<float, std::milli> difference = clock::now() - m_time_point;
        return difference.count();
    }
    float average_ms_qstring(void){
        float out = 0;
        const std::chrono::duration<float, std::milli> difference = clock::now() - m_time_point;
        m_avg[m_avg_idx++] = difference.count();
        if(m_avg_idx > 7) m_avg_idx = 0;
        for(uchar i=0; i < 8; ++i){out += *(m_avg+i);}
        return out/8.;
    }
};
