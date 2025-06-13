#include "tokeniser.h"

#include <utils/third_party/utf8.h>
#include <utils/third_party/unicodelib.h>

namespace barnack::text_parser
	{

	template <typename char_t>
	std::string tokeniser<char_t>::iterator_with_info::to_string() const noexcept
		{
		return
			"global position " + std::to_string(position) +
			", line " + std::to_string(line) +
			", position in line " + std::to_string(position_in_line);
		}


	template <typename char_t>
	typename tokeniser<char_t>::view_t tokeniser<char_t>::range::string() const noexcept { return view_t{begin.it, end.it}; }

	template <typename char_t>
	bool tokeniser<char_t>::range::empty() const noexcept { return string().empty(); }

	template <typename char_t>
	bool tokeniser<char_t>::range::size() const noexcept { return string().size(); }


	template <typename char_t>
	tokeniser<char_t>::tokeniser(view_t string) : string{string} {}


	template <typename char_t>
	typename tokeniser<char_t>::iterator tokeniser<char_t>::begin() const noexcept
		{
		const iterator ret{string.data()};
		return ret;
		}

	template <typename char_t>
	tokeniser<char_t>::iterator_with_info tokeniser<char_t>::begin_with_info() const noexcept
		{
		const iterator_with_info ret{.it{begin()}};
		return ret;
		}

	template <typename char_t>
	typename tokeniser<char_t>::iterator tokeniser<char_t>::end() const noexcept
		{
		const iterator ret{string.data() + string.size()};
		return ret;
		}

	template <typename char_t>
	typename tokeniser<char_t>::codepoint_with_raw_range tokeniser<char_t>::next_codepoint_raw(const typename tokeniser<char_t>::iterator& begin) const noexcept
		{
		iterator end{begin};

		const iterator string_end_it{string.data() + string.size()};

		const char32_t codepoint{[&]()
			{
			if constexpr (std::same_as<char_t, char8_t> || std::same_as<char_t, char>)
				{
				return utf8::next(end, this->end());
				}
			if constexpr (std::same_as<char_t, char16_t>)
				{
				return utf8::next16(end, this->end());
				}
			}()};
		const codepoint_with_raw_range ret
			{
			.codepoint{codepoint},
			.range{begin, end}
			};
		return {ret};
		}

	template <typename char_t>
	typename tokeniser<char_t>::codepoint_with_range tokeniser<char_t>::next_codepoint(const typename tokeniser<char_t>::iterator_with_info& begin) const noexcept
		{
		const codepoint_with_raw_range next_codepoint_ret{next_codepoint_raw(begin.it)};

		//const ptrdiff_t ptrdiff_position_delta{next_codepoint_ret.range.end() - begin.it};
		//assert(ptrdiff_position_delta >= 0);
		//const size_t position_delta{static_cast<size_t>(ptrdiff_position_delta)};
		const size_t position_delta{next_codepoint_ret.range.size()};

		const bool am_newline{next_codepoint_ret.codepoint == U'\n'};

		const codepoint_with_range ret
			{
			.codepoint{next_codepoint_ret.codepoint},
			.range
				{
				.begin{begin},
				.end
					{
					.it              {next_codepoint_ret.range.data() + next_codepoint_ret.range.size()},
					.position        {begin.position + position_delta},
					.line            {am_newline ? (begin.line + 1) : begin.line},
					.position_in_line{am_newline ? 0 : begin.position_in_line + position_delta}
					}
				}
			};
		return ret;
		}


	template <typename char_t>
	typename tokeniser<char_t>::range tokeniser<char_t>::next_whitespace(const typename tokeniser<char_t>::iterator_with_info& begin) const noexcept
		{
		return next_if(begin, [](const codepoint_with_range& cpwr)
			{
			const bool ret{unicode::is_white_space(cpwr.codepoint)};
			return ret;
			});
		}

	template <typename char_t>
	typename tokeniser<char_t>::range tokeniser<char_t>::next_identifier(const typename tokeniser<char_t>::iterator_with_info& begin) const noexcept
		{
		const auto first{next_codepoint(begin)};
		const bool first_valid
			{
			(first.codepoint >= U'a' && first.codepoint <= U'z') ||
			(first.codepoint >= U'A' && first.codepoint <= U'z') ||
			(first.codepoint == U'_')
			};
		if (!first_valid)
			{
			return range{begin, begin};
			}

		const range after_first{next_if(first.range.end, [](const codepoint_with_range& cpwr)
			{
			const bool ret
				{
				(cpwr.codepoint >= U'a' && cpwr.codepoint <= U'z') ||
				(cpwr.codepoint >= U'A' && cpwr.codepoint <= U'z') ||
				(cpwr.codepoint >= U'0' && cpwr.codepoint <= U'9') ||
				(cpwr.codepoint == U'_')
				};
			return ret;
			})};

		return range{begin, after_first.end};
		}

	template <typename char_t>
	typename tokeniser<char_t>::range tokeniser<char_t>::next_number(const typename tokeniser<char_t>::iterator_with_info& begin) const noexcept
		{
		const range first_half{next_if(begin, [](const codepoint_with_range& cpwr)
			{
			const bool ret{cpwr.codepoint >= U'0' && cpwr.codepoint <= U'9'};
			return ret;
			})};

		if (first_half.end.it == this->end()) { return first_half; }

		const auto mid_codepoint{next_codepoint(first_half.end)};
		if (mid_codepoint.codepoint != U'.')
			{
			return first_half;
			}

		if (mid_codepoint.range.end.it == this->end()) { return first_half; }

		const range second_half{next_if(mid_codepoint.range.end, [](const codepoint_with_range& cpwr)
			{
			const bool ret{cpwr.codepoint >= U'0' && cpwr.codepoint <= U'9'};
			return ret;
			})};

		return range{first_half.begin, second_half.end};
		}


	template <typename char_t>
	typename tokeniser<char_t>::range tokeniser<char_t>::next_string(const typename tokeniser<char_t>::iterator_with_info& begin) const noexcept
		{
		const codepoint_with_range first{next_codepoint(begin)};
		if (first.codepoint != U'\"')
			{
			return range{begin, begin};
			}

		codepoint_with_range previous{first};
		while (true)
			{
			if (previous.range.end.it == this->end())
				{
				return range{begin, previous.range.end};
				}
			codepoint_with_range current{next_codepoint(previous.range.end)};

			if ((current.codepoint == U'\"') && (previous.codepoint != U'\\'))
				{
				return range{begin, current.range.end};
				}

			previous = current;
			}
		}



	template <typename char_t>
	bool tokeniser<char_t>::is_whitespace() const noexcept
		{
		const range range{this->next_whitespace(this->begin_with_info())};
		const bool is_full_range{(range.begin.it == this->begin()) && (range.end.it == this->end())};
		return is_full_range;
		}
	
	template <typename char_t>
	bool tokeniser<char_t>::is_identifier() const noexcept
		{
		const range range{this->next_identifier(this->begin_with_info())};
		const bool is_full_range{(range.begin.it == this->begin()) && (range.end.it == this->end())};
		return is_full_range;
		}

	template <typename char_t>
	bool tokeniser<char_t>::is_number() const noexcept
		{
		const range range{this->next_number(this->begin_with_info())};
		const bool is_full_range{(range.begin.it == this->begin()) && (range.end.it == this->end())};
		return is_full_range;
		}

	template <typename char_t>
	bool tokeniser<char_t>::is_string() const noexcept
		{
		const range range{this->next_string(this->begin_with_info())};
		const bool is_full_range{(range.begin.it == this->begin()) && (range.end.it == this->end())};
		return is_full_range;
		}

	template <typename char_t>
	float tokeniser<char_t>::extract_number() const //TODO better function with less error, this was written in haste just to get the rest of the project going
		{
		if (!is_number()) 
			{
			throw std::runtime_error{"Error extracting number from tokeniser.\nTokeniser does not contain a number. Check with \"is_number\" before calling \"extract_number\""};
			}

		float ret{0.f};

		codepoint_with_range cp{next_codepoint(begin_with_info())};
		while (true)
			{
			if (cp.codepoint == U'.')
				{
				break;
				}

			const float digit{static_cast<float>(cp.codepoint - U'0')};
			ret += digit;

			if (cp.range.end.it == this->end())
				{
				return ret;
				}

			ret *= 10.f;
			cp = next_codepoint(cp.range.end);
			}
		cp = next_codepoint(cp.range.end);

		float fractional_digits_multiplier{.1f};
		while (true)
			{
			const float digit{static_cast<float>(cp.codepoint - U'0')};
			ret += digit * fractional_digits_multiplier;

			if (cp.range.end.it == this->end())
				{
				break;
				}

			fractional_digits_multiplier *= .1f;
			cp = next_codepoint(cp.range.end);
			}
		return ret;
		}

	template <typename char_t>
	std::basic_string<char_t> tokeniser<char_t>::extract_string() const
		{
		if (!is_string())
			{
			throw std::runtime_error{"Error extracting string from tokeniser.\nTokeniser does not contain a string. Check with \"is_string\" before calling \"extract_string\""};
			}

		std::basic_string<char_t> ret;

		codepoint_with_range cp{next_codepoint(next_codepoint(begin_with_info()).range.end)};
		while (true)
			{
			if (cp.codepoint == U'\\')
				{
				codepoint_with_range after_backslash{next_codepoint(cp.range.end)};
				if (after_backslash.codepoint == U'\\')
					{
					utf8::append(U'\\', std::back_inserter(ret));//TODO is this the right function?
					}
				else if (after_backslash.codepoint == U'\"')
					{
					utf8::append(U'\"', std::back_inserter(ret));
					}
				else if (after_backslash.codepoint == U't')
					{
					utf8::append(U'\t', std::back_inserter(ret));
					}
				else if (after_backslash.codepoint == U'n')
					{
					utf8::append(U'\n', std::back_inserter(ret));
					}
				else
					{
					throw std::runtime_error{"Error extracting string from tokeniser.\nInvalid escape sequence \"\\" + utils::string::cast<char>(after_backslash.range.string()) + "\""};
					}
				cp = next_codepoint(after_backslash.range.end);
				}
			else if (cp.codepoint == U'\"')
				{
				return ret;
				}
			else if (cp.range.end.it == this->end())
				{
				return ret;
				}
			else
				{
				utf8::append(cp.codepoint, std::back_inserter(ret));
				cp = next_codepoint(cp.range.end);
				}
			}
		}


	template struct tokeniser<char16_t>;
	template struct tokeniser<char8_t>;
	template struct tokeniser<char>;
	}
