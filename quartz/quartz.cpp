#include <AK/Assertions.h>
#include <AK/ByteString.h>
#include <AK/Format.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Types.h>
#include <AK/Utf16String.h>
#include <LibCore/EventLoop.h>
#include <LibCore/Resource.h>
#include <LibCore/Timer.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/SystemTheme.h>
#include <LibMain/Main.h>
#include <LibURL/Parser.h>
#include <LibWeb/HTML/VisibilityState.h>
#include <LibWeb/UIEvents/KeyCode.h>
#include <LibWeb/UIEvents/MouseButton.h>
#include <LibWebView/Application.h>
#include <LibWebView/ViewImplementation.h>

#include <SDL3/SDL.h>

#include <cstdlib>
#include <cstring>

struct quartz;
struct simple_view;

static quartz* g_app = NULL;
static simple_view* g_view = NULL;

static SDL_SystemCursor cursor_to_sdl(Gfx::StandardCursor c)
{
	switch (c) {
		case Gfx::StandardCursor::Arrow:
			return SDL_SYSTEM_CURSOR_DEFAULT;
		case Gfx::StandardCursor::IBeam:
			return SDL_SYSTEM_CURSOR_TEXT;
		case Gfx::StandardCursor::Hand:
			return SDL_SYSTEM_CURSOR_POINTER;
		case Gfx::StandardCursor::Crosshair:
			return SDL_SYSTEM_CURSOR_CROSSHAIR;
		case Gfx::StandardCursor::Wait:
			return SDL_SYSTEM_CURSOR_WAIT;
		case Gfx::StandardCursor::ResizeHorizontal:
			return SDL_SYSTEM_CURSOR_EW_RESIZE;
		case Gfx::StandardCursor::ResizeVertical:
			return SDL_SYSTEM_CURSOR_NS_RESIZE;
		case Gfx::StandardCursor::ResizeDiagonalTLBR:
			return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
		case Gfx::StandardCursor::ResizeDiagonalBLTR:
			return SDL_SYSTEM_CURSOR_NESW_RESIZE;
		case Gfx::StandardCursor::Disallowed:
			return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
		case Gfx::StandardCursor::Move:
			return SDL_SYSTEM_CURSOR_MOVE;
		default:
			return SDL_SYSTEM_CURSOR_DEFAULT;
	}
}

static void set_cursor(Gfx::StandardCursor c)
{
	static SDL_Cursor* s_cursor = NULL;
	SDL_Cursor* cur = SDL_CreateSystemCursor(cursor_to_sdl(c));
	if (!cur)
		return;

	SDL_SetCursor(cur);

	if (s_cursor)
		SDL_DestroyCursor(s_cursor);

	s_cursor = cur;
}

static Web::UIEvents::KeyCode key_from_sdl(i32 k);
static Web::UIEvents::KeyModifier mods_from_sdl(u16 m);
static Web::UIEvents::MouseButton btn_from_sdl(u8 b);
static Web::UIEvents::MouseButton btns_from_sdl(u32 b);

static void handle_sdl_event(simple_view* v, SDL_Event const* ev);
static void on_ev_tick();
static void on_paint_tick();

static void on_title_change_cb(AK::Utf16String const& t);
static void on_ready_to_paint_cb();
static void on_cursor_change_cb(AK::Variant<Gfx::StandardCursor, Gfx::ImageCursor> const& cursor);

struct simple_view : public WebView::ViewImplementation {
	SDL_Window*           win;
	SDL_Renderer*         ren;
	SDL_Texture*          tex;

	Gfx::IntSize          vp_size;
	int                   w;
	int                   h;

	bool                  is_active;
	bool                  is_dirty;

	/* expose ViewImplementation API (is protected) */
	using ViewImplementation::client;
	using ViewImplementation::enqueue_input_event;
	using ViewImplementation::traverse_the_history_by_delta;

	simple_view(SDL_Window* window, SDL_Renderer* renderer)
	: win(window), ren(renderer), tex(NULL), w(0), h(0),
	is_active(false), is_dirty(true)
	{
		SDL_GetWindowSize(win, &w, &h);
		m_device_pixel_ratio = 1.0f;

		/* single instance callbacks */
		g_view = this;
		ViewImplementation::on_title_change = on_title_change_cb;
		ViewImplementation::on_ready_to_paint = on_ready_to_paint_cb;
		ViewImplementation::on_cursor_change = on_cursor_change_cb;

		initialize_client(CreateNewClient::Yes);
	}

