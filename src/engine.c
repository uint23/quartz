#include <stdbool.h>
#include <stdlib.h>

#include "engine.h"
#include "quark.h"

static quark_tab_t tabs[MAX_TABS];
static int tab_count = 0;
static int current_tab = -1;

/* TODO: actually implement webkit */

int engine_get_current_index(void)
{
	/* fetch private */
	return current_tab;
}

int engine_get_tab_count(void)
{
	/* fetch private */
	return tab_count;
}

bool engine_init(void)
{
	LOG_PASS("engine: init");
	return true;
}

void engine_load_url(int id, const char* url)
{
	if (id < 0 || id >= tab_count)
		return;

	free(tabs[id].url);
	tabs[id].url = SDL_strdup(url);
	tabs[id].loading = true;

	LOG_INFO("engine: url=%s loaded to tab=%d", url, id);
}

void engine_shutdown(void)
{
	LOG_INFO("engine: shutting down");
	for (int i = 0; i < tab_count; i++) {
		free(tabs[i].url);
		free(tabs[i].title);
	}
}

void engine_tab_close(int id)
{
	if (id < 0 || id >= tab_count)
		return;

	LOG_INFO("engine: closed tab %d", id);

	free(tabs[id].url);
	free(tabs[id].title);

	/* collapse */
	for (int i = id; i < tab_count-1; i++)
		tabs[i] = tabs[i+1];

	tab_count--;
	if (current_tab >= tab_count)
		current_tab = tab_count - 1;
}

quark_tab_t* engine_tab_current(void)
{
	for (int i = 0; i < tab_count; i++)
		if (i == current_tab)
			return &tabs[i];

	return NULL;
}

int engine_tab_new(const char* url)
{
	if (tab_count >= MAX_TABS) {
		LOG_WARN("engine: maximum number of tabs reached");
		return -1;
	}

	quark_tab_t* t = &tabs[tab_count];
	t->url = SDL_strdup(url ? url : "about_blank");
	t->title = SDL_strdup("new tab"); /* TODO: customise */
	t->loading = true;

	current_tab = tab_count;
	tab_count++;

	LOG_INFO("engine: created new tab: id=%d url=%s", current_tab, url);
	return current_tab;
}

void engine_tab_switch(int id)
{
	if (id < 0 || id >= tab_count)
		return;
	current_tab = id;
	LOG_INFO("engine: switched to tab %d", id);
}

void engine_update(void)
{
	/* nothing here */
}
