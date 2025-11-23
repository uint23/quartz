#include <stdbool.h>

#include <SDL2/SDL.h>

#include "quark.h"
#include "sdl.h"

SDL_Window* sdl_create_window(const char* title, int w, int h)
{
	return SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
}

bool sdl_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("sdl: SDL_Init failed: %s", SDL_GetError());
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	return true;
}

void sdl_quit(void)
{
	SDL_Quit();
}
