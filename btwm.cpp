
#include <memory>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <variant>
#include <algorithm>

#include <config.hpp>
#include <x11.hpp>
#include <utils.hpp>
#include <layouts.hpp>



using namespace btwm;

class bt_window_manager {

public:
	static std::unique_ptr<bt_window_manager> create() {
		return std::make_unique<bt_window_manager>();
	}
	~bt_window_manager() { }
	bt_window_manager():
		m_display(),
		m_root(m_display.default_root_window()),
		atoms(m_display),
		keys(m_display)
	{
		detect_other_wm();
		m_display.select_input(m_root, x11::event_mask::substructure_redirect | x11::event_mask::substructure_notify);
		m_display.sync(false);
		XSetErrorHandler([](x11::display_base *display, x11::events::error *e) -> int{
				const int MAX_ERROR_TEXT_LENGTH = 1024;
				char error_text[MAX_ERROR_TEXT_LENGTH];
				XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
				std::cerr << "Received X error:\n"
				<< "    Request: " << int(e->request_code)
				// << " - " << XRequestCodeToString(e->request_code) << "\n"
				<< "    Error code: " << int(e->error_code)
				<< " - " << error_text << "\n"
				<< "    Resource ID: " << e->resourceid;
				// The return value is ignored.
				return 0;
			});
		screen_rect = get_screen_rect();
		content_rect = {
			screen_rect.x + config::outer_gaps,
			screen_rect.y + config::outer_gaps,
			screen_rect.w - 2*config::outer_gaps,
			screen_rect.h - 2*config::outer_gaps
		};

		auto grab_super = [&](const x11::key_code& k) {
			m_display.grab_key( { k, x11::mod_mask::mod4,
					x11::mod_mask::lock | x11::mod_mask::control | x11::mod_mask::mod1 },
					m_root, false,
					x11::grab_mode::async, x11::grab_mode::async
				);
		};

		grab_super(keys.Return);
		grab_super(keys.e);
		grab_super(keys.space);

	}

	int run() {

		for(;;) {
			auto e = m_display.next_event();

			switch(e.type) {
				case CreateNotify: break;
				case DestroyNotify: break;
				case ReparentNotify: break;
				case ButtonPress: break;
				case ConfigureRequest:
					on_configure_request(e.xconfigurerequest);
					break;
				case MapRequest:
					on_map_request(e.xmaprequest);
					break;
				case UnmapNotify:
					on_unmap(e.xunmap);
					break;
				case KeyPress:
					if( on_key_press(e.xkey) ) {
						return 0;
					}
					break;
				default:
					std::cout << "unknown event; ignored.\n";
			}
		}
	}

private:
	btwm::layout_container root_layout;
	btwm::rect screen_rect;
	btwm::rect content_rect;



	void kill_window(const x11::window& w) {
		if( m_display.is_protocoll_supported(w, atoms.wm_delete_window) ) {
			x11::events::event event;
			auto& msg = event.xclient;
			msg.type = ClientMessage;
			msg.message_type = static_cast<x11::atom_base>(atoms.wm_protocols);
			msg.window = static_cast<x11::window_base>(w);
			msg.format = 32;
			msg.data.l[0] = static_cast<x11::atom_base>(atoms.wm_delete_window);
			if(!m_display.send_event(w, false, x11::event_mask::none, event)) {
				throw std::runtime_error("error sending kill message");
			}
		}
		else {
			m_display.kill_client(w);
		}
	}