	~simple_view() override
	{
		if (tex)
			SDL_DestroyTexture(tex);
	}

	void update_screen_rects()
	{
		SDL_Rect bounds;

		if (!SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &bounds))
			return;

		Vector<Web::DevicePixelRect> rects;
		rects.append(Web::DevicePixelRect(bounds.x, bounds.y, bounds.w, bounds.h));
		client().async_update_screen_rects(m_client_state.page_index, rects, 0);
	}

	void update_viewport()
	{
		int sw = (int)(w * m_device_pixel_ratio);
		int sh = (int)(h * m_device_pixel_ratio);

		vp_size = { sw, sh };

		client().async_set_viewport_size(
			m_client_state.page_index,
			vp_size.to_type<Web::DevicePixels>());

		handle_resize();
	}

	void initialize_client(CreateNewClient create_new) override
	{
		ViewImplementation::initialize_client(create_new);

		NonnullRefPtr<Core::Resource> ini = MUST(Core::Resource::load_from_uri("resource://themes/Default.ini"_string));
		Core::AnonymousBuffer theme = Gfx::load_system_theme(ini->filesystem_path().to_byte_string())
			.release_value_but_fixme_should_propagate_errors();

		client().async_update_system_theme(m_client_state.page_index, theme);

		update_viewport();
		update_screen_rects();
		client().async_set_window_size(m_client_state.page_index, viewport_size());
	}

	void update_zoom() override
	{
		ViewImplementation::update_zoom();
		update_viewport();
	}

	Web::DevicePixelSize viewport_size() const override
	{
		return vp_size.to_type<Web::DevicePixels>();
	}

	Gfx::IntPoint to_content_position(Gfx::IntPoint p) const override
	{
		return p;
	}

	Gfx::IntPoint to_widget_position(Gfx::IntPoint p) const override
	{
		return p;
	}

	void mark_dirty()
	{
		is_dirty = true;
	}

	bool consume_dirty()
	{
		bool d = is_dirty;
		is_dirty = false;
		return d;
	}

	void resize(int width, int height)
	{
		w = width;
		h = height;

		update_viewport();
		mark_dirty();

		if (tex) {
			SDL_DestroyTexture(tex);
			tex = NULL;
		}
	}

	void set_active(bool active)
	{
		is_active = active;

		set_system_visibility_state(
			active ? Web::HTML::VisibilityState::Visible : Web::HTML::VisibilityState::Hidden
		);

		if (active)
			client().async_set_has_focus(m_client_state.page_index, true);
	}

	u64 page_idx() const
	{
		return m_client_state.page_index;
	}

	void paint()
	{
		Gfx::Bitmap const* bmp = NULL;
		Gfx::IntSize bmp_size;

		if (m_client_state.has_usable_bitmap) {
			bmp = m_client_state.front_bitmap.bitmap.ptr();
			bmp_size = m_client_state.front_bitmap.last_painted_size.to_type<int>();
		}
		else if (m_backup_bitmap) {
			bmp = m_backup_bitmap.ptr();
			bmp_size = m_backup_bitmap_size.to_type<int>();
		}

		SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
		SDL_RenderClear(ren);

		if (bmp) {
			bool need_tex = !tex;

			if (tex) {
				float tw = 0.0f, th = 0.0f;
				SDL_GetTextureSize(tex, &tw, &th);

				if ((int)tw != bmp_size.width() || (int)th != bmp_size.height())
					need_tex = true;
			}

			if (need_tex) {
				if (tex)
					SDL_DestroyTexture(tex);

				tex = SDL_CreateTexture(
					ren,
					SDL_PIXELFORMAT_BGRA32,
					SDL_TEXTUREACCESS_STREAMING,
					bmp_size.width(),
					bmp_size.height());
			}

			if (tex) {
				SDL_UpdateTexture(tex, NULL, bmp->scanline_u8(0), bmp->pitch());

				int win_w = 0, win_h = 0;
				SDL_GetWindowSize(win, &win_w, &win_h);

				SDL_FRect dst = { 0.0f, 0.0f, (float)win_w, (float)win_h };
				SDL_RenderTexture(ren, tex, NULL, &dst);
			}
		}
		SDL_RenderPresent(ren);
	}
};

