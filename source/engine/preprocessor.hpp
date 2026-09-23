#pragma once

struct Macro_Definition
{
	std::string par_list;

	std::string replacement;

	bool replace_content = false;
};

struct Quote_Definition
{
};

using Macro_Definition_Map = std::map<std::string, Definition_Stack<Macro_Definition>, std::less<>>;

using Quote_Definition_Map = std::map<std::string, Definition_Stack<Quote_Definition>, std::less<>>;

struct Definition_State
{
	const Macro_Definition_Map* macro_definition_map;

	const Quote_Definition_Map* quote_definition_map;

	size_t definition_version;
};

template<typename Print_Error, typename Process_Content>
class Preprocessor
{
private:

	Print_Error _print_error;

	Process_Content _process_content;

	const Macro_Definition_Map* _macro_definition_map;

	const Quote_Definition_Map* _quote_definition_map;

	Definition_Time _time;

	size_t _line_offset;

public:

	Preprocessor(Print_Error print_error, Process_Content process_content, Definition_State state) : _print_error(print_error), _process_content(process_content)
	{
		_macro_definition_map = state.macro_definition_map;

		_quote_definition_map = state.quote_definition_map;

		_time = { state.definition_version, SIZE_MAX };

		_line_offset = 0;
	}

	void preprocess(std::string& string)
	{
		replace_definitions<true>(string);
	}

	std::string preprocess(std::string_view& block)
	{
		return replace_definitions<true>(block);
	}

	template<bool Trim_Start>
	void replace_definitions(std::string& string)
	{
		std::string_view block = string;

		auto replace_buffer = replace_definitions<Trim_Start>(block);

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

		auto start_line_offset = _line_offset;

		while (string_find(it, block_end, '$'))
		{
			string_skip(it, block_end, '$');

			auto instance_start = it;

			string_skip_word_part(it, block_end);

			std::string_view base(instance_start, it);

			auto definition_it = _macro_definition_map->find(base);

			if (definition_it == _macro_definition_map->end())
			{
				continue;
			}

			Definition_Time local_time = _time;

			auto definition = definition_it->second.get_definition(local_time);

			if (definition == nullptr)
			{
				continue;
			}

			auto arg_list_start = it;

			string_skip_word(it, block_end);

			auto rit = std::reverse_iterator(instance_start);

			auto rend = std::reverse_iterator(block_start);

			string_skip_word(rit, rend);

			std::string_view word(rit.base(), it);

			if (is_word_quoted(word, _time))
			{
				continue;
			}

			const auto& [par_list, replacement, replace_content] = *definition;

			auto par_it = par_list.begin();

			auto par_end = par_list.end();

			_line_offset = start_line_offset + line_iterator.get_line_number(it);

			std::string_view arg_list(arg_list_start, it);

			std::string arg_buffer = replace_definitions<false>(arg_list);

			auto arg_it = arg_list.begin();

			auto arg_end = arg_list.end();

			std::string content(replacement);

			while (par_it != par_end)
			{
				auto arg_start = arg_it;

				if (string_skip_template(arg_it, arg_end) == false)
				{
					_print_error(_line_offset, base, arg_list);

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

			if constexpr (std::is_same_v<Process_Content, std::nullptr_t> == false)
			{
				_process_content(content);
			}

			if (replace_content)
			{
				std::swap(local_time, _time);

				replace_definitions<false>(content);

				std::swap(local_time, _time);

				replace_definitions<false>(content);
			}

			auto macro_start = instance_start - 1;

			if constexpr (Trim_Start)
			{
				bool is_word_start = true;

				if (macro_start != block_start)
				{
					auto last_character_it = macro_start - 1;

					auto last_character = *last_character_it;

					if (string_is_word(last_character))
					{
						is_word_start = false;
					}
				}

				if (is_word_start)
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

		if (replace_buffer.empty() == false)
		{
			replace_buffer.append(block_start, block_end);

			block = replace_buffer;
		}

		_line_offset = start_line_offset;

		return replace_buffer;
	}

private:

	bool is_word_quoted(std::string_view word, Definition_Time time) const
	{
		auto definition_it = _quote_definition_map->find(word);

		if (definition_it != _quote_definition_map->end())
		{
			auto definition = definition_it->second.get_definition(time);

			if (definition != nullptr)
			{
				return true;
			}
		}

		return false;
	}
};
