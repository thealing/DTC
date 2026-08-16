#pragma once

class String_View_Streambuf : public std::streambuf
{
public:
	String_View_Streambuf(std::string_view view)
	{
		auto* data = const_cast<char*>(view.data());

		setg(data, data, data + view.size());
	}
};

class String_View_Istream : public std::istream
{
	String_View_Streambuf buffer;

public:
	String_View_Istream(std::string_view view)
		: std::istream(&buffer), buffer(view)
	{
	}
};

class Compiler
{
private:

	Template_Registry _template_registry;

	std::set<std::string> _template_instances;

	std::string _result;

	std::vector<std::string_view> _split_buffer;

	std::vector<std::string> _file_names;

	std::vector<std::pair<std::string_view, size_t>> _template_locations;

	std::vector<std::tuple<size_t, size_t, std::string_view>> _origin_stack;

public:

	Compiler()
	{
	}

	std::string compile(std::string_view path, std::string_view content)
	{
		auto start = content.begin();

		auto end = content.end();

		auto it = start;

		std::string_view file_name = _file_names.emplace_back(path);

		size_t line_number = 1;

		size_t line_offset = 0;

		while (it != end)
		{
			if (*it == '#')
			{
				it++;

				string_skip_inline_space(it, end);

				auto directive_start = it;

				while (true)
				{
					if (string_find(it, end, '\n') == false)
					{
						break;
					}

					if (it[-1] != '\\')
					{
						break;
					}

					it++;
				}

				std::string_view directive(directive_start, it);

				if (directive.starts_with("line"))
				{
					directive.remove_prefix(4);

					String_View_Istream directive_stream(directive);

					directive_stream >> line_number;

					line_number--;

					line_offset = std::distance(start, it);

					auto& file_name_string = _file_names.back();

					directive_stream >> std::quoted(file_name_string);

					file_name = file_name_string;
				}

				continue;
			}

			Block block;

			auto block_end = parser_parse(it, end, block);

			if (block_end == it)
			{
				_result += *it;

				it++;

				continue;
			}

			size_t name_offset = block.name.data() - content.data();

			while (line_offset != name_offset)
			{
				if (content[line_offset] == '\n')
				{
					line_number++;
				}

				line_offset++;
			}

			_split_buffer.clear();

			auto& parts = _split_buffer;

			std::string pattern;

			bool valid_template = template_split_special(block.name, pattern, std::back_inserter(parts));

			if (valid_template)
			{
				if (parts.size() == 1)
				{
					_origin_stack.emplace_back(SIZE_MAX, line_number, block.name);

					emit_block(block.content);

					_origin_stack.pop_back();

					it = block_end;

					continue;
				}

				size_t template_id = _template_locations.size();

				Template_Block template_block(template_id, pattern, block);

				for (auto& part : parts)
				{
					part = template_block.translate(block, part);
				}

				auto base = parts[0];

				auto par_start = parts.begin() + 1;

				auto par_end = parts.end();

				auto previous_template = _template_registry.add_template_special(base, par_start, par_end, std::move(template_block));

				if (previous_template != nullptr)
				{
					std::cerr << file_name << "(" << line_number << "): ";

					std::cerr << "error: template already defined: " << block.name << std::endl;

					auto previous_template_id = previous_template->get_id();

					auto previous_template_location = _template_locations[previous_template_id];

					std::cerr << "  " << previous_template_location.first << "(" << previous_template_location.second << "): ";

					std::cerr << "note: previous definition: " << previous_template->get_name() << std::endl;

					indicate_error();
				}
				else
				{
					_template_locations.emplace_back(file_name, line_number);
				}
			}
			else
			{
				std::cerr << file_name << "(" << line_number << "): ";

				std::cerr << "error: invalid template definition: " << block.name << std::endl;

				indicate_error();
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

		size_t line_distance = 0;

		size_t line_offset = 0;

		for (auto instance : instances)
		{
			auto advance_line = [&]()
			{
				size_t offset = instance.data() - block.data();

				while (line_offset != offset)
				{
					if (block[line_offset] == '\n')
					{
						line_distance++;
					}

					line_offset++;
				}
			};

			auto report_error = [&](std::string_view label)
			{
				auto last_line_count = line_distance;

				auto last_origin_name = instance;

				size_t stack_top_index = _origin_stack.size() - 1;

				for (size_t stack_index = stack_top_index; stack_index <= stack_top_index; stack_index--)
				{
					std::pair<std::string_view, size_t> location;

					auto [location_index, line_count, origin_name] = _origin_stack[stack_index];

					if (location_index == SIZE_MAX)
					{
						location.first = _file_names.back();

						location.second = line_count;
					}
					else
					{
						location = _template_locations[location_index];
					}

					location.second += last_line_count;

					if (stack_index == stack_top_index)
					{
						std::cerr << location.first << "(" << location.second << "): ";

						std::cerr << "error: " << label << ": " << instance << std::endl;
					}
					else
					{
						std::cerr << "  " << location.first << "(" << location.second << "): ";

						std::cerr << "note: within template: " << last_origin_name << std::endl;
					}

					location.second -= last_line_count;

					if (location_index == SIZE_MAX)
					{
						std::cerr << "  " << location.first << "(" << location.second << "): ";

						std::cerr << "note: instantiated in " << origin_name << std::endl;
					}

					last_line_count = line_count;

					last_origin_name = origin_name;
				}

				indicate_error();
			};

			_split_buffer.clear();

			auto& args = _split_buffer;

			bool valid_template = template_split_instance(instance, std::back_inserter(args));

			if (valid_template == false)
			{
				advance_line();

				report_error("invalid template instantiation");

				continue;
			}

			auto result = _template_instances.emplace(instance);

			if (result.second)
			{
				advance_line();

				auto base = args[0];

				auto arg_start = args.begin() + 1;

				auto arg_end = args.end();

				auto template_exists = false;

				auto template_block = _template_registry.find_special(base, arg_start, arg_end, template_exists);

				if (template_block == nullptr)
				{
					if (template_exists)
					{
						report_error("specialization not found");
					}
					else
					{
						report_error("template not found");
					}

					continue;
				}

				std::string content = template_block->instantiate(instance);

				auto template_id = template_block->get_id();

				_origin_stack.emplace_back(template_id, line_distance, instance);

				emit_template(std::move(content), template_block->get_pattern(), arg_start, arg_end);

				_origin_stack.pop_back();
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

	bool ends_with_double_newline() const
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

	void indicate_error() const
	{
		if (compiler_arguments.stop_on_error)
		{
			throw 1;
		}
	}
};
