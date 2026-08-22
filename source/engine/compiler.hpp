#pragma once

class String_View_Streambuf : public std::streambuf
{
public:

	String_View_Streambuf(std::string_view view)
	{
		auto* data = (char*)view.data();

		setg(data, data, data + view.size());
	}
};

class String_View_Stream : public std::istream
{
	String_View_Streambuf buffer;

public:

	String_View_Stream(std::string_view view) : std::istream(&buffer), buffer(view)
	{
	}
};

template<typename It>
class Line_Iterator
{
private:

	It _line_it;

	size_t _line_number;

public:

	Line_Iterator(It it) : _line_it(it), _line_number(1)
	{
	}

	void set_line_number(It it, size_t number)
	{
		_line_it = it;

		_line_number = number;
	}

	size_t get_line_number(It it)
	{
		while (string_find(_line_it, it, '\n'))
		{
			_line_number++;

			_line_it++;
		}

		return _line_number;
	}
};

class Compiler
{
private:

	Template_Registry _template_registry;

	std::set<std::string> _template_instances;

	std::string _result;

	std::vector<std::pair<std::string_view, ptrdiff_t>> _split_buffer;

	std::set<std::string> _file_names;

	std::string_view _current_file_name;

	std::vector<std::pair<std::string_view, size_t>> _template_locations;

	std::vector<std::tuple<size_t, size_t, std::string_view>> _origin_stack;

	intptr_t _suppression_level = 0;

	bool _set_line_number = false;

public:

	Compiler()
	{
	}

	std::string compile(std::string_view path, std::string_view content)
	{
		auto path_result = _file_names.emplace(path);

		_current_file_name = *path_result.first;

		auto start = content.begin();

		auto end = content.end();

		auto it = start;

		Line_Iterator line_iterator(it);

		while (it != end)
		{
			if (*it == '#')
			{
				auto line_start = it;

				it++;

				string_skip_inline_space(it, end);

				auto directive_start = it;

				string_find(it, end, '\n');

				auto directive_name_end = directive_start;

				string_skip_word(directive_name_end, it);

				std::string_view directive_name(directive_start, directive_name_end);

				if (directive_name == "line")
				{
					std::string_view directive(directive_name_end, it);

					String_View_Stream directive_stream(directive);

					size_t line_number = 0;

					if (directive_stream >> line_number)
					{
						line_number--;

						line_iterator.set_line_number(it, line_number);
					}

					std::string file_name;

					if (directive_stream >> std::quoted(file_name))
					{
						std::replace(file_name.begin(), file_name.end(), '\\', '/');

						auto file_name_result = _file_names.insert(std::move(file_name));

						_current_file_name = *file_name_result.first;
					}

					if (compiler_arguments.insert_line_directives == false)
					{
						continue;
					}
				}

				if (directive_name == "pragma")
				{
					string_skip_space(directive_name_end, it);

					auto pragma_name_end = directive_name_end;

					string_skip_word(pragma_name_end, it);

					std::string_view pragma_name(directive_name_end, pragma_name_end);

					if (pragma_name == "DTC")
					{
						std::string_view directive(pragma_name_end, it);

						String_View_Stream directive_stream(directive);

						std::string command;

						directive_stream >> command;

						if (command == "disable")
						{
							_suppression_level++;

							continue;
						}

						if (command == "enable")
						{
							_suppression_level--;

							continue;
						}

						auto line_number = line_iterator.get_line_number(it);

						std::cerr << _current_file_name << "(" << line_number << "): ";

						std::cerr << "warning: unknown command: " << command << std::endl;

						continue;
					}
				}

				if (compiler_arguments.insert_line_directives && _set_line_number)
				{
					_set_line_number = false;

					auto line_number = line_iterator.get_line_number(it);

					emit_line_directive(_current_file_name, line_number);
				}

				std::string_view line(line_start, it);

				_result += line;

				continue;
			}

			if (_suppression_level > 0)
			{
				_result += *it;

				it++;

				continue;
			}

			Block block;

			auto block_end = parser_parse(it, end, block);

			if (block_end == it)
			{
				if (compiler_arguments.insert_line_directives && _set_line_number)
				{
					if (*it != '\n' && _result.back() == '\n')
					{
						_set_line_number = false;

						auto line_number = line_iterator.get_line_number(it);

						emit_line_directive(_current_file_name, line_number);
					}
				}

				_result += *it;

				it++;

				continue;
			}

			auto block_line_number = line_iterator.get_line_number(it);

			auto block_name_offset = block.name.data() - content.data();

			auto block_name_it = content.begin() + block_name_offset;

			_split_buffer.clear();

			auto& pars = _split_buffer;

			bool valid_template = template_split_special(block.name, pars);

			if (valid_template)
			{
				if (pars.size() == 1)
				{
					auto line_number = line_iterator.get_line_number(it);

					_origin_stack.emplace_back(SIZE_MAX, line_number, block.name);

					emit_block(block.content);

					_origin_stack.pop_back();

					it = block_end;

					continue;
				}

				auto par_start = pars.begin();

				auto par_end = pars.end();

				std::string pattern;

				for (auto par_it = par_start + 1; par_it != par_end; par_it++)
				{
					pattern += par_it->first;
				}

				size_t template_id = _template_locations.size();

				Template_Block template_block(template_id, pattern, block);

				for (auto& par : pars)
				{
					par.first = template_block.translate(block, par.first);
				}

				auto previous_template = _template_registry.add_template_special(par_start, par_end, std::move(template_block));

				if (previous_template != nullptr)
				{
					auto line_number = line_iterator.get_line_number(it);

					std::cerr << _current_file_name << "(" << line_number << "): ";

					std::cerr << "error: template already defined: " << block.name << std::endl;

					auto previous_template_id = previous_template->get_id();

					auto previous_template_location = _template_locations[previous_template_id];

					std::cerr << "  " << previous_template_location.first << "(" << previous_template_location.second << "): ";

					std::cerr << "note: previous definition: " << previous_template->get_name() << std::endl;

					indicate_error();
				}
				else
				{
					_template_locations.emplace_back(_current_file_name, block_line_number);
				}
			}
			else
			{
				auto line_number = line_iterator.get_line_number(it);

				std::cerr << _current_file_name << "(" << line_number << "): ";

				std::cerr << "error: invalid template definition: " << block.name << std::endl;

				indicate_error();
			}

			_set_line_number = true;

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
						location.first = _current_file_name;

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

						std::cerr << "note: originated from: " << origin_name << std::endl;
					}

					last_line_count = line_count;

					last_origin_name = origin_name;
				}

