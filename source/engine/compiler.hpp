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

	Line_Iterator(It it, size_t number) : _line_it(it), _line_number(number)
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

struct Location
{
	std::string_view file_name;

	size_t line_number;
};

struct Template_Location : Location
{
	size_t name_line_number;
};

struct Origin
{
	Location template_location;

	Location instance_location;

	std::string_view instance_name;
};

class Compiler
{
private:

	Template_Registry _template_registry;

	std::set<std::string> _template_instances;

	std::map<std::string, std::vector<std::pair<std::string, std::string>>, std::less<>> _definitions;

	std::string _result;

	std::vector<std::pair<std::string_view, ptrdiff_t>> _split_buffer;

	std::set<std::string> _file_names;

	std::string_view _current_file_name;

	std::vector<Template_Location> _template_locations;

	std::vector<Origin> _origin_stack;

	intptr_t _suppression_level = 0;

	bool _set_line_number = false;

	size_t _definition_line_offset = 0;

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

		Line_Iterator line_iterator(it, 1);

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

					auto pragma_it = directive_name_end;

					string_skip_word(pragma_it, it);

					std::string_view pragma_name(directive_name_end, pragma_it);

					if (pragma_name == "DTC")
					{
						string_skip_space(pragma_it, it);

						auto command_start = pragma_it;

						string_skip_word(pragma_it, it);

						std::string_view command(command_start, pragma_it);

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

						if (command == "push")
						{
							string_skip_space(pragma_it, it);

							auto pattern_start = pragma_it;

							string_skip_word(pragma_it, it);

							std::string_view pattern(pattern_start, pragma_it);

							bool valid_pattern = true;

							if (pattern.empty())
							{
								valid_pattern = false;
							}

							if (pattern.starts_with('$') || pattern.ends_with('$'))
							{
								valid_pattern = false;
							}

							if (pattern.find("$$") != SIZE_MAX)
							{
								valid_pattern = false;
							}

							if (valid_pattern)
							{
								auto base_length = pattern.find('$');

								if (base_length == SIZE_MAX)
								{
									base_length = pattern.size();
								}

								auto base = pattern.substr(0, base_length);

								auto par_list = pattern.substr(base_length);

								string_skip_space(pragma_it, it);

								std::string_view replacement(pragma_it, it);

								auto definition_result = _definitions.emplace(base, 0);

								definition_result.first->second.emplace_back(par_list, replacement);

								continue;
							}
						}

						if (command == "pop")
						{
							string_skip_space(pragma_it, it);

							auto pattern_start = pragma_it;

							string_skip_word(pragma_it, it);

							std::string_view pattern(pattern_start, pragma_it);
							
							string_skip_space(pragma_it, it);

							if (pattern.empty() == false && pragma_it == it)
							{
								auto definition_it = _definitions.find(pattern);

								if (definition_it == _definitions.end())
								{
									auto line_number = line_iterator.get_line_number(it);

									std::cerr << _current_file_name << "(" << line_number << "): ";

									std::cerr << "error: pattern not defined: " << pattern << std::endl;

									indicate_error();

									continue;
								}

								definition_it->second.pop_back();

								if (definition_it->second.empty())
								{
									_definitions.erase(definition_it);
								}

								continue;
							}
						}

						std::string_view directive(command_start, it);

						auto line_number = line_iterator.get_line_number(it);

						std::cerr << _current_file_name << "(" << line_number << "): ";

						std::cerr << "error: invalid pragma: " << directive << std::endl;

						indicate_error();

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

			auto block_name_line_number = line_iterator.get_line_number(block_name_it);

			_split_buffer.clear();

			auto& pars = _split_buffer;

			bool valid_template = template_split_special(block.name, pars);

			if (valid_template)
			{
				if (pars.size() == 1)
				{
					Origin origin = {};

					origin.template_location = { _current_file_name, block_line_number };

					origin.instance_location = { _current_file_name, block_name_line_number };

					origin.instance_name = block.name;

					_origin_stack.push_back(origin);

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
					std::cerr << _current_file_name << "(" << block_name_line_number << "): ";

					std::cerr << "error: template already defined: " << block.name << std::endl;

					auto previous_template_id = previous_template->get_id();

					auto previous_template_location = _template_locations[previous_template_id];

					std::cerr << "  " << previous_template_location.file_name << "(" << previous_template_location.name_line_number << "): ";

					std::cerr << "note: previous definition is here: " << previous_template->get_name() << std::endl;

					indicate_error();
				}
				else
				{
					Template_Location location = {};

					location.file_name = _current_file_name;

					location.line_number = block_line_number;

					location.name_line_number = block_name_line_number;

					_template_locations.push_back(location);
				}
			}
			else
			{
				std::cerr << _current_file_name << "(" << block_name_line_number << "): ";

				std::cerr << "error: invalid template definition: " << block.name << std::endl;

				indicate_error();
			}

			_set_line_number = true;

			it = block_end;
		}

		return std::move(_result);
	}

