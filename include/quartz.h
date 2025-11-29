#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>

/* types */
typedef struct {
	SDL_Window*   win;
	SDL_GLContext gl;
	int           win_w;
	int           win_h;
	bool          running;
} quartz_t;

/* logging */
#define ANSI_RESET     "\x1b[0m"
#define ANSI_FG_WHITE  "\x1b[37m"
#define ANSI_BG_CYAN   "\x1b[46m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_RED    "\x1b[41m"
#define ANSI_BG_GREEN  "\x1b[42m"
#define ANSI_BG_YELLOW "\x1b[43m"

#define LOG(fmt, ...) \
	fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
	fprintf(stdout, ANSI_FG_WHITE ANSI_BG_CYAN " INFO  " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define LOG_PASS(fmt, ...) \
	fprintf(stdout, ANSI_FG_WHITE ANSI_BG_GREEN " PASS  " ANSI_RESET " " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#define LOG_WARN(fmt, ...) \
	fprintf(stderr, ANSI_FG_WHITE ANSI_BG_YELLOW " WARN  " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
	fprintf(stderr, ANSI_FG_WHITE ANSI_BG_RED " ERROR " ANSI_RESET " " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) \
	fprintf(stdout, ANSI_FG_WHITE ANSI_BG_MAGENTA " DEBUG " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif
