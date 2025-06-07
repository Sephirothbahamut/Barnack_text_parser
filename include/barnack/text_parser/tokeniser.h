#pragma once

#include <string>

#include <utils/string.h>
#include <utils/memory.h>

namespace barnack::text_parser
	{
	template <typename CHAR_T>
	struct tokeniser
		{
		using char_t = CHAR_T;
		using view_t = std::basic_string_view<char_t>;

		//Note: not using string_view's iterators because you can't compare iterators of different string_views, and we're using multiple string_views for sub-parts of the overall string.
		using iterator = utils::observer_ptr<const char_t>;

		struct iterator_with_info
			{
			iterator it;
			size_t position{0};
			size_t line{0};
			size_t position_in_line{0};

			std::string to_string() const noexcept;
			};

		struct range
			{
			iterator_with_info begin;
			iterator_with_info end;
			view_t string() const noexcept;
			bool   empty () const noexcept;
			bool   size  () const noexcept;
			};

		using codepoint = char32_t;

		struct codepoint_with_range
			{
			codepoint codepoint;
			range     range;
			};
		struct codepoint_with_raw_range
			{
			codepoint codepoint;
			view_t range;
			};

		tokeniser(view_t string);

		view_t string;

		iterator begin() const noexcept;
		iterator_with_info begin_with_info() const noexcept;
		iterator end() const noexcept;

		codepoint_with_raw_range next_codepoint_raw(const iterator& begin) const noexcept;
		codepoint_with_range next_codepoint(const iterator_with_info& begin) const noexcept;

		range next_until(const iterator_with_info& begin, auto&& callback) const noexcept
			requires(std::same_as<bool, decltype(callback(codepoint_with_range{}))>)
			{
			iterator_with_info end{begin};
			while (true)
				{
				if (end.it == this->end()) { return {begin, end}; }

				const codepoint_with_range next_codepoint_ret{next_codepoint(end)};
				if(callback(next_codepoint_ret)) { break; }
				end = next_codepoint_ret.range.end;
				}
			return range{begin, end};
			}

		range next_if(const iterator_with_info& begin, auto&& callback) const noexcept
			requires(std::same_as<bool, decltype(callback(codepoint_with_range{}))>)
			{
			return next_until(begin, [&callback](const codepoint_with_range& cpwr)
				{
				const bool ret{!callback(cpwr)};
				return ret;
				});
			}

		range next_whitespace(const iterator_with_info& begin) const noexcept;
		range next_identifier(const iterator_with_info& begin) const noexcept;
		range next_number    (const iterator_with_info& begin) const noexcept;
		};
	}

#ifdef IMPLEMENTATION
#include "tokeniser.cpp"
#endif