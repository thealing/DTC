#pragma once

class Compiler
{
private:

	Template_Registry _template_registry;

	std::set<std::string> _template_instances;

	std::string _result;

	std::vector<std::string_view> _split_scratch;

public:

	Compiler()
	{
	}

	std::string compile(std::string_view string)
	{
		auto it = string.begin();

		auto end = string.end();

		while (it != end)
		{
			Block block;

			auto block_end = parser_parse(it, end, block);

			if (block_end == it)
			{
				_result += *it;

				it++;

				continue;
			}

			_split_scratch.clear();

			auto& parts = _split_scratch;

			std::string pattern;

			bool valid_template = template_split_special(block.name, pattern, std::back_inserter(parts));

			if (valid_template)
			{
				if (parts.size() == 1)
				{
					emit_block(block.content);

					it = block_end;

					continue;
				}

				Template_Block template_block(pattern, block);

				for (auto& part : parts)
				{
					part = template_block.translate(block, part);
				}

				auto base = parts[0];

				auto par_start = parts.begin() + 1;

				auto par_end = parts.end();

				bool added_template = _template_registry.add_template_special(base, par_start, par_end, std::move(template_block));

				if (added_template == false)
				{
					std::cout << "DTC: duplicate template: " << block.name << std::endl;
				}
			}
			else
			{
				std::cout << "DTC: invalid template: " << block.name << std::endl;
			}

			bool remove_padding = ends_with_double_newline();

			if (remove_padding)
			{
				_result.pop_back();

				_result.pop_back();
			}

			it = block_end;
		}

		return std::move(_result);
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
			_split_scratch.clear();

			auto& args = _split_scratch;

			bool valid_template = template_split_instance(instance, std::back_inserter(args));

			if (valid_template == false)
			{
				std::cout << "DTC: invalid template usage: " << instance << std::endl;

				continue;
			}

			auto result = _template_instances.emplace(instance);

			if (result.second)
			{
				auto base = args[0];

				auto arg_start = args.begin() + 1;

				auto arg_end = args.end();

				const Template_Block* template_block = _template_registry.find_special(base, arg_start, arg_end);

				if (template_block == nullptr)
				{
					std::cout << "DTC: overload not found: " << instance << std::endl;

					continue;
				}

				std::string content = template_block->instantiate(instance);

				emit_template(std::move(content), template_block->pattern, arg_start, arg_end);
			}
		}

		_result += block;
	}

	template<typename It>
	void emit_template(std::string content, std::string_view pattern, It arg_start, It arg_end)
	{
		template_replace(content, pattern, arg_start, arg_end);

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
