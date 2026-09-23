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

struct Compiler_Arguments
{
	bool stop_on_error;

	bool insert_line_directives;

	bool conformance_mode;

	bool expand_macros_in_definitions;
};

class Compiler
{
private:

	Compiler_Arguments _arguments;

	Template_Registry _template_registry;

	std::set<std::string> _template_instances;

	std::string _result;

	std::vector<std::pair<std::string_view, ptrdiff_t>> _split_buffer;

	std::set<std::string> _file_names;

	std::string_view _current_file_name;

	std::vector<Template_Location> _template_locations;

	std::vector<Origin> _origin_stack;

	Macro_Definition_Map _macro_definition_map;

	Quote_Definition_Map _quote_definition_map;

	size_t _definition_counter = 0;

	intptr_t _suppression_level = 0;

	bool _set_line_number = false;

public:

	Compiler(Compiler_Arguments arguments) : _arguments(arguments)
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

		Block block;

		std::string renamed_block_buffer;

		while (it != end)
		{
			if (*it == '#')
			{
				parse_directive(it, end, line_iterator);

				continue;
			}

			bool process_block = true;

			if (_suppression_level > 0)
			{
				process_block = false;
			}

			if (process_block)
			{
				auto block_end = parser_parse(it, end, block);

				if (block_end == it)
				{
					process_block = false;
				}
			}

			if (process_block == false)
			{
				if (_arguments.insert_line_directives && _set_line_number)
				{
					if (*it != '\n' && _result.ends_with('\n'))
					{
						_set_line_number = false;

						auto line_number = line_iterator.get_line_number(it);

						emit_line_directive(_current_file_name, line_number);
					}
				}

				auto start_it = it;

				if (string_skip_string(it, end))
				{
					_result.append(start_it, it);

					continue;
				}

				_result += *it;

				it++;

				continue;
			}

			auto block_line_number = line_iterator.get_line_number(it);

			auto block_name_offset = block.name.data() - content.data();

			auto block_name_it = content.begin() + block_name_offset;

			auto block_name_line_number = line_iterator.get_line_number(block_name_it);

			auto block_end = it + block.content.size();

			if (_arguments.expand_macros_in_definitions)
			{
				Origin origin = {};

				origin.template_location = { _current_file_name, block_name_line_number };

				_origin_stack.push_back(origin);

				auto print_error = std::bind_front(&Compiler::print_definition_error, this);

				Definition_State definition_state = { &_macro_definition_map, &_quote_definition_map, SIZE_MAX };

				Preprocessor preprocessor(print_error, nullptr, definition_state);

				std::string_view block_name = block.name;

				auto preprocess_buffer = preprocessor.preprocess(block_name);

				if (preprocess_buffer.empty() == false)
				{
					size_t name_offset = block.name.data() - block.content.data();

					renamed_block_buffer = block.content;

					renamed_block_buffer.replace(name_offset, block.name.size(), block_name);

					block.content = renamed_block_buffer;

					block.name = block.content.substr(name_offset, block_name.size());
				}

				_origin_stack.pop_back();
			}

			_split_buffer.clear();

			auto& pars = _split_buffer;

			bool valid_template = template_split_special(block.name, pars);

			if (valid_template)
			{
				bool is_template = true;

				if (pars.size() == 1)
				{
					is_template = false;
				}

				if (is_template)
				{
					auto definition_it = _quote_definition_map.find(block.name);

					if (definition_it != _quote_definition_map.end())
					{
						Definition_Time time(SIZE_MAX, SIZE_MAX);

						auto definition = definition_it->second.get_definition(time);

						if (definition != nullptr)
						{
							is_template = false;
						}
					}
				}

				if (is_template == false)
				{
					Origin origin = {};

					origin.template_location = { _current_file_name, block_line_number };

					origin.instance_location = { _current_file_name, block_name_line_number };

					origin.instance_name = block.name;

					_origin_stack.push_back(origin);

					auto print_error = std::bind_front(&Compiler::print_definition_error, this);

					Definition_State definition_state = { &_macro_definition_map, &_quote_definition_map, SIZE_MAX };

					Preprocessor preprocessor(print_error, nullptr, definition_state);

					std::string_view block_content = block.content;

					auto preprocess_buffer = preprocessor.preprocess(block_content);

					emit_block(block_content, SIZE_MAX);

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

		if (_arguments.insert_line_directives == false)
		{
			string_remove_line_directives(_result);
		}

		if (_arguments.conformance_mode)
		{
			string_remove_dollar_signs(_result);
		}

		return std::move(_result);
	}

private:

	template<typename It>
	void parse_directive(It& it, It end, Line_Iterator<It>& line_iterator)
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

			if (_arguments.insert_line_directives == false)
			{
				return;
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

				if (parse_pragma(pragma_it, it, line_iterator))
				{
					return;
				}

				std::string_view pragma(pragma_it, it);

				auto line_number = line_iterator.get_line_number(it);

				std::cerr << _current_file_name << "(" << line_number << "): ";

				std::cerr << "error: invalid pragma: " << pragma << std::endl;

				indicate_error();

				return;
			}
		}

		if (_arguments.insert_line_directives && _set_line_number)
		{
			_set_line_number = false;

			auto line_number = line_iterator.get_line_number(it);

			emit_line_directive(_current_file_name, line_number);
		}

		std::string_view line(line_start, it);

		_result += line;
	}

