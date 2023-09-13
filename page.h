#pragma once
#include "window.h"
#include "usb.h"

struct ImageInfo{
	ivec2 pos = {};
	ivec2 size = {};
};

#define PAGEFLAGSTYPE unsigned char
enum PAGEFLAGS : PAGEFLAGSTYPE{
	PAGE_LOAD=1
};
void _default_page_function(HWND){}
#define MAX_MENUS 2
#define MAX_IMAGES 5
struct Page{
	PAGEFLAGSTYPE flags;									//Seitenflags
	Menu* menus[MAX_MENUS];									//Menüs die die Seite beinhalten sollen
	WORD menu_count = 0;									//Anzahl der Menüs auf der Seite
	Image* images[MAX_IMAGES];								//Bilder die auf der Seite angezeigt werden sollen
	ImageInfo imageInfo[MAX_IMAGES];						//Bilderinformation
	WORD image_count = 0;									//Anzahl der Bilder
	Font* font = nullptr;									//Font der Seite
	void (*code)(HWND) = _default_page_function;			//updatePage führt diese Funktion immer aus
	BYTE* data = nullptr;									//Zusätzliche Daten
};

constexpr inline void setPageFlag(Page& page, PAGEFLAGS flag){page.flags |= flag;}
constexpr inline void resetPageFlag(Page& page, PAGEFLAGS flag){page.flags &= ~flag;}
constexpr inline bool getPageFlag(Page& page, PAGEFLAGS flag){return page.flags & flag;}

//Gibt das nächste gesetzte Flag zurück und resetet dieses anschließend
constexpr inline PAGEFLAGS getNextPageFlag(Page& page){
	PAGEFLAGSTYPE flag = page.flags & -page.flags;
	resetPageFlag(page, (PAGEFLAGS)flag);
	return (PAGEFLAGS)flag;
}

//Kopiert die Daten von data in page.data, somit muss sich nicht um eine eigene Speicherverwaltung gekümmert werden
void allocPageData(Page& page, void* data, WORD bytes){
	page.data = new BYTE[bytes];
	for(WORD i=0; i < bytes; ++i){
		page.data[i] = ((BYTE*)data)[i];
	}
}

//Löscht die Daten von page.data
void destroyPageData(Page& page){
	delete[] page.data;
	page.data = nullptr;
}

//Löscht die font nicht!
void destroyPageNoFont(Page& page){
	for(WORD i=0; i < page.menu_count; ++i){
		destroyMenu(*page.menus[i]);
	}
	for(WORD i=0; i < page.image_count; ++i){
		destroyImage(*page.images[i]);
	}
	destroyPageData(page);
	page.menu_count = 0;
	page.image_count = 0;
}

//Löscht die gespeicherte Font mit!
void destroyPage(Page& page){
	for(WORD i=0; i < page.menu_count; ++i){
		destroyMenu(*page.menus[i]);
	}
	for(WORD i=0; i < page.image_count; ++i){
		destroyImage(*page.images[i]);
	}
	if(page.font){
		destroyImage(page.font->image);
	}
	destroyPageData(page);
	page.menu_count = 0;
	page.image_count = 0;
	page.font = nullptr;
}

void updatePage(Page& page, HWND window){
	for(WORD i=0; i < page.image_count; ++i){
		ImageInfo& info = page.imageInfo[i];
		ErrCheck(copyImageToWindow(window, *page.images[i], info.pos.x, info.pos.y, info.size.x, info.size.y));
	}
	for(WORD i=0; i < page.menu_count; ++i){
		updateMenu(window, *(page.menus[i]), *page.font);
	}
	page.code(window);
}
