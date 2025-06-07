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

	template class tokeniser<char16_t>;
	template class tokeniser<char8_t>;
	template class tokeniser<char>;
	}
