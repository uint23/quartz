#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <stdlib.h>

#include "engine.h"
#include "quartz.h"
#include "render.h"
#include "sdl.h"

quartz_t quartz;

void init(void);
void quit(void);
void render_tmp(void);
void run(void);

void quit(void)
{
	engine_shutdown();
	render_shutdown();
	sdl_quit();
	LOG_INFO("quartz: quitting");
}

void render_tmp(void)
{
	render_begin(quartz.win_w, quartz.win_h);

	/* top bar */
	render_rect(0, 0, quartz.win_w, 32, 0.15f, 0.15f, 0.22f, 1.0f);

	/* number of tabs changes color */
	int n = engine_get_tab_count();
	int cur = engine_get_current_index();

	float intensity = 0.2f + 0.05f * (float)n;
	if (intensity > 1.0f)
		intensity = 1.0f;

	/* tab indicator */
	render_rect(10, 8, 100, 16, intensity, (cur >= 0 ? 0.5f : 0.2f), 0.2f, 1.0f);

	render_end(quartz.win);
}

void run(void)
{
	quartz.running = true;
	LOG_INFO("quartz: starting");

	SDL_Event ev;

	while (quartz.running) {
		/* TODO: add event handler + this is just test until WebKit implemented */
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quartz.running = false;

			if (ev.type == SDL_WINDOWEVENT) {
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					quartz.win_w = ev.window.data1;
					quartz.win_h = ev.window.data2;
				}
			}

			if (ev.type == SDL_KEYDOWN) {
				SDL_Keycode key = ev.key.keysym.sym;
				SDL_Keymod mods = ev.key.keysym.mod;

				/* C-t: new tab */
				if ((mods & KMOD_CTRL) && key == SDLK_t) {
					int id = engine_tab_new("about:blank");
					if (id >= 0) {
						LOG_INFO("quartz: new tab %d", id);
						engine_tab_switch(id);
					}
				}

				/* C-w: close current tab */
				if ((mods & KMOD_CTRL) && key == SDLK_w) {
					quartz_tab_t* cur = engine_tab_current();
					if (cur) {
						LOG_INFO("quartz: closing current tab");
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
	quartz.win = sdl_create_window("quartz", 800, 600);
	if (!quartz.win) {
		LOG_ERROR("sdl: failed to create quartz window");
		exit(EXIT_FAILURE);
	}
	else {
		LOG_PASS("sdl: created SDL window");
	}
	quartz.win_w = 800;
	quartz.win_h = 600;

	/* init systems */
	quartz.gl = SDL_GL_CreateContext(quartz.win);
	if (!quartz.gl) {
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
