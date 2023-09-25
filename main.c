#include <stdio.h>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include <stdint.h>

#include "draw.h"
#include "ui.h"
#include "in.h"
#include "dev.h"
#include "sim.h"

SDL_Window *        m_window;
SDL_GLContext *     m_context;
uint32_t            m_window_width = 800;
uint32_t            m_window_height = 600;

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("oh, shit...\n");
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow("simoslator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
    m_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1);

    GLenum status = glewInit();
    if(status != GLEW_OK)
    {
        printf("oh, crap...\n%s\n", glewGetErrorString(status));
        return -2;
    }

    d_Init();
    ui_Init();
    dev_Init();
    sim_Init();

    glClearColor(0.8, 0.8, 0.8, 1);
    glClearDepth(1);

    struct dev_t *device = dev_CreateDevice(DEV_DEVICE_TYPE_NMOS);
    device->position[1] = -200;

    device = dev_CreateDevice(DEV_DEVICE_TYPE_PMOS);
    device->position[0] = -200;

    uint32_t value = 0;


    while(!in_ReadInput())
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        d_DrawDevices();
        ui_BeginFrame();

        ui_EndFrame();
        SDL_GL_SwapWindow(m_window);
    }

    ui_Shutdown();
}



