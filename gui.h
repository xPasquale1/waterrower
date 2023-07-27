#pragma once

#include "util.h"
#include "font.h"

int _defaultEvent(void){return 0;}
struct Button{
	int (*event)(void)=&_defaultEvent;	//Funktionspointer zu einer Funktion die gecallt werden soll wenn der Button gedrückt wird
	std::string text;
	ivec2 pos;
	ivec2 size;
	uchar state=0;	//Bits: Sichtbarkeit, Maus hover aktiv, Maus hover state, Button gedrückt, Rest ungenutzt
	uint color = RGBA(120, 120, 120);
	uint hover_color = RGBA(120, 120, 255);
	uint textcolor = RGBA(180, 180, 180);
	uint textsize=2;
};
enum BUTTONSTATE{
	BUTTON_VISIBLE=1, BUTTON_CAN_HOVER=2, BUTTON_HOVER=4, BUTTON_PRESSED=8
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
				b.event();
				b.state |= BUTTON_PRESSED;
			}
			else if(!getButton(mouse, MOUSE_LMB)) b.state &= ~BUTTON_PRESSED;
		}else if(checkButtonState(b, BUTTON_CAN_HOVER)){
			b.state &= ~BUTTON_HOVER;
		}
	}
}
void draw_rectangle(uint, uint, uint, uint, uint)noexcept;
inline void drawButtons(Button* buttons, uint button_count){
	for(uint i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!checkButtonState(b, BUTTON_VISIBLE)) continue;
		if(checkButtonState(b, BUTTON_CAN_HOVER) && checkButtonState(b, BUTTON_HOVER)) draw_rectangle(b.pos.x, b.pos.y, b.size.x, b.size.y, b.hover_color);
		else draw_rectangle(b.pos.x, b.pos.y, b.size.x, b.size.y, b.color);
		for(size_t i=0; i < b.text.size(); ++i){
			draw_character(b.pos.x+i*10+i+1, b.pos.y+b.size.y/2-b.textsize*5/2, 2, b.text[i], b.textcolor);
		}
	}
}
inline void updateButtons(Button* buttons, uint button_count, Mouse& mouse){
	buttonsClicked(buttons, button_count, mouse);
	drawButtons(buttons, button_count);
}

//Speichert und zeigt x Buttons an
struct Menu{
	Button* buttons;
	uint button_count=0;
	uchar state=0;	//Bits: offen, toggle bit für offen, Rest ungenutzt
	ivec2 pos = {};	//Position in Bildschirmpixelkoordinaten, buttons bekommen zusätzlich diesen offset
};
enum MENUSTATE{
	MENU_OPEN=1, MENU_OPEN_TOGGLE=2
};
inline constexpr bool checkMenuState(Menu& menu, MENUSTATE state){return (menu.state&state);}
inline void updateMenu(Menu& menu, Mouse& mouse){
	if(checkMenuState(menu, MENU_OPEN)){
		updateButtons(menu.buttons, menu.button_count, mouse);
	}
}
