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
	void (*code)(void) = _default_page_function;
};

void destroyPage(Page& page){
	for(WORD i=0; i < page.menu_count; ++i){
		delete page.menus[i];
	}
	for(WORD i=0; i < page.image_count; ++i){
		destroyImage(*page.images[i]);
	}
	page.menu_count = 0;
	page.image_count = 0;
}

void updatePage(Page& page, HWND window){
	for(WORD i=0; i < page.menu_count; ++i){
		updateMenu(window, *(page.menus[i]));
	}
	page.code();
}