	bool on_key_press(const x11::events::key_pressed& e) {
		auto mod_match = [&](const x11::mod_mask& include, const x11::mod_mask& exclude) {
			const auto mods = static_cast<x11::mod_mask>(e.state);

			return (mods & include) == include &&
				(mods & exclude) == x11::mod_mask::none;
		};
		auto key = static_cast<x11::key_code>(e.keycode);
		auto win = static_cast<x11::window>(e.window);

		if (mod_match(x11::mod_mask::mod4 | x11::mod_mask::shift, x11::mod_mask::mod1)) {
			if (key == keys.q) {
				std::cout << "kill" <<std::endl;
				kill_window(win);
				root_layout.resize(m_display, content_rect);
			}
			else if (key == keys.h) {
				root_layout.template move_window<btwm::direction::left>(win);
				root_layout.resize(m_display, content_rect);
			} else if (key == keys.j) {
				root_layout.template move_window<btwm::direction::down>(win);
				root_layout.resize(m_display, content_rect);
			} else if (key == keys.k) {
				root_layout.template move_window<btwm::direction::up>(win);
				root_layout.resize(m_display, content_rect);
			} else if (key == keys.l) {
				root_layout.template move_window<btwm::direction::right>(win);
				root_layout.resize(m_display, content_rect);
			}
			else if (key == keys.e) {
				return true;
			}
			else {
				std::clog << "unknown key pressed: 1\n";
			}
		}
		else if (mod_match(x11::mod_mask::mod4, x11::mod_mask::shift | x11::mod_mask::mod1)) {
			if (key == keys.h) {
				root_layout.template focus_window<btwm::direction::left>(m_display, win);
			} else if (key == keys.j) {
				root_layout.template focus_window<btwm::direction::down>(m_display, win);
			} else if (key == keys.k) {
				root_layout.template focus_window<btwm::direction::up>(m_display, win);
			} else if (key == keys.l) {
				root_layout.template focus_window<btwm::direction::right>(m_display, win);
			} else if (key == keys.Return) {
				std::cout << "st" << std::endl;
				//std::system("dmenu_run");

				auto run_st = std::array<std::string,0>{};
				m_display.launch_app("st",run_st);
			} else if (key == keys.space) {
				auto run_st = std::array<std::string,0>{};
				m_display.launch_app("dmenu_run",run_st);

			} else if (key == keys.e) {
				if(std::holds_alternative<btwm::layout_vsplit>(root_layout.type)){
					root_layout.type = btwm::layout_hsplit{};
				} else {
					root_layout.type = btwm::layout_vsplit{};
				}
				root_layout.resize(m_display, content_rect);
			}

			else {
				std::clog << "unknown key pressed: 2\n";
			}
		}
		else {
			std::clog << "unknown key pressed: 3\n";
		}

		return false;
	}

	auto get_screen_rect() const -> btwm::rect{
		btwm::rect r;
		r.x = 0;
		r.y = 0;
		auto screen = m_display.default_screen();
		r.w = m_display.display_width(screen);
		r.h = m_display.display_height(screen);
		return r;
	}

	void detect_other_wm() {
		XSetErrorHandler([](x11::display_base*, x11::events::error*) -> int {throw std::runtime_error("other wm running"); });
	}


	void on_configure_request(const x11::events::configure_request& e) {
		x11::window_changes changes;
		changes.x = e.x;
		changes.y = e.y;
		changes.width = e.width;
		changes.height = e.height;
		changes.border_width = e.border_width;
		changes.sibling = e.above;
		changes.stack_mode = e.detail;
		m_display.configure_window(
				static_cast<x11::window>(e.window), static_cast<unsigned int>(e.value_mask), changes);
	}

	void on_map_request(const x11::events::map_request& e) {
		auto win = static_cast<x11::window>(e.window);

		auto grab_super_shift = [&](const x11::key_code& k) {
		m_display.grab_key(
			{ k, x11::mod_mask::mod4 | x11::mod_mask::shift,
			x11::mod_mask::lock | x11::mod_mask::control | x11::mod_mask::mod1 },
			win, false,
			x11::grab_mode::async, x11::grab_mode::async);
		};

		auto grab_super = [&](const x11::key_code& k) {
			m_display.grab_key( { k, x11::mod_mask::mod4,
					x11::mod_mask::lock | x11::mod_mask::control | x11::mod_mask::mod1 },
					win, false,
					x11::grab_mode::async, x11::grab_mode::async
				);
		};

		grab_super_shift(keys.q);

		grab_super(keys.h);
		grab_super(keys.j);
		grab_super(keys.k);
		grab_super(keys.l);
		grab_super(keys.Return);



		m_display.map_window(win);
		root_layout.add(btwm::layout_leave{win});
		root_layout.resize(m_display, content_rect);
	}

	void on_unmap(const x11::events::unmap& e) {
		if ( !root_layout.remove_window(static_cast<x11::window>(e.window)) ) {
			root_layout.resize(m_display, content_rect);
			root_layout.focus_any(m_display);
		}
	}

	x11::display m_display;
	const x11::window m_root;
	const x11::atoms atoms;
	const x11::key_codes keys;
};

int main() {
	auto wm = bt_window_manager::create();
	return wm->run();

}
