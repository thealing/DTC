#pragma once

class Compiler
{
private:

	std::unordered_map<std::string, Template_Registry> _templates;

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

			if (block_end == it)
			{
				_result += *it;

				it++;

				continue;
			}

			std::string base;

			std::string key;

			std::string pattern;

			bool valid_template = template_split_special(block.name, base, key, pattern);

			if (valid_template == false)
			{
				std::cout << "DTC: invalid template: " << block.name << std::endl;

				it = block_end;

				continue;
			}

			if (key.empty())
			{
				emit_block(block.content);

				it = block_end;

				continue;
			}

			Template_Block template_block(pattern, block);

			auto& template_registry = _templates[base];

			bool added_template = template_registry.add_template_special(std::move(key), std::move(template_block));

			if (added_template == false)
			{
				std::cout << "DTC: duplicate template: " << block.name << std::endl;
			}

			bool remove_padding = ends_with_double_newline();

			if (remove_padding)
			{
				_result.pop_back();

				_result.pop_back();
			}

			it = block_end;
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
				std::cout << "DTC: invalid template usage: " << instance << std::endl;

				continue;
			}

			std::string base(args[0]);

			auto template_pair = _templates.find(base);

			if (template_pair == _templates.end())
			{
				std::cout << "DTC: undeclared template: " << instance << std::endl;

				continue;
			}

			auto result = _template_instances.emplace(instance);

			if (result.second)
			{
				const Template_Block* template_block = nullptr;

				const auto& template_registry = template_pair->second;

				if (template_registry.find_special(args.begin() + 1, args.end(), template_block) == false)
				{
					std::cout << "DTC: overload not found: " << instance << std::endl;

					continue;
				}

				std::string content = template_block->instantiate(instance);

				emit_template(std::move(content), template_block->pattern, args);
			}
		}

		_result += block;
	}

	void emit_template(std::string content, std::string_view pattern, const std::vector<std::string_view>& args)
	{
		auto it = pattern.begin();

		auto end = pattern.end();

		for (size_t arg_index = 1; arg_index < args.size() && it != end; arg_index++)
		{
			auto par_end = template_get_start(it + 1, end);

			std::string_view par(it, par_end);

			if (par[1] != '*')
			{
				content = template_replace(content, par, args[arg_index]);
			}

			it = par_end;
		}

		bool add_padding = ends_with_double_newline();

		emit_block(content);

		_result += '\n';

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
