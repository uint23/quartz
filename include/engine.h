#pragma once

#include <stdbool.h>

#define MAX_TABS 32 /* TODO: move to config */

typedef struct {
	char*         url;
	char*         title;
	bool          loading;
} quark_tab_t;

int engine_get_current_index(void);
int engine_get_tab_count(void);
bool engine_init(void);
void engine_load_url(int id, const char* url);
void engine_shutdown(void);
void engine_tab_close(int id);
quark_tab_t* engine_tab_current(void);
int engine_tab_new(const char* url);
void engine_tab_switch(int id);
void engine_update(void);
