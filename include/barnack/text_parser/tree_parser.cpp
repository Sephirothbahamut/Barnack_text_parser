#include "tree_parser.h"

#include <cassert>
#include <stdexcept>

namespace barnack::text_parser
	{
	template <typename char_t>
	tree_parser<char_t>::tree_parser()
		{
		sequences_stack.push(std::addressof(root.children));
		}

	template <typename char_t>
	void tree_parser<char_t>::parse_all(tokeniser_t& tokeniser)
		{
		typename tokeniser_t::iterator_with_info it{tokeniser.begin_with_info()};
		while (it.it != tokeniser.end())
			{
			it = step(tokeniser, it);
			}
		}

	template <typename char_t>
	typename tree_parser<char_t>::tokeniser_t::iterator_with_info tree_parser<char_t>::step(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin)
		{
		const auto first_codepoint{tokeniser.next_codepoint(begin)};
		if (first_codepoint.codepoint == U'}')
			{
			if (sequences_stack.size() <= 1) 
				{
				throw std::runtime_error{"Curly brackets closed found without there being a matched opening.\n" + begin.to_string()};
				}
			sequences_stack.pop();
			return first_codepoint.range.end;
			}
		else if (first_codepoint.codepoint == U'\\')
			{
			return step_command(tokeniser, first_codepoint.range.end);
			}
		else 
			{
			return step_raw(tokeniser, begin);
			}
		}


	template <typename char_t>
	typename tree_parser<char_t>::tokeniser_t::iterator_with_info tree_parser<char_t>::step_raw(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin)
		{
		const typename tokeniser_t::range raw_text{tokeniser.next_until(begin, [](const tokeniser_t::codepoint_with_range& cpwr) -> bool
			{
			const auto ret{cpwr.codepoint == U'}' || cpwr.codepoint == U'\\'};
			return ret;
			})};

		//Only called if the first character is already valid as raw content, so the raw_text view should never be empty.
		if(true && !raw_text.empty())
			{
			assert(!raw_text.empty());//If this ever triggers (it shouldn't) remove the true from above. Or try to understand why it's happening to begin with.
			auto& topmost_sequence{*(sequences_stack.top())};
			auto& emplaced{std::get<tokeniser_t::range>(topmost_sequence.emplace_back(raw_text))};
			}

		return {raw_text.end};
		}


	template <typename char_t>
	typename tree_parser<char_t>::tokeniser_t::iterator_with_info tree_parser<char_t>::step_command(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin)
		{
		const typename tokeniser_t::range command_name{tokeniser.next_identifier(begin)};

		//Only called if the first character is already valid as raw content, so the raw_text view should never be empty.
		if (command_name.empty())
			{
			throw std::runtime_error
				{
				"Empty command. \"\\\" should be followed by a valid identifier\n"
				"An identifier is a sequence of lower or upper case latin alphabet non-decorated letters, arabic numerals, and underscores.\n"
				"It also cannot begin with arabic numerals.\n"
				"Examples: \n"
				"\t\\something;\n" 
				"\t\\stuff_123\n" +
				begin.to_string()
				};
			}

		auto& topmost_sequence{*(sequences_stack.top())};
		auto& emplaced{std::get<command>(topmost_sequence.emplace_back(command{.name{command_name}}))};

		auto next_codepoint{tokeniser.next_codepoint(command_name.end)};
		if (next_codepoint.codepoint == U'(')
			{
			const auto step_parameters_end{step_parameters(tokeniser, next_codepoint.range.end, emplaced.parameters)};
			next_codepoint = tokeniser.next_codepoint(step_parameters_end);
			}

		if (next_codepoint.codepoint == U'{')
			{
			//finalize command and add its children vector to the stack
			sequences_stack.push(&emplaced.children);
			return next_codepoint.range.end;
			}
		else if(next_codepoint.codepoint == U';')
			{
			return next_codepoint.range.end;
			}
		else
			{
			throw std::runtime_error
				{
				"Invalid command termination.\n"
				"Commands should be either followed by a curly brackets enclosed block, or a semicolon\n"
				"Examples: \n"
				"\t\\command;\n" 
				"\t\\command{content}\n" 
				"\t\\command(paramters);\n"
				"\t\\command(paramters){content}\n" +
				begin.to_string()
				};
			}
		}


	template <typename char_t>
	typename tree_parser<char_t>::tokeniser_t::range tree_parser<char_t>::next_parameter(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin)
		{
		typename tokeniser_t::range ret{tokeniser.next_identifier(begin)};
		if (ret.empty()) 
			{
			ret = tokeniser.next_number(begin);
			}
		if (ret.empty())
			{
			throw std::runtime_error
				{
				"Invalid command parameter. Command parameters must be valid identifier or number\n"
				"An identifier is a sequence of lower or upper case latin alphabet non-decorated letters, arabic numerals, and underscores.\n"
				"A number is a sequence of arabic numerals, with one or no dot as decimal separator.\n"
				"Examples: \n"
				"\tparam\n"
				"\tparam_qwerty_456\n"
				"\t123456\n"
				"\t123.456\n"
				"\t.123\n"
				"\t123.\n" +
				begin.to_string()
				};
			}
		return ret;
		}


	template <typename char_t>
	typename tree_parser<char_t>::tokeniser_t::iterator_with_info tree_parser<char_t>::step_parameters(tokeniser_t& tokeniser, const typename tokeniser_t::iterator_with_info& begin, typename command::parameters_t& parameters_out)
		{
		typename tokeniser_t::iterator_with_info it{begin};
				
		while (true)
			{
			it = tokeniser.next_whitespace(it).end;
			const auto parameter{next_parameter(tokeniser, it)};
			it = parameter.end;
			parameters_out.emplace_back(parameter);
			it = tokeniser.next_whitespace(it).end;
			const auto next_codepoint{tokeniser.next_codepoint(it)};
			if (next_codepoint.codepoint == U')')
				{
				return next_codepoint.range.end;
				}
			if (next_codepoint.codepoint == U',')
				{
				it = next_codepoint.range.end;
				}
			else
				{
				throw std::runtime_error
					{
					"Invalid command parameters. Command parameters must be a round brackets enclosed sequence of comma separated valid identifiers or numbers\n"
					"An identifier is a sequence of lower or upper case latin alphabet non-decorated letters, arabic numerals, and underscores.\n"
					"A number is a sequence of arabic numerals, with one or no dot as decimal separator.\n"
					"A comma is \",\" :)\n"
					"Examples: \n"
					"\t(param, param_qwerty_456, 123456)\n"
					"\t(123.456, .123, 123.)\n" +
					begin.to_string()
					};
				}
			}
		}

	template class tree_parser<char16_t>;
	template class tree_parser<char8_t>;
	template class tree_parser<char>;
	}