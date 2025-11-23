#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <stdlib.h>

#include "engine.h"
#include "quark.h"
#include "render.h"
#include "sdl.h"

quark_t quark;

void init(void);
void quit(void);
void render_tmp(void);
void run(void);

void quit(void)
{
	engine_shutdown();
	render_shutdown();
	sdl_quit();
	LOG_INFO("quark: quitting");
}

void render_tmp(void)
{
	render_begin(quark.win_w, quark.win_h);

	/* top bar */
	render_rect(0, 0, quark.win_w, 32, 0.15f, 0.15f, 0.22f, 1.0f);

	/* number of tabs changes color */
	int n = engine_get_tab_count();
	int cur = engine_get_current_index();

	float intensity = 0.2f + 0.05f * (float)n;
	if (intensity > 1.0f)
		intensity = 1.0f;

	/* tab indicator */
	render_rect(10, 8, 100, 16, intensity, (cur >= 0 ? 0.5f : 0.2f), 0.2f, 1.0f);

	render_end(quark.win);
}

void run(void)
{
	quark.running = true;
	LOG_INFO("quark: starting");

	SDL_Event ev;

	while (quark.running) {
		/* TODO: add event handler + this is just test until WebKit implemented */
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quark.running = false;

			if (ev.type == SDL_WINDOWEVENT) {
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					quark.win_w = ev.window.data1;
					quark.win_h = ev.window.data2;
				}
			}

			if (ev.type == SDL_KEYDOWN) {
				SDL_Keycode key = ev.key.keysym.sym;
				SDL_Keymod mods = ev.key.keysym.mod;

				/* C-t: new tab */
				if ((mods & KMOD_CTRL) && key == SDLK_t) {
					int id = engine_tab_new("about:blank");
					if (id >= 0) {
						LOG_INFO("quark: new tab %d", id);
						engine_tab_switch(id);
					}
				}

				/* C-w: close current tab */
				if ((mods & KMOD_CTRL) && key == SDLK_w) {
					quark_tab_t* cur = engine_tab_current();
					if (cur) {
						LOG_INFO("quark: closing current tab");
						engine_tab_close(engine_get_current_index());
					}
				}

				/* C-t: switch tab */
				if ((mods & KMOD_CTRL) && key == SDLK_TAB) {
					int n = engine_get_tab_count();
					int cur = engine_get_current_index();
					if (n > 0 && cur >= 0) {
						int next = (cur + 1) % n;
						engine_tab_switch(next);
					}
				}
			}
		}

		engine_update();
		render_tmp();
		SDL_Delay(10);
	}
}

void init(void)
{
	sdl_init();
	quark.win = sdl_create_window("quark", 800, 600);
	if (!quark.win) {
		LOG_ERROR("sdl: failed to create quark window");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_PASS("sdl: created SDL window");
	}
	quark.win_w = 800;
	quark.win_h = 600;

	/* init systems */
	quark.gl = SDL_GL_CreateContext(quark.win);
	if (!quark.gl) {
		LOG_ERROR("opengl: failed to create GL context");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_PASS("opengl: created OpenGL ES context");
	}

	if (!render_init()) {
		LOG_ERROR("renderer: failed to initialise renderer");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_PASS("renderer: initialised renderer ");
	}

	if (!engine_init()) {
		LOG_ERROR("engine: failed to init engine");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_ERROR("engine: initialised engine");
	}

	/* starting tab */
	if (engine_tab_new("about:blank") < 0) {
		LOG_ERROR("engine: failed to create initial tab");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_PASS("engine: created initial tab");
	}
}

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;

	init();
	run();
	quit();

	return EXIT_SUCCESS;
}