private:

	template<bool Trim_Start>
	void replace_definitions(std::string& string)
	{
		std::string_view block = string;

		std::string replace_buffer = replace_definitions<Trim_Start>(block);

		if (replace_buffer.empty() == false)
		{
			std::swap(string, replace_buffer);
		}
	}

	template<bool Trim_Start>
	std::string replace_definitions(std::string_view& block)
	{
		std::string replace_buffer;

		auto block_start = block.begin();

		auto block_end = block.end();

		auto it = block_start;

		Line_Iterator line_iterator(it, 0);

		auto start_line_offset = _definition_line_offset;

		while (string_find(it, block_end, '$'))
		{
			string_skip(it, block_end, '$');

			auto instance_start = it;

			string_skip_word_part(it, block_end);

			std::string_view base(instance_start, it);

			auto definition_result = _definitions.find(base);

			if (definition_result != _definitions.end())
			{
				_definition_line_offset = start_line_offset + line_iterator.get_line_number(it);

				auto arg_list_start = it;

				string_skip_word(it, block_end);

				std::string_view arg_list(arg_list_start, it);

				std::string arg_buffer = replace_definitions<false>(arg_list);

				auto arg_it = arg_list.begin();

				auto arg_end = arg_list.end();

				const auto& [par_list, replacement] = definition_result->second.back();

				auto par_it = par_list.begin();

				auto par_end = par_list.end();

				std::string content = replacement;

				while (par_it != par_end)
				{
					auto arg_start = arg_it;

					if (string_skip_template(arg_it, arg_end) == false)
					{
						auto location = get_current_block_location();

						location.line_number += _definition_line_offset;

						std::cerr << location.file_name << "(" << location.line_number << "): ";

						std::cerr << "error: invalid macro expansion: " << base << arg_list << std::endl;

						indicate_error();

						break;
					}

					auto par_start = par_it;

					par_it++;

					string_skip_word_part(par_it, par_end);

					std::string_view par(par_start, par_it);

					std::string_view arg(arg_start, arg_it);

					content = template_replace_macro(content, par, arg);
				}

				if (par_it != par_end)
				{
					continue;
				}

				content.append(arg_it, arg_end);

				replace_definitions<Trim_Start>(content);

				auto macro_start = instance_start - 1;

				if constexpr (Trim_Start)
				{
					char last_character = 0;

					if (last_character == 0 && macro_start != block_start)
					{
						last_character = macro_start[-1];
					}

					if (last_character == 0 && replace_buffer.empty() == false)
					{
						last_character = replace_buffer.back();
					}

					if (last_character == '$')
					{
						macro_start--;
					}

					if (string_is_word(last_character) == false)
					{
						auto content_start = content.begin();

						auto content_end = content.end();

						auto content_it = content_start;

						string_skip(content_it, content_end, '$');

						content.erase(content_start, content_it);
					}
				}

				replace_buffer.append(block_start, macro_start);

				replace_buffer.append(content);

				block_start = it;
			}
		}

		if (replace_buffer.empty() == false)
		{
			replace_buffer.append(block_start, block_end);

			block = replace_buffer;
		}

		_definition_line_offset = start_line_offset;

		return replace_buffer;
	}

	void emit_block(std::string_view block)
	{
		_definition_line_offset = 0;

		auto replace_buffer = replace_definitions<true>(block);

		auto start = block.begin();

		auto end = block.end();

		std::vector<std::string_view> instances;

		template_get_instances(start, end, std::back_inserter(instances));

		Line_Iterator line_iterator(start, 0);

		for (auto instance : instances)
		{
			auto get_instance_line_offset = [&]()
			{
				auto instance_offset = instance.data() - block.data();

				auto instance_start = start + instance_offset;

				return line_iterator.get_line_number(instance_start);
			};

			auto report_error = [&](std::string_view label)
			{
				const auto& current_origin = _origin_stack.back();

				auto current_location = current_origin.template_location;

				auto instance_line_offset = get_instance_line_offset();

				current_location.line_number += instance_line_offset;

				std::cerr << current_location.file_name << "(" << current_location.line_number << "): ";

				std::cerr << "error: " << label << ": " << instance << std::endl;

				size_t stack_top_index = _origin_stack.size() - 1;

				for (size_t stack_index = stack_top_index; stack_index <= stack_top_index; stack_index--)
				{
					const auto& origin = _origin_stack[stack_index];

					std::cerr << "  " << origin.instance_location.file_name << "(" << origin.instance_location.line_number << "): ";

					if (stack_index == 0)
					{
						std::cerr << "note: required from here: " << origin.instance_name << std::endl;
					}
					else
					{
						std::cerr << "note: instantiated from here: " << origin.instance_name << std::endl;
					}
				}

				indicate_error();
			};

			_split_buffer.clear();

			auto& args = _split_buffer;

			bool valid_template = template_split_instance(instance, args);

			if (valid_template == false)
			{
				report_error("invalid template instantiation");

				continue;
			}

			auto result = _template_instances.emplace(instance);

			if (result.second)
			{
				std::string_view end_arg(instance.end(), instance.end());

				args.emplace_back(end_arg, 0);

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

				auto instance_line_offset = get_instance_line_offset();

				const auto& current_origin = _origin_stack.back();

				Origin origin = {};

				origin.template_location = _template_locations[template_id];

				origin.instance_location = current_origin.template_location;

				origin.instance_location.line_number += instance_line_offset;

				origin.instance_name = instance;

				_origin_stack.push_back(origin);

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
			auto location = get_current_block_location();

			emit_line_directive(location.file_name, location.line_number);
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

	Location get_current_block_location() const
	{
		const auto& origin = _origin_stack.back();

		return origin.template_location;
	}

	void indicate_error() const
	{
		if (compiler_arguments.stop_on_error)
		{
			throw 1;
		}
	}
};
