#include "commands_executor.h"

namespace barnack::text_parser
	{
	template <typename char_t>
	void commands_executor<char_t>::execute(const input_command_t& input_command)
		{
		auto command_definition_it{commands_definitions.find(input_command.name.string())};
		if (command_definition_it == commands_definitions.end())
			{
			throw std::runtime_error{"Error resolving command \"" + utils::string::cast<char>(input_command.name.string()) + "\"\n"
				"Command not found.\n"
				"Command at: " + input_command.name.begin.to_string()};
			}

		auto& command_definition{command_definition_it->second.get()};
		command_definition.validate(input_command);

		command_definition.on_begin(input_command);

		for (const auto& child : input_command.children)
			{
			std::visit([&command_definition, input_command](const auto& child) { command_definition.on_child(input_command, child); }, child);

			if (command_definition.execute_child_commands())
				{
				if (std::holds_alternative<input_command_t>(child))
					{
					const input_command_t& child_command{std::get<input_command_t>(child)};
					execute(child_command);
					}
				}
			}

		command_definition.on_end(input_command);
		}

	template class commands_executor<char16_t>;
	template class commands_executor<char8_t>;
	template class commands_executor<char>;
	}