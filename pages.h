#pragma once
#include "window.h"
#include "usb.h"

struct ImageInfo{
	ivec2 pos = {};
	ivec2 size = {};
};

void _default_page_function(void){}
#define MAX_MENUS 2
#define MAX_IMAGES 5
struct Page{
	Menu* menus[MAX_MENUS];
	WORD menu_count = 0;
	Image* images[MAX_IMAGES];
	ImageInfo imageInfo[MAX_IMAGES];
	WORD image_count = 0;
	Font* font = nullptr;		//TODO Sollte evtl. auch im Menu sein?
	void (*code)(void) = _default_page_function;
};

//Löscht die font nicht!
void destroyPageNoFont(Page& page){
	for(WORD i=0; i < page.menu_count; ++i){
		destroyMenu(*page.menus[i]);
		delete page.menus[i];
	}
	for(WORD i=0; i < page.image_count; ++i){
		destroyImage(*page.images[i]);
		delete page.images[i];
	}
	page.menu_count = 0;
	page.image_count = 0;
}

//Löscht die gespeicherte Font mit!
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
	for(WORD i=0; i < page.image_count; ++i){
		ImageInfo& info = page.imageInfo[i];
		copyImageToWindow(window, *page.images[i], info.pos.x, info.pos.y, info.size.x, info.size.y);
	}
	for(WORD i=0; i < page.menu_count; ++i){
		updateMenu(window, *(page.menus[i]), *page.font);
	}
	page.code();
}
