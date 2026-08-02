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
			Block block;

			auto it = parser_parse(string.begin() + index, string.end(), block);

			auto end_index = it - string.begin();

			if (end_index != index)
			{
				std::cout << "Parsed block: " << block.name << std::endl;

				std::cout << "CONTENT:\n " << block.content << "\nEND" << std::endl << std::endl;

				index = end_index;
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
