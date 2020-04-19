#ifndef BTWM_LAYOUTS_HPP
#define BTWM_LAYOUTS_HPP

#include <x11.hpp>
#include <utils.hpp>
#include <config.hpp>

#include <variant>
#include <vector>
#include <algorithm>
#include <stack>

namespace btwm {
	inline namespace layouts {
		enum class focus_data {
			has_not_window,
			could_not_focus,
			focus_succeeded
		};
		enum class direction {
			up,
			down,
			left,
			right,
			next,
			prev,
		};


		struct layout_container;
		struct layout_leave{
			x11::window win;
			bool has_win(const x11::window& a_win) {
				return win == a_win;
			}
			void resize(x11::display& display, const rect & r) {
				display.window_to_rect(win, r);
			}
			bool remove_window(const x11::window& a_win) {
				return win == a_win;
			}

			void raise(x11::display& display) {
				display.raise_window(win);
			}

			void focus_any(x11::display& display) const {
				display.set_input_focus(win, x11::revert_to::pointer_root, x11::time::current_time);
			}

			template<direction>
			focus_data focus(x11::display&, const x11::window & w) const {
				return (w == win) ? focus_data::could_not_focus : focus_data::has_not_window;
			}


			template <direction dir>
			focus_data move(x11::display&, const x11::window & w) const {
				return (w == win) ? focus_data::could_not_focus : focus_data::has_not_window;
			}
		};
		using layout_node = std::variant<layout_container, layout_leave>;



		struct layout_vsplit {
			void resize(x11::display& display, const rect & r, std::vector<layout_node>& sub_nodes);

			template <direction dir>
			focus_data focus(x11::display& display, const std::size_t win, std::vector<layout_node>& sub_nodes);

			template <direction dir>
			focus_data move(const std::size_t & w, std::vector<layout_node>& sub_nodes);
			template <direction dir>
			focus_data insert(layout_leave&& value, const std::size_t & w, std::vector<layout_node> & sub_nodes);
		};
		struct layout_hsplit {
			void resize(x11::display& display, const rect & r, std::vector<layout_node>& sub_nodes);

			template <direction dir>
			focus_data focus(x11::display& display, const std::size_t win, std::vector<layout_node>& sub_nodes);

			template <direction dir>
			focus_data move(const std::size_t & w, std::vector<layout_node>& sub_nodes);
			template <direction dir>
			focus_data insert(layout_leave&& value, const std::size_t & w, std::vector<layout_node> & sub_nodes);
		};

		using layout_type = std::variant<layout_vsplit, layout_hsplit>;

		struct layout_container{
			std::vector<layout_node> sub_nodes;
			layout_type type = layout_vsplit();

			template <direction dir>
			focus_data move(const std::size_t & index) {
				return std::visit([&](auto & lt) -> focus_data { return lt.template move<dir>(index, sub_nodes); }, type);
			}

			template <direction dir>
			focus_data insert(layout_leave && leave, const std::size_t & index) {
				return std::visit([&](auto & lt) -> focus_data { return lt.template insert<dir>(std::move(leave), index, sub_nodes); }, type);
			}

			template <direction dir>
			focus_data move_window(const x11::window& win) {
				auto build_stack = [&](auto & container, auto & self) -> focus_data {
					std::size_t index = 0;
					for (auto & elem : container.sub_nodes) {
						switch (elem.index()) {
							case 0: // layout container
								{
									auto & subcon = std::get<layout_container>(elem);
									auto res = self(subcon, self);
									if( subcon.sub_nodes.size() == 1 ) {
										auto tmp_copy = std::move(subcon.sub_nodes.front());
										elem = std::move(tmp_copy);
									}
									if ( res == focus_data::focus_succeeded ) {
										return res;
									}
									else if (res == focus_data::could_not_focus) {
										return container.template insert<dir>(layout_leave{win}, index);
									}
								} break;
							case 1:
								if( std::get<layout_leave>(elem).has_win(win) ) {
									//container.sub_nodes.erase(container.sub_nodes.begin() + index);
									return container.template move<dir>(index);
								} break;
						}
						index++;
					}
					return focus_data::has_not_window;
				};
				auto res = build_stack(*this, build_stack);
				if(res != focus_data::could_not_focus) {
					return res;
				}

				auto new_sub = std::move(*this);
				switch (dir) {
					case direction::up:
						type = layout_hsplit{};
						add(layout_leave{win});
						add(std::move(new_sub));
						break;
					case direction::down:
						type = layout_hsplit{};
						add(std::move(new_sub));
						add(layout_leave{win});
						break;
					case direction::left:
						type = layout_vsplit{};
						add(layout_leave{win});
						add(std::move(new_sub));
						break;
					case direction::right:
						type = layout_vsplit{};
						add(std::move(new_sub));
						add(layout_leave{win});
						break;
				}
				return focus_data::focus_succeeded;
			}


			void focus_any(x11::display& display) {
				std::visit([&](auto & node){node.focus_any(display);}, sub_nodes.front());
			}

			template <direction dir>
			auto focus(x11::display& disp, const std::size_t & index) -> focus_data {
				return std::visit([&](auto & lt) -> focus_data { return lt.template focus<dir>(disp, index, sub_nodes);}, type);
			}

			template <direction dir>
			auto focus_window(x11::display& disp, const x11::window & w) {
				auto recurse = [&](auto & container, auto & self) -> focus_data {
					std::size_t index = 0;
					for(auto & elem: container.sub_nodes) {
						switch (elem.index()) {
							case 0:
								{
									auto & subcon = std::get<layout_container>(elem);
									auto res = self(subcon, self);
									if (res == focus_data::focus_succeeded) {
										return focus_data::focus_succeeded;
									}
									else if (res == focus_data::could_not_focus) {
										return container.template focus<dir>(disp, index);
									}
								}
								break;
							case 1:
								if( std::get<layout_leave>(elem).has_win(w) ) {
									return container.template focus<dir>(disp, index);
								}
								break;
						}
						index++;
					}
					return focus_data::has_not_window;
				};
				recurse(*this, recurse);
			}

			void add(layout_node&& lt_node) { sub_nodes.push_back(std::move(lt_node)); }

			void resize(x11::display& display, const rect& r) { std::visit([&](auto & layout){layout.resize(display, r, sub_nodes);}, type); }

			bool remove_window(const x11::window& win) {
				auto new_end = std::remove_if(std::begin(sub_nodes), std::end(sub_nodes), [&](layout_node & lt) -> bool{
						return std::visit([&](auto & l)->bool{ return l.remove_window(win); }, lt);
						});
				sub_nodes.erase(new_end, std::end(sub_nodes));
				return sub_nodes.empty();
			}

			bool has_win(const x11::window& win) {
				for( auto & node : sub_nodes ) {
					if(std::visit([&](auto& nnode) -> bool{return nnode.has_win(win);}, node)) {
						return true;
					}
				}
				return false;
			}
		};

		void layout_vsplit::resize(x11::display& display, const rect & r, std::vector<layout_node>& sub_nodes) {
			if(sub_nodes.empty()) { return; }
			int spacing_w = static_cast<int>(sub_nodes.size() - 1) * config::gaps;
			int width_per_win = (r.w - spacing_w) / static_cast<int>(sub_nodes.size());
			auto x = r.x;
			for(auto & subnode: sub_nodes) {
				std::visit([&](auto & nnode){ nnode.resize(display, rect{x, r.y, width_per_win, r.h}); }, subnode);
				x += width_per_win + config::gaps;
				}
		}

		void layout_hsplit::resize(x11::display& display, const rect & r, std::vector<layout_node>& sub_nodes) {
			if(sub_nodes.empty()) { return; }
			int spacing_h = static_cast<int>(sub_nodes.size() - 1) * config::gaps;
			int height_per_win = (r.h - spacing_h) / static_cast<int>(sub_nodes.size());
			auto y = r.y;
			for(auto & subnode: sub_nodes) {
				std::visit([&](auto & nnode){ nnode.resize(display, rect{r.x, y, r.w, height_per_win}); }, subnode);
				y += height_per_win + config::gaps;
			}
		}

		template <direction dir>
		focus_data layout_vsplit::focus(x11::display& display, const std::size_t index, std::vector<layout_node>& sub_nodes) {
			switch (dir) {
				case direction::next: [[fallthrough]];
				case direction::right:
					if (index + 1 >= sub_nodes.size()) {
						return focus_data::could_not_focus;
					}
					std::visit([&](auto & node){node.focus_any(display);}, sub_nodes[index + 1]);
					return focus_data::focus_succeeded;

				case direction::prev: [[fallthrough]];
				case direction::left:
					if (index == 0) {
						return focus_data::could_not_focus;
					}
					std::visit([&](auto & node){node.focus_any(display);}, sub_nodes[index - 1]);
					return focus_data::focus_succeeded;
				case direction::up: [[fallthrough]];
				case direction::down:
					return focus_data::could_not_focus;
			}
		}
		template <direction dir>
		focus_data layout_hsplit::focus(x11::display& display, const std::size_t index, std::vector<layout_node>& sub_nodes) {
			switch (dir) {
				case direction::next: [[fallthrough]];
				case direction::down:
					if (index + 1 >= sub_nodes.size()) {
						return focus_data::could_not_focus;
					}
					std::visit([&](auto & node){node.focus_any(display);}, sub_nodes[index + 1]);
					return focus_data::focus_succeeded;

				case direction::prev: [[fallthrough]];
				case direction::up:
					if (index == 0) {
						return focus_data::could_not_focus;
					}
					std::visit([&](auto & node){node.focus_any(display);}, sub_nodes[index - 1]);
					return focus_data::focus_succeeded;
				case direction::left: [[fallthrough]];
				case direction::right:
					return focus_data::could_not_focus;
			}
		}



	template <direction dir>
		focus_data layout_vsplit::insert(layout_leave&& value, const std::size_t & w, std::vector<layout_node> & sub_nodes) {
			switch (dir) {
				case direction::up: [[fallthrough]];
				case direction::down:
					return focus_data::could_not_focus;
				case direction::prev: [[fallthrough]];
				case direction::left:
					if (w >= sub_nodes.size() ) {
						throw std::runtime_error("index out of range");
					}
					sub_nodes.insert(sub_nodes.begin() + w, std::move(value));
					return focus_data::focus_succeeded;
				case direction::next: [[fallthrough]];
				case direction::right:
					if (w >= sub_nodes.size() ) {
						throw std::runtime_error("index out of range");
					}
					sub_nodes.insert(sub_nodes.begin() + w + 1, value);
					return focus_data::focus_succeeded;
			}
		}

		template <direction dir>
		focus_data layout_hsplit::insert(layout_leave&& value, const std::size_t & w, std::vector<layout_node> & sub_nodes) {
			switch (dir) {
				case direction::left: [[fallthrough]];
				case direction::right:
					return focus_data::could_not_focus;
				case direction::prev: [[fallthrough]];
				case direction::up:
					if (w >= sub_nodes.size() ) {
						throw std::runtime_error("index out of range");
					}
					sub_nodes.insert(sub_nodes.begin() + w, value);
					return focus_data::focus_succeeded;
				case direction::next: [[fallthrough]];
				case direction::down:
					if (w >= sub_nodes.size() ) {
						throw std::runtime_error("index out of range");
					}
					sub_nodes.insert(sub_nodes.begin() + w + 1, value);
					return focus_data::focus_succeeded;
			}
		}

		template <direction dir>
		focus_data layout_vsplit::move(const std::size_t & w, std::vector<layout_node>& sub_nodes) {
			if(sub_nodes.empty()) {
				throw std::runtime_error("move: passed empty vector");
			}
			switch (dir) {
				case direction::up: [[fallthrough]];
				case direction::down:
					sub_nodes.erase(sub_nodes.begin() + w);
					return focus_data::could_not_focus;
				case direction::left:
					if (w == 0) {
						sub_nodes.erase(sub_nodes.begin());
						return focus_data::could_not_focus;
					}
					else {
						std::swap(sub_nodes[w], sub_nodes[w - 1]);
						return focus_data::focus_succeeded;
					}
				case direction::right:
					if (w + 1 >= sub_nodes.size()) {
						sub_nodes.pop_back();
						return focus_data::could_not_focus;
					}
					else {
						std::swap(sub_nodes[w], sub_nodes[w + 1]);
						return focus_data::focus_succeeded;
					}
			}
		}
		template <direction dir>
		focus_data layout_hsplit::move(const std::size_t & w, std::vector<layout_node>& sub_nodes) {
			if(sub_nodes.empty()) {
				throw std::runtime_error("move: passed empty vector");
			}
			switch (dir) {
				case direction::left: [[fallthrough]];
				case direction::right:
					sub_nodes.erase(sub_nodes.begin() + w);
					return focus_data::could_not_focus;
				case direction::up:
					if (w == 0) {
						sub_nodes.erase(sub_nodes.begin());
						return focus_data::could_not_focus;
					}
					else {
						std::swap(sub_nodes[w], sub_nodes[w - 1]);
						return focus_data::focus_succeeded;
					}
				case direction::down:
					if (w + 1 >= sub_nodes.size()) {
						sub_nodes.pop_back();
						return focus_data::could_not_focus;
					}
					else {
						std::swap(sub_nodes[w], sub_nodes[w + 1]);
						return focus_data::focus_succeeded;
					}
			}
		}
	}
}

#endif
