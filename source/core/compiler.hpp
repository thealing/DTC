#pragma once

struct String_Block
{
	std::string name;

	std::string content;

	String_Block()
	{
	}

	String_Block(Block block)
	{
		name = block.name;

		content = block.content;
	}

	operator Block() const
	{
		Block block;

		block.name = name;

		block.content = content;

		return block;
	}
};

class Compiler
{
private:

	std::unordered_map<std::string, String_Block> _templates;

	std::unordered_set<std::string> _template_instances;

	std::string _result;

public:

	Compiler()
	{
	}

	std::string_view compile(std::string_view string)
	{
		auto result_start_size = _result.size();

		auto it = string.begin();

		auto end = string.end();

		while (it < end)
		{
			Block block;

			auto block_end = parser_parse(it, end, block);

			if (block_end != it)
			{
				std::ostringstream template_key;

				if (template_get_key(block.name, template_key))
				{
					auto key = std::move(template_key).str();

					auto result = _templates.emplace(key, block);

					if (result.second == false)
					{
						std::cout << "DTC: template already defined: " << block.name << std::endl;
					}

					bool remove_padding = ends_with_double_newline();

					if (remove_padding)
					{
						_result.pop_back();

						_result.pop_back();
					}
				}
				else
				{
					emit_block(block.content);
				}

				it = block_end;

				continue;
			}

			_result += *it;
			
			it++;
		}

		std::string_view result_view(_result);

		return result_view.substr(result_start_size);
	}

private:

	void emit_block(std::string_view block)
	{
		auto start = block.begin();

		auto end = block.end();

		std::vector<std::string_view> instances;

		template_get_instances(start, end, std::back_inserter(instances));

		for (auto instance : instances)
		{
			std::vector<std::string_view> args;

			bool valid_template = template_split_instance(instance, std::back_inserter(args));

			if (valid_template == false)
			{
				std::cout << "DTC: invalid template: " << instance << std::endl;

				continue;
			}

			auto arg_count = args.size() - 1;

			std::ostringstream template_key;

			template_get_key(args[0], arg_count, template_key);

			auto key = std::move(template_key).str();

			auto template_pair = _templates.find(key);

			if (template_pair == _templates.end())
			{
				std::cout << "DTC: undeclared template: " << instance << std::endl;

				continue;
			}

			auto result = _template_instances.emplace(instance);

			if (result.second)
			{
				Block template_block = template_pair->second;

				emit_template(template_block, args);
			}
		}

		_result += block;
	}

	void emit_template(Block template_block, const std::vector<std::string_view>& args)
	{
		std::string content(template_block.content);

		auto it = template_get_start(template_block.name);

		auto end = template_block.name.end();

		for (size_t arg_index = 1; arg_index < args.size() && it != end; arg_index++)
		{
			auto par_end = template_get_start(it + 1, end);

			std::string_view par(it, par_end);

			content = template_replace(content, par, args[arg_index]);

			it = par_end;
		}

		emit_block(content);

		_result += '\n';

		bool add_padding = ends_with_double_newline();

		if (add_padding)
		{
			_result += '\n';
		}
	}

	bool ends_with_double_newline()
	{
		if (_result.size() >= 2)
		{
			auto tail = _result.end();

			if (tail[-1] == '\n' && tail[-2] == '\n')
			{
				return true;
			}
		}

		return false;
	}
};
