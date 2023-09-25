#ifndef UI_H
#define UI_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdint.h>

void ui_Init();

void ui_Shutdown();

void ui_MouseMoveEvent(float mouse_x, float mouse_y);

void ui_MouseClickEvent(uint32_t button, uint32_t down);

void ui_KeyboardEvent(uint32_t key, uint32_t key_down);

void ui_TextInputEvent(uint32_t c);

void ui_BeginFrame();

void ui_EndFrame();



#endif