	template<typename It>
	bool parse_pragma(It it, It end, Line_Iterator<It>& line_iterator)
	{
		auto command_start = it;

		string_skip_word(it, end);

		std::string_view command(command_start, it);

		if (command == "quote")
		{
			string_skip_space(it, end);

			auto pattern_start = it;

			string_skip_word(it, end);

			std::string_view pattern(pattern_start, it);

			string_skip_space(it, end);

			if (it == end)
			{
				if (pattern.empty())
				{
					_suppression_level++;
				}
				else
				{
					auto definition_it = _quote_definition_map.find(pattern);

					if (definition_it == _quote_definition_map.end())
					{
						Definition_Stack<Quote_Definition> stack;

						auto result = _quote_definition_map.emplace(pattern, std::move(stack));

						definition_it = result.first;
					}

					Quote_Definition definition;

					size_t version = _template_locations.size();

					definition_it->second.add_definition(std::move(definition), version, _definition_counter);
				}

				return true;
			}
		}

		if (command == "unquote")
		{
			string_skip_space(it, end);

			auto pattern_start = it;

			string_skip_word(it, end);

			std::string_view pattern(pattern_start, it);

			string_skip_space(it, end);

			if (it == end)
			{
				if (pattern.empty() && _suppression_level > 0)
				{
					_suppression_level--;

					return true;
				}

				auto definition_it = _quote_definition_map.find(pattern);

				if (definition_it != _quote_definition_map.end())
				{
					size_t version = _template_locations.size();

					if (definition_it->second.remove_definition(version, _definition_counter))
					{
						return true;
					}
				}
			}
		}

		if (command == "push")
		{
			string_skip_space(it, end);

			auto pattern_start = it;

			string_skip_word(it, end);

			std::string_view pattern(pattern_start, it);

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

				string_skip_space(it, end);

				std::string_view replacement(it, end);

				auto definition_it = _macro_definition_map.find(base);

				if (definition_it == _macro_definition_map.end())
				{
					Definition_Stack<Macro_Definition> stack;

					auto result = _macro_definition_map.emplace(base, std::move(stack));

					definition_it = result.first;
				}

				Macro_Definition definition;

				definition.par_list = par_list;

				definition.replacement = replacement;

				if (_suppression_level == 0)
				{
					definition.replace_content = true;
				}

				size_t version = _template_locations.size();

				definition_it->second.add_definition(std::move(definition), version, _definition_counter);

				return true;
			}
		}

		if (command == "pop")
		{
			string_skip_space(it, end);

			auto pattern_start = it;

			string_skip_word(it, end);

			std::string_view pattern(pattern_start, it);

			string_skip_space(it, end);

			if (pattern.empty() == false && it == end)
			{
				auto definition_it = _macro_definition_map.find(pattern);

				if (definition_it != _macro_definition_map.end())
				{
					size_t version = _template_locations.size();

					if (definition_it->second.remove_definition(version, _definition_counter))
					{
						return true;
					}
				}

				auto line_number = line_iterator.get_line_number(end);

				std::cerr << _current_file_name << "(" << line_number << "): ";

				std::cerr << "error: macro not defined: " << pattern << std::endl;

				indicate_error();

				return true;
			}
		}

