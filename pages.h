#pragma once
#include "window.h"
#include "usb.h"

void _default_page_function(void){}
#define MAX_MENUS 2
#define MAX_IMAGES 5
struct Page{
	Menu* menus[MAX_MENUS];
	WORD menu_count = 0;
	Image* images[MAX_IMAGES];	//TODO sollte eigentlich im Menü gespeichert werden
	WORD image_count = 0;
	Font* font = nullptr;		//Sollte auch in Menu sein, vllt?
	void (*code)(void) = _default_page_function;
};

void destroyPage(Page& page){
	for(WORD i=0; i < page.menu_count; ++i){
		delete page.menus[i];
	}
	for(WORD i=0; i < page.image_count; ++i){
		destroyImage(*page.images[i]);
		delete page.images[i];
	}
	if(page.font){
		destroyImage(page.font->image);
		delete page.font;
	}
	page.menu_count = 0;
	page.image_count = 0;
	page.font = nullptr;
}

void updatePage(Page& page, HWND window){
	for(WORD i=0; i < page.menu_count; ++i){
		updateMenu(window, *(page.menus[i]), *page.font);
	}
	page.code();
}
