#pragma once

#include <string>
#include <limits>
#include <cassert>
#include <sstream>

#include <utils/string.h>
#include <utils/containers/regions.h>

#include "tree_parser.h"
#include "commands_executor.h"

namespace barnack::text_parser::command_definition
	{
	template <typename CHAR_T>
	struct comment : base<CHAR_T>
		{
		using char_t = typename base<CHAR_T>::char_t;

		virtual std::string name() const noexcept final override { return "comment"; }
		};

	template <typename CHAR_T, typename OUTPUT_CHAR_T>
	struct output_body_base : base<CHAR_T>
		{
		using char_t = typename base<CHAR_T>::char_t;
		using output_char_t = OUTPUT_CHAR_T;
		using output_string_t = std::basic_string<output_char_t>;

		utils::observer_ptr<output_string_t> output_string_ptr;

		virtual void on_child(const typename tree_parser<char_t>::command& command, const typename tokeniser<char_t>::range& child_range) final override
			{
			if (output_string_ptr)
				{
				auto& output_string{*output_string_ptr};
				output_string += utils::string::cast<output_char_t>(child_range.string());
				}
			}
		};

	template <typename CHAR_T, typename OUTPUT_CHAR_T>
	struct output_body_root : output_body_base<CHAR_T, OUTPUT_CHAR_T>
		{
		using char_t        = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::char_t;
		using output_char_t = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::output_char_t;

		virtual std::string name() const noexcept final override { return {}; }

		virtual void validate(const typename tree_parser<char_t>::command& command) const override 
			{
			if (!command.parameters.empty())
				{
				throw std::runtime_error{"Error parsing root command\n"
					"Expects no parameters.\n"
					"Command at: " + command.name.begin.to_string()};
				}
			}
		};

	template <typename CHAR_T, typename OUTPUT_CHAR_T>
	struct output_body : output_body_base<CHAR_T, OUTPUT_CHAR_T>
		{
		using char_t        = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::char_t;
		using output_char_t = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::output_char_t;

		virtual std::string name() const noexcept final override { return "output_body"; }

		virtual void validate(const typename tree_parser<char_t>::command& command) const override
			{
			if (!command.parameters.empty())
				{
				throw std::runtime_error{"Error parsing output_body command\n"
					"Expects no parameters.\n"
					"Command at: " + command.name.begin.to_string()};
				}
			}
		};

	struct runtime_checked_parameters
		{
		struct parameter_type
			{
			struct any {};
			struct number
				{
				float min{-std::numeric_limits<float>::infinity()};
				float max{ std::numeric_limits<float>::infinity()};
				};
			struct identifier
				{
				std::vector<std::string> one_of;
				};
			struct string {};
			//using command = observer_ptr<text_parser>::command ???
			};
		
		using parameter_type_variant = std::variant
			<
			typename parameter_type::any,
			typename parameter_type::number,
			typename parameter_type::identifier,
			typename parameter_type::string//,
			//typename parameter_type::command,
			>;
		
		struct parameters_type
			{
			struct any {};
			using exact = std::vector<parameter_type_variant>;
			struct absent {};
			};
		
		using parameters_type_variant = std::variant
			<
			typename parameters_type::any,
			typename parameters_type::exact,
			typename parameters_type::absent
			>;

		enum class body_requirement { optional, required, absent };

		parameters_type_variant parameters{parameters_type::any{}};
		body_requirement body{body_requirement::optional};

		template <typename char_t>
		void validate(const std::string& command_prototype_name, const typename tree_parser<char_t>::command& command) const
			{
			if (utils::string::cast<char>(command.name.string()) != command_prototype_name)
				{ 
				throw std::runtime_error{"Error parsing command. Name does not match.\n"
					"Expected: \"" + command_prototype_name + "\", received: \"" + utils::string::cast<char>(command.name.string()) + "\"\n"
					"Command at: " + command.name.begin.to_string()};
				}
		
			if (std::holds_alternative<typename parameters_type::any>(parameters))
				{
				}
			else if (std::holds_alternative<typename parameters_type::exact>(parameters))
				{
				const auto& parameters_vec{std::get<parameters_type::exact>(parameters)};
				for (size_t i{0}; i < parameters_vec.size(); i++)
					{
					const auto& parameter_variant{parameters_vec[i]};
					const auto& input_parameter{command.parameters[i]};
		
					if (std::holds_alternative<typename parameter_type::any>(parameter_variant))
						{
						}
					else if (std::holds_alternative<typename parameter_type::number>(parameter_variant))
						{
						const tokeniser<char_t> tokeniser{input_parameter.string()};
						const auto ret{tokeniser.next_number(tokeniser.begin_with_info())};
						if (ret.begin.it != input_parameter.begin.it || ret.end.it != input_parameter.end.it)
							{
							throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
								"Expects number as parameter #" + std::to_string(i) + ",\n"
								"Received \"" + utils::string::cast<char>(input_parameter.string()) + "\" instead.\n"
								"Command at: " + command.name.begin.to_string() + "\n"
								"Parameter at: " + input_parameter.begin.to_string()};
							}
						}
					else if (std::holds_alternative<typename parameter_type::identifier>(parameter_variant))
						{
						const tokeniser<char_t> tokeniser{input_parameter.string()};
						const auto ret{tokeniser.next_identifier(tokeniser.begin_with_info())};
						if (ret.begin.it != input_parameter.begin.it || ret.end.it != input_parameter.end.it)
							{
							throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
								"Expects identifier as parameter #" + std::to_string(i) + ",\n"
								"Received \"" + utils::string::cast<char>(input_parameter.string()) + "\" instead.\n"
								"Command at: " + command.name.begin.to_string() + "\n"
								"Parameter at: " + input_parameter.begin.to_string()};
							}
						}
					else if (std::holds_alternative<typename parameter_type::string>(parameter_variant))
						{
						//TODO check string
						assert(false);
						}
					}
				}
			else if (std::holds_alternative<typename parameters_type::absent>(parameters) && !command.parameters.empty())
				{
				throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
					"Expects no parameters.\n"
					"Command at: " + command.name.begin.to_string()};
				}
		
			if (body == body_requirement::optional)
				{
				}
			else if (body == body_requirement::required)
				{
				throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
					"Expects body.\n"
					"Command at: " + command.name.begin.to_string()};
				}
			else if (body == body_requirement::absent && !command.children.empty())
				{
				throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
					"Expects no body.\n"
					"Command at: " + command.name.begin.to_string()};
				}
			}
		};

	template <typename CHAR_T>
	class replacement_piece
		{
		public:
			using char_t = CHAR_T;

			std::basic_string<char_t> replacement_string_prototype;

			struct replacement_parameters_range_t
				{
				size_t parameter_index;
				size_t index_begin;
				size_t index_end;
				};
			std::vector<replacement_parameters_range_t> replacement_parameters_ranges;
			size_t replacement_string_parameters_count{0};
			
			replacement_piece(const std::string& command_name, const std::basic_string<char_t>& replacement_string_prototype) :
				replacement_string_prototype{replacement_string_prototype}
				{
				generate_replacement_parameters_ranges(command_name);
				}

			std::basic_string<char_t> generate_string(const typename tree_parser<char_t>::command& command) const noexcept
				{
				std::basic_string<char_t> ret;
				
				utils::observer_ptr<const char_t> it{replacement_string_prototype.data()};
				
				for (size_t i{0}; i < replacement_parameters_ranges.size(); i++)
					{
					const auto& replacement_parameter_range{replacement_parameters_ranges[i]};
				
					ret += std::basic_string_view<char_t>{it, replacement_string_prototype.data() + replacement_parameter_range.index_begin};
					ret += command.parameters[replacement_parameter_range.parameter_index].string();
					it = replacement_string_prototype.data() + replacement_parameter_range.index_end;
					}
				ret += std::basic_string_view<char_t>{it, replacement_string_prototype.data() + replacement_string_prototype.size()};
				
				return ret;
				}
				
			void validate(const std::string& command_prototype_name, const typename tree_parser<char_t>::command& command) const
				{
				if (command.parameters.size() < replacement_string_parameters_count)
					{
					throw std::runtime_error{"Error parsing command \"" + command_prototype_name + "\"\n"
						"Expects at least #" + std::to_string(replacement_string_parameters_count) + " parameters,\n"
						"Received " + std::to_string(command.parameters.size()) + " instead.\n"
						"Command at: " + command.name.begin.to_string()};
					}
				}
		private:
			void generate_replacement_parameters_ranges(const std::string& command_name)
				{
				tokeniser<char_t> tokeniser{replacement_string_prototype};
				auto it{tokeniser.begin_with_info()};
				
				while (it.it != tokeniser.end())
					{
					const auto codepoint_with_range{tokeniser.next_codepoint(it)};
				
					if (codepoint_with_range.codepoint == U'\\')
						{
						const auto second_codepoint_with_range{tokeniser.next_codepoint(codepoint_with_range.range.end)};
				
						if (second_codepoint_with_range.codepoint == U'#')
							{
							const auto number_range{tokeniser.next_number(second_codepoint_with_range.range.end)};
							if (number_range.empty())
								{
								throw std::runtime_error{"Error validating command \"" + command_name + "\"\n"
									"Expects number after \"\\#\"\n"
									"at: " + codepoint_with_range.range.begin.to_string()};
								}
							else
								{
								// Causes internal compiler error, converting to std::stringstream instead
								std::stringstream parameter_index_stringstream{utils::string::cast<char>(number_range.string())};
				
								//std::string numbers_range_as_string{string::cast<char, char_t>(number_range.string())};
								//std::stringstream parameter_index_stringstream{numbers_range_as_string};
								size_t parameter_index{0};
								parameter_index_stringstream >> parameter_index;
				
								replacement_string_parameters_count = std::max(replacement_string_parameters_count, parameter_index + 1);
								replacement_parameters_ranges.push_back(replacement_parameters_range_t
									{
									.parameter_index{parameter_index},
									.index_begin    {codepoint_with_range.range.begin.position},
									.index_end      {number_range.end.position}
									});
								}
							}
						}
				
					it = codepoint_with_range.range.end;
					}
				}
		};
		
	template <typename CHAR_T>
	class runtime_defined_replacement : public base<CHAR_T>
		{
		public:
			using char_t = typename base<CHAR_T>::char_t;
		
				
			struct create_info
				{
				std::string name;
				std::basic_string<char_t> replacement_string_before_body_prototype;
				std::basic_string<char_t> replacement_string_after_body_prototype;
				runtime_checked_parameters::parameters_type_variant parameters;
				runtime_checked_parameters::body_requirement body;
				};
			runtime_defined_replacement(const create_info& create_info) : 
				inner_name{create_info.name},
				runtime_checked_parameters{create_info.parameters, create_info.body},
				replacement_piece_before_body{utils::string::cast<char>(create_info.name), create_info.replacement_string_before_body_prototype},
				replacement_piece_after_body {utils::string::cast<char>(create_info.name), create_info.replacement_string_after_body_prototype }
				{
				if (std::holds_alternative<runtime_checked_parameters::parameters_type::any>(runtime_checked_parameters.parameters))
					{
					}
				else if (std::holds_alternative<runtime_checked_parameters::parameters_type::exact>(runtime_checked_parameters.parameters))
					{
					const auto& exact_parameters{std::get<runtime_checked_parameters::parameters_type::exact>(runtime_checked_parameters.parameters)};
					if (exact_parameters.size() < replacement_piece_before_body.replacement_string_parameters_count)
						{
						throw std::runtime_error{"Error validating command \"" + inner_name + "\"\n"
							"Explicit parameters requirement expects " + std::to_string(exact_parameters.size()) + " parameters,\n"
							"Replacement string before body prototype has " + std::to_string(replacement_piece_before_body.replacement_string_parameters_count) + " parameters instead."};
						}
					if (exact_parameters.size() < replacement_piece_after_body.replacement_string_parameters_count)
						{
						throw std::runtime_error{"Error validating command \"" + inner_name + "\"\n"
							"Explicit parameters requirement expects " + std::to_string(exact_parameters.size()) + " parameters,\n"
							"Replacement string after body prototype has " + std::to_string(replacement_piece_after_body.replacement_string_parameters_count) + " parameters instead."};
						}
					}
				else if (std::holds_alternative<runtime_checked_parameters::parameters_type::absent>(runtime_checked_parameters.parameters))
					{
					if (replacement_piece_before_body.replacement_string_parameters_count > 0)
						{
						throw std::runtime_error{"Error validating command \"" + inner_name + "\"\n"
							"Explicit parameters requirement expects no parameters,\n"
							"Replacement string before body prototype has " + std::to_string(replacement_piece_before_body.replacement_string_parameters_count) + " parameters instead."};
						}
					if (replacement_piece_after_body.replacement_string_parameters_count > 0)
						{
						throw std::runtime_error{"Error validating command \"" + inner_name + "\"\n"
							"Explicit parameters requirement expects no parameters,\n"
							"Replacement string after body prototype has " + std::to_string(replacement_piece_after_body.replacement_string_parameters_count) + " parameters instead."};
						}
					}
				}
		private:
			std::string inner_name;
			runtime_checked_parameters runtime_checked_parameters;
			replacement_piece<char_t> replacement_piece_before_body;
			replacement_piece<char_t> replacement_piece_after_body;

		public:
			utils::observer_ptr<commands_executor<char_t>> commands_executor_ptr{nullptr};
		
			virtual std::string name() const noexcept final override
				{
				return inner_name;
				}

			virtual void validate(const typename tree_parser<char_t>::command& command) const override
				{
				runtime_checked_parameters   .validate<char_t>(inner_name, command);
				replacement_piece_before_body.validate        (inner_name, command);
				replacement_piece_after_body .validate        (inner_name, command);
				}
			virtual bool execute_child_commands() const noexcept override { return false; }
			

			virtual void on_begin(const typename tree_parser<char_t>::command& command) final override
				{
				if (!commands_executor_ptr) { throw std::logic_error{"commands_executor_ptr must be assigned before executing an executor which contains this command."}; }
				try
					{
					const auto generated_string_before_body{replacement_piece_before_body.generate_string(command)};
					const auto generated_string_after_body {replacement_piece_after_body .generate_string(command)};
					
					tokeniser<char_t> tokeniser_before_body{generated_string_before_body};
					tokeniser<char_t> tokeniser_after_body {generated_string_after_body };
						
					tree_parser<char_t> parser;
					parser.parse_all(tokeniser_before_body);
					
					auto& top_sequence{*parser.sequences_stack.top()};
					top_sequence.insert(top_sequence.end(), command.children.begin(), command.children.end());
					
					parser.parse_all(tokeniser_after_body);
					
					commands_executor<char_t>& commands_executor{*commands_executor_ptr};
					commands_executor.execute(parser.root);
					}
				catch (const std::exception& e)
					{
					throw std::runtime_error{"Error parsing command \"" + inner_name + "\"\n"
						"Command at: " + command.name.begin.to_string() + "\n"
						"Errors parsing the generated string.\n" + e.what()};
					}
				}
		};


	template <typename CHAR_T, typename OUTPUT_CHAR_T, typename REGIONS_VALUE_TYPE>
	struct region_properties : output_body_base<CHAR_T, OUTPUT_CHAR_T>
		{
		using char_t          = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::char_t;
		using output_char_t   = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::output_char_t;
		using output_string_t = typename output_body_base<CHAR_T, OUTPUT_CHAR_T>::output_string_t;

		using regions_value_type = REGIONS_VALUE_TYPE;
		using regions_t = utils::containers::regions<regions_value_type>;

		using output_body_base<CHAR_T, OUTPUT_CHAR_T>::output_string;

		utils::observer_ptr<regions_t>& output_region_ptr;
		regions_value_type previous_value;

		virtual regions_value_type region_value(const typename tree_parser<char_t>::command& command) = 0;
	
		virtual void on_begin(const typename tree_parser<char_t>::command& command) final override
			{
			if (output_region_ptr)
				{
				auto& output_region{*output_region_ptr};
				const auto last_slot = output_region.at_element_index(output_string.size());
				previous_value = last_slot.value();

				output_region.add(region_value(command), utils::containers::region::create::from(output_string.size()));
				}
			}
	
		virtual void on_end(const typename tree_parser<char_t>::command& command) final override
			{
			if (output_region_ptr)
				{
				auto& output_region{*output_region_ptr};
				output_region.add(previous_value, utils::containers::region::create::from(output_string.size()));
				}
			}
		};







	template <typename CHAR_T, typename OUTPUT_CHAR_T>
	struct unicode_codepoint : base<CHAR_T>
		{
		using char_t = typename base<CHAR_T>::char_t;
		using output_char_t   = OUTPUT_CHAR_T;
		using output_string_t = std::basic_string<output_char_t>;

		utils::observer_ptr<output_string_t> output_string_ptr;

		virtual std::basic_string_view<char_t> name() const noexcept final override
			{
			static std::basic_string<char_t> ret{utils::string::cast<char_t>("unicode_codepoint")}; //TODO constexpr once utf8 library becomes constexpr, if ever
			return ret;
			}

		virtual void on_begin(const typename tree_parser<char_t>::command& command) final override
			{
			const auto& parameter{command.parameters[0]};
			const std::basic_string_view<char_t> hex_number_string{parameter.begin.it + 1/*skip starting 'c'*/, parameter.end.it};
			const char32_t codepoint{utils::string::parse_codepoint(hex_number_string)};
			const std::basic_string<output_char_t> codepoint_as_string{utils::string::codepoint_to_string<output_char_t>(codepoint)};
			
			if (output_string_ptr)
				{
				auto& output_string{*output_string_ptr};
				output_string += codepoint_as_string;
				}
			}

		virtual void validate(const typename tree_parser<char_t>::command& command) const override
			{
			const std::runtime_error error{"Error parsing \"unicode_codepoint\" command\n"
					"Expects an unicode escape sequence (without prior backslash) as parameter and expects no body.\n"
					"Example \"\\unicode_codepoint(u1F604);\" for 😄.\n"
					"Command at: " + command.name.begin.to_string()};

			if (command.parameters.size() != 1 || !command.children.empty())
				{
				throw error;
				}
			const auto& parameter{command.parameters[0]};
			text_parser::tokeniser<char_t> tokeniser{parameter.string()};
			const typename text_parser::tokeniser<char_t>::codepoint_with_range first_codepoint{tokeniser.next_codepoint(tokeniser.begin_with_info())};
			if (first_codepoint.codepoint != U'u')
				{
				throw error;
				}
			}
		};
	}

#ifdef IMPLEMENTATION
#include "commands_definitions.cpp"
#endif