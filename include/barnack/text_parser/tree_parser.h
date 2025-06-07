#pragma once

#include <stack>
#include <string>
#include <vector>
#include <memory>
#include <variant>

#include <utils/string.h>
#include <utils/memory.h>

#include "tokeniser.h"

namespace barnack::text_parser
	{
	template <typename CHAR_T>
	class tree_parser
		{
		public:
			tree_parser();

			using char_t         = CHAR_T;
			using view_t         = std::basic_string_view <char_t>;
			using string_t       = std::basic_string      <char_t>;
			using stringstream_t = std::basic_stringstream<char_t>;
			using tokeniser_t    = tokeniser<char_t>;

			struct command;
			using sequence_element = std::variant<command, typename tokeniser_t::range>;
			using sequence = std::vector<sequence_element>;

			struct command
				{
				using parameters_t = std::vector<typename tokeniser_t::range>;
				typename tokeniser_t::range name;
				parameters_t parameters;
				sequence children;
				};

			command root;
			std::stack<utils::observer_ptr<sequence>> sequences_stack;
			
			void parse_all(tokeniser_t& tokeniser);

		private:
			typename tokeniser_t::iterator_with_info step           (tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin);
			typename tokeniser_t::iterator_with_info step_raw       (tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin);
			typename tokeniser_t::iterator_with_info step_command   (tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin);
			typename tokeniser_t::range              next_parameter (tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin);
			typename tokeniser_t::iterator_with_info step_parameters(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin, typename command::parameters_t& parameters_out);
		};
	}

#ifdef IMPLEMENTATION
#include "tree_parser.cpp"
#endif