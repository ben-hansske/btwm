#ifndef BTWM_X11_HPP
#define BTWM_X11_HPP

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
}

#include <utils.hpp>

#include <stdexcept>
#include <algorithm>

namespace btwm {
	namespace x11 {
		using display_base = ::Display;
		using window_base = ::Window;
		enum class window: window_base{};
		using window_changes = ::XWindowChanges;
		using atom_base = ::Atom;
		enum class atom: atom_base {};
		using key_code_base = ::KeyCode;
		enum class key_code: key_code_base {};
		using screen = ::Screen;
		using screen_index = int;
		enum class key_sym: ::KeySym {
			e = XK_E,
			h = XK_H,
			j = XK_J,
			k = XK_K,
			l = XK_L,
			q = XK_Q,
			Return = XK_Return,
			space = XK_space
		};

		using time_base = ::Time;
		enum class time : time_base { current_time };

		using revert_to_base = int;
		enum class revert_to : revert_to_base {
			none = RevertToNone,
			pointer_root = RevertToPointerRoot,
			parent = RevertToParent
		};

		using grab_mode_base = int;
		enum class grab_mode: grab_mode_base {
			sync = GrabModeSync,
			async = GrabModeAsync
		};


		using mod_mask_base = unsigned int;
		enum class mod_mask: mod_mask_base {
			none = 0,
			shift = ShiftMask,
			lock = LockMask,
			control = ControlMask,
			mod1 = Mod1Mask,
			mod2 = Mod2Mask,
			mod3 = Mod3Mask,
			mod4 = Mod4Mask,
			mod5 = Mod5Mask,
			button1 = Button1Mask,
			button2 = Button2Mask,
			button3 = Button3Mask,
			button4 = Button4Mask,
			button5 = Button5Mask,
			any = AnyModifier
		};

		using event_mask_base = long;
		enum class event_mask: event_mask_base {
			no_event              = NoEventMask,
			none                  = NoEventMask,
			key_press             = KeyPressMask,
			key_release           = KeyReleaseMask,
			button_press          = ButtonPressMask,
			button_release        = ButtonReleaseMask,
			enter_window          = EnterWindowMask,
			leave_window          = LeaveWindowMask,
			pointer_motion        = PointerMotionMask,
			pointer_motion_hint   = PointerMotionHintMask,
			button1_motion        = Button1MotionMask,
			button2_motion        = Button2MotionMask,
			button3_motion        = Button3MotionMask,
			button4_motion        = Button4MotionMask,
			button5_motion        = Button5MotionMask,
			button_motion         = ButtonMotionMask,
			keymap_state          = KeymapStateMask,
			exposure              = ExposureMask,
			visibility_change     = VisibilityChangeMask,
			structure_notify      = StructureNotifyMask,
			resize_redirect       = ResizeRedirectMask,
			substructure_notify   = SubstructureNotifyMask,
			substructure_redirect = SubstructureRedirectMask,
			focus_change          = FocusChangeMask,
			property_change       = PropertyChangeMask,
			colormap_change       = ColormapChangeMask,
			owner_grab_button     = OwnerGrabButtonMask
		};

		[[nodiscard]] constexpr auto operator | (const event_mask & a, const event_mask & b) -> event_mask {
			return static_cast<event_mask>(static_cast<event_mask_base>(a) | static_cast<event_mask_base>(b));
		}

