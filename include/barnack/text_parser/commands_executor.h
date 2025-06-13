#pragma once

#include <unordered_map>
#include <utils/string.h>
#include "tree_parser.h"

namespace barnack::text_parser
	{
	namespace command_definition
		{
		template <typename CHAR_T>
		struct base
			{
			using char_t = CHAR_T;

			virtual std::basic_string_view<char_t> name() const noexcept = 0;

			virtual void validate(const typename tree_parser<char_t>::command& command) const {}
			virtual void on_begin(const typename tree_parser<char_t>::command& command) {}
			virtual void on_end  (const typename tree_parser<char_t>::command& command) {}
			virtual void on_child(const typename tree_parser<char_t>::command& command, const typename tree_parser<char_t>::command& child_command) {}
			virtual void on_child(const typename tree_parser<char_t>::command& command, const typename tokeniser  <char_t>::range  & child_range  ) {}
			virtual bool execute_child_commands() const noexcept { return true; }
			};
		}


	template <typename T, typename char_t>
	concept commands_observers_iterable_list = std::ranges::range<T> && std::derived_from<std::remove_cvref_t<decltype(**(T{}.begin()))>, command_definition::base<char_t>>;
	template <typename T, typename char_t>
	concept commands_iterable_list = std::ranges::range<T> && std::derived_from<std::remove_cvref_t<decltype(*(T{}.begin()))>, command_definition::base<char_t>>;

	template <typename CHAR_T>
	class commands_executor
		{
		public:
			using char_t         = CHAR_T;
			using view_t         = std::basic_string_view <char_t>;
			using string_t       = std::basic_string      <char_t>;
			using stringstream_t = std::basic_stringstream<char_t>;
			using input_command_t  = typename tree_parser<char_t>::command;

			std::unordered_map<view_t, std::reference_wrapper<command_definition::base<char_t>>> commands_definitions;

			void set_commands(commands_observers_iterable_list<char_t> auto& commands_observers_iterable_list)
				{
				commands_definitions.clear();
				add_commands(commands_observers_iterable_list);
				}
			void set_commands(commands_iterable_list<char_t> auto& commands_observers_iterable_list)
				{
				commands_definitions.clear();
				add_commands(commands_observers_iterable_list);
				}

			void add_commands(commands_observers_iterable_list<char_t> auto& commands_observers_iterable_list)
				{
				for (auto& command_observer : commands_observers_iterable_list)
					{
					commands_definitions.insert({view_t{command_observer->name()}, std::reference_wrapper<command_definition::base<char>>{*command_observer}});
					}
				}
			void add_commands(commands_iterable_list<char_t> auto& commands_iterable_list)
				{
				for (auto& command : commands_iterable_list) { add_command(command); }
				}
			void add_command(std::derived_from<command_definition::base<char_t>> auto& command)
				{
				commands_definitions.insert({view_t{command.name()}, std::reference_wrapper<command_definition::base<char_t>>{command}});
				}

			void execute(const input_command_t& input_command);

		private:
		};
	}

#ifdef IMPLEMENTATION
#include "commands_executor.cpp"
#endif