struct quartz : public WebView::Application { /* this is a disgrace */
	WEB_VIEW_APPLICATION(quartz)

public:
	SDL_Window*           win;
	SDL_Renderer*         ren;
	simple_view*          view;

	quartz() : win(NULL), ren(NULL), view(NULL)
	{
		/* nothing here */
	}

	~quartz() override
	{
		if (view)
			delete view;

		if (ren)
			SDL_DestroyRenderer(ren);

		if (win)
			SDL_DestroyWindow(win);

		SDL_Quit();
	}

	void create_platform_options(WebView::BrowserOptions&, WebView::RequestServerOptions&, WebView::WebContentOptions&) override
	{
		/* nothing here */
	}

	NonnullOwnPtr<Core::EventLoop> create_platform_event_loop() override
	{
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			warnln("sdl init failed: {}", SDL_GetError());
			std::exit(1);
		}

		win = SDL_CreateWindow("Ladybird", 900, 650, SDL_WINDOW_RESIZABLE);
		if (!win) {
			warnln("window create failed: {}", SDL_GetError());
			SDL_Quit();
			std::exit(1);
		}

		ren = SDL_CreateRenderer(win, NULL);
		if (!ren) {
			warnln("renderer create failed: {}", SDL_GetError());
			SDL_DestroyWindow(win);
			SDL_Quit();
			std::exit(1);
		}

		SDL_StartTextInput(win);

		return WebView::Application::create_platform_event_loop();
	}

	Optional<WebView::ViewImplementation&> active_web_view() const override
	{
		if (view)
			return *view;
		return {};
	}

	Utf16String clipboard_text() const override
	{
		if (!SDL_HasClipboardText())
			return {};

		char* txt = SDL_GetClipboardText();
		if (!txt)
			return {};

		Utf16String result = AK::Utf16String::from_utf8(StringView { txt, std::strlen(txt) });
		SDL_free(txt);
		return result;
	}

	Vector<Web::Clipboard::SystemClipboardRepresentation> clipboard_entries() const override
	{
		Vector<Web::Clipboard::SystemClipboardRepresentation> entries;

		if (!SDL_HasClipboardText())
			return entries;

		char* txt = SDL_GetClipboardText();
		if (!txt)
			return entries;

		entries.empend(ByteString { txt, std::strlen(txt) }, "text/plain"_string);
		SDL_free(txt);
		return entries;
	}

	void insert_clipboard_entry(Web::Clipboard::SystemClipboardRepresentation entry) override
	{
		if (entry.mime_type == "text/plain"sv)
			SDL_SetClipboardText(entry.data.characters());
	}
};

static void on_title_change_cb(AK::Utf16String const& t)
{
	if (!g_view || !g_view->win)
		return;

	/* best-effort title: if conversion fails, keep old title */
	ByteString bs_or = t.to_byte_string();
	if (!bs_or.is_empty())
		SDL_SetWindowTitle(g_view->win, bs_or.characters());

	g_view->mark_dirty();
}

static void on_ready_to_paint_cb()
{
	if (!g_view)
		return;

	g_view->mark_dirty();
}

static void on_cursor_change_cb(AK::Variant<Gfx::StandardCursor, Gfx::ImageCursor> const& cursor)
{
	struct cursor_visitor {
		void operator()(Gfx::StandardCursor c) const { set_cursor(c); }
		void operator()(Gfx::ImageCursor const&) const { }
	};

	cursor.visit(cursor_visitor {});
}

static Web::UIEvents::KeyCode key_from_sdl(i32 k)
{
	switch (k) {
		case SDLK_BACKSPACE:
			return Web::UIEvents::Key_Backspace;
		case SDLK_TAB:
			return Web::UIEvents::Key_Tab;
		case SDLK_RETURN:
			return Web::UIEvents::Key_Return;
		case SDLK_ESCAPE:
			return Web::UIEvents::Key_Escape;
		case SDLK_SPACE:
			return Web::UIEvents::Key_Space;
		case SDLK_LEFT:
			return Web::UIEvents::Key_Left;
		case SDLK_RIGHT:
			return Web::UIEvents::Key_Right;
		case SDLK_UP:
			return Web::UIEvents::Key_Up;
		case SDLK_DOWN:
			return Web::UIEvents::Key_Down;
		case SDLK_HOME:
			return Web::UIEvents::Key_Home;
		case SDLK_END:
			return Web::UIEvents::Key_End;
		case SDLK_PAGEUP:
			return Web::UIEvents::Key_PageUp;
		case SDLK_PAGEDOWN:
			return Web::UIEvents::Key_PageDown;
		case SDLK_DELETE:
			return Web::UIEvents::Key_Delete;
		default:
			if (k >= (i32)SDLK_A && k <= (i32)SDLK_Z)
				return (Web::UIEvents::KeyCode)(Web::UIEvents::Key_A + (k - (i32)SDLK_A));
			if (k >= (i32)SDLK_0 && k <= (i32)SDLK_9)
				return (Web::UIEvents::KeyCode)(Web::UIEvents::Key_0 + (k - (i32)SDLK_0));
			return Web::UIEvents::Key_Invalid;
	}
}

