#pragma once

using Definition_Time = std::pair<size_t, size_t>;

struct Definition
{
	std::string par_list;

	std::string replacement;

	bool replace_content = false;
};

class Definition_Stack
{
private:

	std::vector<Definition> _definitions;

	std::vector<std::pair<Definition_Time, size_t>> _event_history;

public:

	template<typename Definition>
	void add_definition(Definition&& definition, size_t version, size_t& counter)
	{
		_definitions.push_back(std::forward<Definition>(definition));

		Definition_Time time(version, counter);

		auto depth = _definitions.size();

		_event_history.emplace_back(time, depth);

		counter++;
	}

	bool remove_definition(size_t version, size_t& counter)
	{
		if (_event_history.empty())
		{
			return false;
		}

		const auto& event = _event_history.back();

		auto depth = event.second;

		if (depth == 0)
		{
			return false;
		}

		Definition_Time time(version, counter);

		_event_history.emplace_back(time, depth - 1);

		counter++;

		return true;
	}

	const Definition* get_definition(Definition_Time& time) const
	{
		auto compare = [time](const auto& event)
		{
			return event.first < time;
		};

		auto it = std::partition_point(_event_history.begin(), _event_history.end(), compare);

		if (it == _event_history.begin())
		{
			return nullptr;
		}

		it--;

		auto depth = it->second;

		if (depth == 0)
		{
			return nullptr;
		}

		time = it->first;

		const auto& definition = _definitions[depth - 1];

		return &definition;
	}
};

template<typename Print_Error, typename Process_Content, typename Definition_Map>
class Preprocessor
{
private:

	Print_Error _print_error;

	Process_Content _process_content;

	const Definition_Map* _map;

	Definition_Time _time;

	size_t _line_offset;

public:

	Preprocessor(Print_Error print_error, Process_Content process_content, const Definition_Map* map, size_t version) : _print_error(print_error), _process_content(process_content)
	{
		_map = map;

		_time = { version, SIZE_MAX };

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

			auto definition_result = _map->find(base);

			if (definition_result == _map->end())
			{
				continue;
			}

			Definition_Time local_time = _time;

			const Definition* definition = definition_result->second.get_definition(local_time);

			if (definition == nullptr)
			{
				continue;
			}

			const auto& [par_list, replacement, replace_content] = *definition;

			auto par_it = par_list.begin();

			auto par_end = par_list.end();

			_line_offset = start_line_offset + line_iterator.get_line_number(it);

			auto arg_list_start = it;

			string_skip_word(it, block_end);

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

				replace_definitions<Trim_Start>(content);

				std::swap(local_time, _time);
			}

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

		if (replace_buffer.empty() == false)
		{
			replace_buffer.append(block_start, block_end);

			block = replace_buffer;
		}

		_line_offset = start_line_offset;

		return replace_buffer;
	}
};
