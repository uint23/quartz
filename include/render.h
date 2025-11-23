#pragma once

#include <stdbool.h>

#include <SDL2/SDL_video.h>

void render_begin(int w, int h);
void render_end(SDL_Window* win);
bool render_init(void);
void render_rect(float x, float y, float w, float h, float r, float g, float b, float a);
void render_shutdown(void);
