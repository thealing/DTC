#pragma once

struct Template
{
	std::string name;

	std::string content;

	Template()
	{
	}

	Template(Block block)
	{
		name = block.name;

		content = block.content;
	}
};

class Compiler
{
private:

	std::unordered_map<std::string, Template> _templates;

	std::string _result;

public:

	Compiler()
	{
	}

	void compile(std::string_view string)
	{
		auto start = string.begin();

		auto end = string.end();

		auto it = start;

		while (it < end)
		{
			Block block;

			auto block_end = parser_parse(it, end, block);

			if (block_end != it)
			{
				std::cout << "Parsed block: " << block.name << std::endl;

				std::cout << "CONTENT:\n " << block.content << "\nEND" << std::endl << std::endl;

				auto template_start = block.name.find('$');

				if (template_start != std::string_view::npos)
				{
					auto template_arg_count = std::count(block.name.begin(), block.name.end(), '$');

					auto base = block.name.substr(0, template_start);

					auto key = std::string(base) + '$' + std::to_string(template_arg_count);

					Template temp(block);

					_templates[key] = temp;

					it = block_end;

					continue;
				}
			}

			if (string_is_word(*it))
			{
				auto word_start = it;

				auto word_end = it;

				string_skip_word(word_end, end);

				auto template_start = std::find(it, word_end, '$');

				if (template_start != word_end)
				{
					std::vector<std::string_view> parts;

					ptrdiff_t template_level = 0;

					auto part_start = it;

					while (it != word_end)
					{
						string_find(it, word_end, '$');

						if (template_level == 0)
						{
							std::string_view part(part_start, it);

							parts.push_back(part);

							part_start = it;

							template_level++;
						}

						auto dollar_start = it;

						string_skip(it, word_end, '$');

						auto dollar_count = it - dollar_start;

						template_level += dollar_count - 2;
					}

					if (template_level != -1)
					{
						std::cout << "Error: unterminated template" << std::endl;

						exit(1);
					}

					for (auto a : parts)std::cout << "PART: " << a << std::endl;

					std::string_view base(word_start, template_start);

					int template_arg_count = parts.size() - 1;

					auto key = std::string(base) + '$' + std::to_string(template_arg_count);

					auto template_pair = _templates.find(key);

					if (template_pair != _templates.end())
					{
						std::string_view instance(word_start, word_end);



						std::cout << " INSTANTIATING: " << instance<< std::endl;
					}
				}
				else
				{
					it = word_end;
				}

				continue;
			}
			
			it++;
		}
	}

private:

	void emit_block()
	{

	}
};
