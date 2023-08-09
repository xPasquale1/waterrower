#pragma once
#include <windows.h>
#include "util.h"

//Nimmt einen raw-video stream und komprimiert diesen in mein video format
ErrCode encodeVideo(){
	return SUCCESS;
}

//Nimmt einen dekodierten video stream und überbringt diesen ins raw-format
//Cacht nur immer ein paar Bilder pro Aufruf um Speicher zu sparen
ErrCode decodeVideo(WORD image_count){
	return SUCCESS;
}