		[[nodiscard]] constexpr auto operator | (mod_mask a, mod_mask b) {
			return static_cast<mod_mask>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b) );
		}

		[[nodiscard]] constexpr auto operator & (mod_mask a, mod_mask b) {
			return static_cast<mod_mask>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b) );
		}

		namespace events {
			using error = ::XErrorEvent;
			using event = ::XEvent;
			using create_window = ::XCreateWindowEvent;
			using configure_request = ::XConfigureRequestEvent;
			using map_request = ::XMapRequestEvent;
			using unmap = ::XUnmapEvent;
			using key_pressed = ::XKeyPressedEvent;
			using client_message = ::XClientMessageEvent;
		}

		struct key_match {
			x11::key_code key_code;
			x11::mod_mask include_mask = x11::mod_mask::any;
			x11::mod_mask exclude_mask = x11::mod_mask::none;


			bool matches_mod_mask(x11::mod_mask m) const {
				return (m & include_mask) == include_mask && (m & exclude_mask) == x11::mod_mask::none;
			}
			bool matches_mod_mask(x11::mod_mask_base m) const {
				return matches_mod_mask(static_cast<x11::mod_mask>(m));
			}
			bool matches(x11::key_code kc, x11::mod_mask m) const {
				return matches_mod_mask(m) && kc == key_code;
			}
			bool matches(x11::key_code kc, x11::mod_mask_base m) const {
				return matches_mod_mask(m) && kc == key_code;
			}
		};

		class display {
		public:
			display(): disp(XOpenDisplay(nullptr)) {
				if(!disp) {
					throw std::runtime_error("could not open display\n");
				}
			}
			~display() { XCloseDisplay(disp); }
			[[nodiscard]] auto get() -> x11::display_base& { return *disp; }
			[[nodiscard]] auto default_root_window() -> x11::window { return static_cast<x11::window>(DefaultRootWindow(disp)); }
			[[nodiscard]] auto make_atom_only_if_exists(const char* name) -> x11::atom {
				return static_cast<x11::atom>(XInternAtom(disp, name, true));
			}
			[[nodiscard]] auto make_atom_always(const char* name) -> x11::atom {
				return static_cast<x11::atom>(XInternAtom(disp, name, false));
			}
			[[nodiscard]] auto keysym_to_keycode(const x11::key_sym& s) -> x11::key_code {
				return static_cast<x11::key_code>(XKeysymToKeycode(disp, static_cast<::KeySym>(s)));
			}
			auto select_input(const x11::window &w, const x11::event_mask& m) -> void {
				XSelectInput(disp, static_cast<x11::window_base>(w), static_cast<event_mask_base>(m));
			}
			auto sync(bool discard) -> void {
				XSync(disp, discard);
			}
			[[nodiscard]] auto next_event() -> x11::events::event {
				x11::events::event e;
				XNextEvent(disp, &e);
				return e;
			}
			[[nodiscard]] auto is_protocoll_supported(const x11::window& w, const x11::atom& atm) {
				Atom * supported_procs;
				int cnt;
				if( !XGetWMProtocols(disp, static_cast<x11::window_base>(w), &supported_procs, &cnt) ) {
					return false;
				}
				auto end_it = supported_procs + cnt;
				auto search= std::find(supported_procs, end_it, static_cast<x11::atom_base>(atm));
				XFree(supported_procs);
				return search != end_it;
			}

			void kill_client(const x11::window& w) {
				XKillClient(disp, static_cast<x11::window_base>(w));
			}

			auto send_event(const x11::window& w, bool propagate, const event_mask& ev_mask, x11::events::event& event ) {
				return XSendEvent(disp, static_cast<x11::window_base>(w), propagate, static_cast<event_mask_base>(ev_mask), &event);
			}

			auto default_screen() const {
				return DefaultScreen(disp);
			}
			auto display_width(const x11::screen_index& scr) const {
				return DisplayWidth(disp, scr);
			}
			auto display_height(const x11::screen_index& scr) const {
				return DisplayHeight(disp, scr);
			}
			auto configure_window(const x11::window & w, unsigned int value_mask, x11::window_changes changes) {
				XConfigureWindow(disp, static_cast<x11::window_base>(w), value_mask, &changes);
			}
			auto grab_key(const key_match& k, const x11::window& w, const bool owner_events, x11::grab_mode pointer_mode, x11::grab_mode keyboard_mode) {
				for(x11::mod_mask_base i = 0; i < (1 << 8); ++i) {
					if( k.matches_mod_mask(static_cast<x11::mod_mask>(i)) ) {
						XGrabKey(disp,
								static_cast<x11::key_code_base>(k.key_code),
								i,
								static_cast<x11::window_base>(w),
								owner_events,
								static_cast<x11::grab_mode_base>(pointer_mode),
								static_cast<x11::grab_mode_base>(keyboard_mode));
					}
				}
			}
			auto map_window(const x11::window& w) {
				XMapWindow(disp, static_cast<x11::window_base>(w));
			}
			auto window_to_rect(const x11::window& w, const btwm::rect& r) {
				auto win = static_cast<x11::window_base>(w);
				XMoveWindow(disp, win, r.x, r.y);
				XResizeWindow(disp, win, static_cast<unsigned int>(r.w), static_cast<unsigned int>(r.h));
			}
			auto raise_window(const x11::window& w) {
				XRaiseWindow(disp, static_cast<x11::window_base>(w));
			}
			auto set_input_focus(const x11::window& w, x11::revert_to rev, x11::time t) {
				XSetInputFocus(disp, static_cast<x11::window_base>(w),
						static_cast<x11::revert_to_base>(rev),
						static_cast<x11::time_base>(t));
			}
			void launch_app(std::string program, btwm::array_view<std::string> args) {
				auto pid = fork();
				switch(pid) {
					case -1:
						throw std::runtime_error("forking failed\n");
					case 0: {
							std::vector<char*> sargs;
							sargs.reserve(args.size() + 2);

							sargs.push_back(program.data());
							for(auto & arg : args) {
								sargs.push_back(arg.data());
							}
							sargs.push_back(nullptr);

							if(disp) { close(ConnectionNumber(disp)); }
							setsid();
							execvp(program.c_str(), sargs.data());
							throw std::runtime_error("execvp failed\n");
						}
					default:
						break;
				}
			}
		private:
			x11::display_base*const disp;
		};


		struct atoms {
			const x11::atom wm_delete_window;
			const x11::atom wm_protocols;
			atoms() = delete;
			explicit atoms(x11::display& disp):
				wm_delete_window(disp.make_atom_always("WM_DELETE_WINDOW")),
				wm_protocols(disp.make_atom_always("WM_PROTOCOLS"))
			{ }
		};

		struct key_codes {
			const x11::key_code e;
			const x11::key_code h;
			const x11::key_code j;
			const x11::key_code k;
			const x11::key_code l;
			const x11::key_code q;
			const x11::key_code Return;
			const x11::key_code space;
			key_codes() = delete;
			explicit key_codes(x11::display& disp):
				e(disp.keysym_to_keycode(key_sym::e)),
				h(disp.keysym_to_keycode(key_sym::h)),
				j(disp.keysym_to_keycode(key_sym::j)),
				k(disp.keysym_to_keycode(key_sym::k)),
				l(disp.keysym_to_keycode(key_sym::l)),
				q(disp.keysym_to_keycode(key_sym::q)),
				Return(disp.keysym_to_keycode(key_sym::Return)),
				space(disp.keysym_to_keycode(key_sym::space))
			{ }
		};
	}
}


#endif
