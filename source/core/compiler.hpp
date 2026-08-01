#pragma once

class Compiler
{
private:

	std::vector<std::string> _types;

	std::string _result;

public:

	Compiler()
	{
	}

	void compile(std::string_view string)
	{
		size_t index = 0;

		while (index < string.size())
		{
			std::string_view block = string.substr(index);

			Parser parser(block);

			parser.parse();

			size_t end_index = parser.get_end_index();

			if (end_index > 0)
			{
				std::cout << "Parsed block: " << parser.get_block().name << std::endl;

				std::cout << "CONTENT:\n " << parser.get_block().content << "\nEND" << std::endl << std::endl;

				index += end_index;
			}
			else
			{
				index++;
			}
		}
	}

private:

	void emit_block()
	{

	}
};