static Web::UIEvents::KeyModifier mods_from_sdl(u16 m)
{
	Web::UIEvents::KeyModifier r = Web::UIEvents::KeyModifier::Mod_None;

	if (m & SDL_KMOD_SHIFT)
		r |= Web::UIEvents::KeyModifier::Mod_Shift;

	if (m & SDL_KMOD_CTRL)
		r |= Web::UIEvents::KeyModifier::Mod_Ctrl;

	if (m & SDL_KMOD_ALT)
		r |= Web::UIEvents::KeyModifier::Mod_Alt;

	return r;
}

static Web::UIEvents::MouseButton btn_from_sdl(u8 b)
{
	switch (b) {
		case SDL_BUTTON_LEFT:
			return Web::UIEvents::MouseButton::Primary;
		case SDL_BUTTON_RIGHT:
			return Web::UIEvents::MouseButton::Secondary;
		case SDL_BUTTON_MIDDLE:
			return Web::UIEvents::MouseButton::Middle;
		case SDL_BUTTON_X1:
			return Web::UIEvents::MouseButton::Backward;
		case SDL_BUTTON_X2:
			return Web::UIEvents::MouseButton::Forward;
		default:
			return Web::UIEvents::MouseButton::None;
	}
}

static Web::UIEvents::MouseButton btns_from_sdl(u32 b)
{
	enum Web::UIEvents::MouseButton r = Web::UIEvents::MouseButton::None;

	if (b & SDL_BUTTON_LMASK)
		r |= Web::UIEvents::MouseButton::Primary;

	if (b & SDL_BUTTON_RMASK)
		r |= Web::UIEvents::MouseButton::Secondary;

	if (b & SDL_BUTTON_MMASK)
		r |= Web::UIEvents::MouseButton::Middle;

	return r;
}