		if (command == "instantiate")
		{
			string_skip_space(it, end);

			auto is_pattern = [](char c)
			{
				return string_is_word(c) || c == '*';
			};

			auto pattern_start = it;

			string_skip(it, end, is_pattern);

			std::string_view pattern(pattern_start, it);

			string_skip_space(it, end);

			bool valid_pattern = true;

			if (pattern.empty())
			{
				valid_pattern = false;
			}

			if (pattern.find("**") != SIZE_MAX)
			{
				valid_pattern = false;
			}

			if (valid_pattern && it == end)
			{
				auto print_error = std::bind_front(&Compiler::print_definition_error, this);

				Definition_State definition_state = { &_macro_definition_map, &_quote_definition_map, SIZE_MAX };

				Preprocessor preprocessor(print_error, nullptr, definition_state);

				auto pattern_buffer = preprocessor.preprocess(pattern);

				auto line_number = line_iterator.get_line_number(end);

				Origin origin = {};

				origin.template_location = { _current_file_name, line_number };

				origin.instance_location = { _current_file_name, line_number };

				origin.instance_name = "pragma";

				_origin_stack.push_back(origin);

				auto get_instance_line_offset = [&]
				{
					return 0;
				};

				if (pattern.find('*') != SIZE_MAX)
				{
					_split_buffer.clear();

					auto& args = _split_buffer;

					bool valid_template = template_split_pattern(pattern, args);

					if (valid_template == false)
					{
						std::cerr << _current_file_name << "(" << line_number << "): ";

						std::cerr << "error: invalid template pattern: " << pattern << std::endl;

						indicate_error();

						_origin_stack.pop_back();

						return true;
					}

					auto arg_start = args.begin();

					auto arg_end = args.end();

					std::vector<std::string> instances;

					_template_registry.find_specials(arg_start, arg_end, std::back_inserter(instances));

					if (instances.empty())
					{
						std::cerr << _current_file_name << "(" << line_number << "): ";

						std::cerr << "error: pattern not found: " << pattern << std::endl;

						indicate_error();

						_origin_stack.pop_back();

						return true;
					}

					for (const auto& instance : instances)
					{
						instantiate_template(instance, get_instance_line_offset);
					}
				}
				else
				{
					instantiate_template(pattern, get_instance_line_offset);
				}

				_origin_stack.pop_back();

				return true;
			}
		}

		return false;
	}

	template<typename Get_Instance_Line_Offset>
	void instantiate_template(std::string_view instance, Get_Instance_Line_Offset get_instance_line_offset)
	{
		auto result = _template_instances.emplace(instance);

		if (result.second == false)
		{
			return;
		}

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
					std::cerr << "note: instantiation origin: " << origin.instance_name << std::endl;
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
			report_error("invalid template instance");

			return;
		}

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

			_template_instances.erase(result.first);

			return;
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

		emit_template(std::move(content), template_block->get_pattern(), arg_start + 1, arg_end, template_id);

		_origin_stack.pop_back();
	}

	void emit_block(std::string_view block, size_t version)
	{
		auto start = block.begin();

		auto end = block.end();

		std::vector<std::string_view> instances;

		template_get_instances(start, end, std::back_inserter(instances));

		Line_Iterator line_iterator(start, 0);

		for (auto instance : instances)
		{
			auto get_instance_line_offset = [&]
			{
				auto instance_offset = instance.data() - block.data();

				auto instance_start = start + instance_offset;

				return line_iterator.get_line_number(instance_start);
			};

			auto definition_it = _quote_definition_map.find(instance);

			if (definition_it != _quote_definition_map.end())
			{
				Definition_Time time(version, SIZE_MAX);

				auto definition = definition_it->second.get_definition(time);

				if (definition != nullptr)
				{
					continue;
				}
			}

			instantiate_template(instance, get_instance_line_offset);
		}

		if (_arguments.insert_line_directives)
		{
			if (_set_line_number)
			{
				_set_line_number = false;

				emit_line_directive();
			}
		}

		_result += block;
	}

	template<typename It>
	void emit_template(std::string content, std::string_view pattern, It arg_start, It arg_end, size_t template_id)
	{
		_set_line_number = true;

		auto process_content = [&](std::string& template_content)
		{
			template_replace(template_content, pattern, arg_start, arg_end);
		};

		process_content(content);

		auto print_error = std::bind_front(&Compiler::print_definition_error, this);

		Definition_State definition_state = { &_macro_definition_map, &_quote_definition_map, template_id };

		Preprocessor preprocessor(print_error, process_content, definition_state);

		preprocessor.preprocess(content);

		emit_block(content, template_id);

		_set_line_number = true;
	}

	void emit_line_directive()
	{
		if (_arguments.insert_line_directives)
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

	void print_definition_error(size_t line_offset, std::string_view base, std::string_view arg_list) const
	{
		auto location = get_current_block_location();

		location.line_number += line_offset;

		std::cerr << location.file_name << "(" << location.line_number << "): ";

		std::cerr << "error: invalid macro expansion: " << base << arg_list << std::endl;

		indicate_error();
	}

	Location get_current_block_location() const
	{
		const auto& origin = _origin_stack.back();

		return origin.template_location;
	}

	void indicate_error() const
	{
		if (_arguments.stop_on_error)
		{
			throw 1;
		}
	}
};
