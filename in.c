#include "in.h"
#include "SDL2/SDL.h"

#include "ui.h"

uint32_t            in_text_input;
float               in_mouse_x;
float               in_mouse_y;
extern uint32_t     m_window_width;
extern uint32_t     m_window_height;

uint32_t in_ReadInput()
{
    SDL_Event event;
    uint32_t quit = 0;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                quit = 1;
            break;

            case SDL_MOUSEMOTION:
                ui_MouseMoveEvent((float)event.motion.x, (float)event.motion.y);
                in_mouse_x = ((float)event.motion.x / (float)m_window_width) * 2.0f - 1.0f;
                in_mouse_y = 1.0f - ((float)event.motion.y / (float)m_window_height) * 2.0f;
            break;

            case SDL_KEYDOWN:
                ui_KeyboardEvent(event.key.keysym.scancode, 1);
            break;

            case SDL_KEYUP:
                ui_KeyboardEvent(event.key.keysym.scancode, 0);
            break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                ui_MouseClickEvent(event.button.button, event.type == SDL_MOUSEBUTTONDOWN);
            break;

            case SDL_TEXTINPUT:
                for(uint32_t index = 0; event.text.text[index] && index < sizeof(event.text.text); index++)
                {
                    ui_TextInputEvent(event.text.text[index]);
                }
            break;
        }
    }

    return quit;
}

uint32_t in_SetTextInput(uint32_t enable)
{
    enable = enable && 1;
    
    if(in_text_input != enable)
    {
        if(enable)
        {
            SDL_StartTextInput();
        }
        else
        {
            SDL_StopTextInput();
        }
    }

    in_text_input = enable;
}