				indicate_error();
			};

			_split_buffer.clear();

			auto& args = _split_buffer;

			bool valid_template = template_split_instance(instance, args);

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

				std::string_view arg_sentinel(instance.data() + instance.size(), 0);

				args.emplace_back(arg_sentinel, 0);

				auto arg_start = args.begin();

				auto arg_end = args.end();

				auto template_exists = false;

				auto template_block = _template_registry.find_special(arg_start, arg_end - 1, template_exists);

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

				emit_template(std::move(content), template_block->get_pattern(), arg_start + 1, arg_end);

				_origin_stack.pop_back();
			}
		}

		if (compiler_arguments.insert_line_directives)
		{
			if (_set_line_number)
			{
				_set_line_number = false;

				emit_line_directive();
			}

			_result += block;
		}
		else
		{
			string_copy_block(block, std::back_inserter(_result));
		}
	}

	template<typename It>
	void emit_template(std::string content, std::string_view pattern, It arg_start, It arg_end)
	{
		_set_line_number = true;

		template_replace(content, pattern, arg_start, arg_end);

		emit_block(content);

		_set_line_number = true;
	}

	void emit_line_directive()
	{
		if (compiler_arguments.insert_line_directives)
		{
			const auto& [location_index, line_count, origin_name] = _origin_stack.back();

			if (location_index == SIZE_MAX)
			{
				emit_line_directive(_current_file_name, line_count);
			}
			else
			{
				auto location = _template_locations[location_index];

				emit_line_directive(location.first, location.second);
			}
		}
	}

	void emit_line_directive(std::string_view file_name, size_t line_number)
	{
		_result += "#line ";

		_result += std::to_string(line_number);

		_result += " \"";

		_result += file_name;

		_result += "\"\n";
	}

	void indicate_error() const
	{
		if (compiler_arguments.stop_on_error)
		{
			throw 1;
		}
	}
};