static void handle_sdl_event(simple_view* v, SDL_Event const* ev)
{
	if (!v)
		return;

	switch (ev->type) {
	case SDL_EVENT_MOUSE_MOTION: {
		Web::UIEvents::KeyModifier mods = mods_from_sdl(SDL_GetModState());
		Web::DevicePixelPoint pos = Web::DevicePixelPoint { (int)ev->motion.x, (int)ev->motion.y };
		Web::UIEvents::MouseButton btns = btns_from_sdl(ev->motion.state);

		v->enqueue_input_event(Web::MouseEvent {
			Web::MouseEvent::Type::MouseMove, pos, pos,
			Web::UIEvents::MouseButton::None, btns, mods, 0, 0, NULL
		});

		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_DOWN: {
		float mx = 0.0f, my = 0.0f;
		Web::UIEvents::KeyModifier mods = mods_from_sdl(SDL_GetModState());
		Web::DevicePixelPoint pos = Web::DevicePixelPoint { (int)ev->button.x, (int)ev->button.y };
		Web::UIEvents::MouseButton btn = btn_from_sdl(ev->button.button);
		Web::UIEvents::MouseButton btns = btns_from_sdl(SDL_GetMouseState(&mx, &my));

		if (btn == Web::UIEvents::MouseButton::None)
			break;

		v->enqueue_input_event(Web::MouseEvent {
			ev->button.clicks == 2 ? Web::MouseEvent::Type::DoubleClick : Web::MouseEvent::Type::MouseDown,
			pos, pos,
			btn, btns, mods,
			0, 0, NULL
		});

		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_UP: {
		float mx = 0.0f;
		float my = 0.0f;
		Web::UIEvents::KeyModifier mods = mods_from_sdl(SDL_GetModState());
		Web::DevicePixelPoint pos = Web::DevicePixelPoint { (int)ev->button.x, (int)ev->button.y };
		Web::UIEvents::MouseButton btn = btn_from_sdl(ev->button.button);
		Web::UIEvents::MouseButton btns = btns_from_sdl(SDL_GetMouseState(&mx, &my));

		if (btn == Web::UIEvents::MouseButton::None)
			break;

		v->enqueue_input_event(Web::MouseEvent {
			Web::MouseEvent::Type::MouseUp, pos, pos, btn, btns, mods, 0, 0, NULL
		});

		if (btn == Web::UIEvents::MouseButton::Backward)
			v->traverse_the_history_by_delta(-1);
		else if (btn == Web::UIEvents::MouseButton::Forward)
			v->traverse_the_history_by_delta(1);

		break;
	}

	case SDL_EVENT_MOUSE_WHEEL: {
		float mx = 0.0f;
		float my = 0.0f;
		Web::UIEvents::KeyModifier mods = mods_from_sdl(SDL_GetModState());
		Web::UIEvents::MouseButton btns = btns_from_sdl(SDL_GetMouseState(&mx, &my));
		Web::DevicePixelPoint pos = Web::DevicePixelPoint { (int)mx, (int)my };

		v->enqueue_input_event(Web::MouseEvent {
			Web::MouseEvent::Type::MouseWheel, pos, pos,
			Web::UIEvents::MouseButton::None, btns, mods,
			(int)(-ev->wheel.x * 120), (int)(-ev->wheel.y * 120), NULL
		});

		break;
	}

	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP: {
		Web::UIEvents::KeyModifier mods = mods_from_sdl(ev->key.mod);
		Web::UIEvents::KeyCode kc = key_from_sdl(ev->key.key);

		/* TEXT_INPUT generates chars so no need here */
		v->enqueue_input_event(Web::KeyEvent {
			ev->type == SDL_EVENT_KEY_DOWN ? Web::KeyEvent::Type::KeyDown : Web::KeyEvent::Type::KeyUp,
			kc, mods, 0, ev->key.repeat, NULL
		});

		break;
	}

	case SDL_EVENT_TEXT_INPUT: {
		Web::UIEvents::KeyModifier mods = mods_from_sdl(SDL_GetModState());
		for (char const* p = ev->text.text; *p; p++) {
			v->enqueue_input_event(Web::KeyEvent {
				Web::KeyEvent::Type::KeyDown,
				Web::UIEvents::Key_Invalid,
				mods, (u32)(u8)*p, false, NULL
			});
		}
		break;
	}

	case SDL_EVENT_WINDOW_RESIZED:
		v->resize(ev->window.data1, ev->window.data2);
		break;

	case SDL_EVENT_WINDOW_EXPOSED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WINDOW_SHOWN:
		v->mark_dirty();
		break;

	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		v->client().async_set_has_focus(v->page_idx(), true);
		v->mark_dirty();
		break;

	case SDL_EVENT_WINDOW_FOCUS_LOST:
		v->client().async_set_has_focus(v->page_idx(), false);
		break;
	}
}

static void on_ev_tick()
{
	SDL_Event ev;

	if (!g_app || !g_app->view)
		return;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_EVENT_QUIT) {
			Core::EventLoop::current().quit(0);
			return;
		}

		if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) {
			Core::EventLoop::current().quit(0);
			return;
		}

		handle_sdl_event(g_app->view, &ev);
	}
}

static void on_paint_tick()
{
	if (!g_app || !g_app->view)
		return;

	if (g_app->view->consume_dirty())
		g_app->view->paint();
}

ErrorOr<int> ladybird_main(Main::Arguments args)
{
	AK::set_rich_debug_enabled(true);

	quartz* app = TRY(quartz::create(args)).leak_ptr();
	g_app = app;

	app->view = new simple_view(app->win, app->ren);
	app->view->set_active(true);

	/* initial url */
	URL::URL url = URL::Parser::basic_parse("https://lite.duckduckgo.com"sv).release_value();
	app->view->load(url);
	app->view->mark_dirty();

	NonnullRefPtr<Core::Timer> ev_timer = Core::Timer::create_repeating(4, on_ev_tick);
	ev_timer->start();

	NonnullRefPtr<Core::Timer> paint_timer = Core::Timer::create_repeating(16, on_paint_tick);
	paint_timer->start();

	return Core::EventLoop::current().exec();
}
