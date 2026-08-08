#pragma once

template<typename It>
bool parser_continues_with_struct(It it, It end)
{
	bool result = false;

	result |= string_continues_with(it, end, "struct");

	result |= string_continues_with(it, end, "union");

	return result;
}

template<typename It>
It parser_parse_struct(It start, It end, Block& block)
{
	It it = start;

	bool is_struct = parser_continues_with_struct(it, end);

	if (is_struct == false)
	{
		return start;
	}

	auto block_start = it;

	string_skip_word(it, end);

	string_skip_space(it, end);

	auto name_start = it;

	string_skip_word(it, end);

	auto name_end = it;

	bool is_named_struct = name_start != name_end;

	if (is_named_struct == false)
	{
		return start;
	}

	string_skip_space(it, end);

	bool is_struct_definition = string_continues_with(it, end, "{");

	if (is_struct_definition == false)
	{
		return start;
	}

	string_skip_block(it, end);

	string_skip_space(it, end);

	bool is_type_expression = string_continues_with(it, end, ";");

	if (is_type_expression == false)
	{
		return start;
	}

	it++;

	auto block_end = it;

	block.name = { name_start, name_end };

	block.content = { block_start, block_end };

	return block_end;
}

template<typename It>
It parser_parse_declarator(It it, It end, It block_start, It block_end, Block& block)
{
	auto rend = std::reverse_iterator(block_start);

	while (it != end)
	{
		bool has_first_par_list = string_find(it, end, '(');

		auto par_it = it;

		string_skip_par_list(par_it, end);

		auto par_end = par_it;

		bool has_second_par_list = string_find(par_it, end, '(');

		if (has_second_par_list)
		{
			it++;

			end = std::prev(par_end);

			continue;
		}

		if (has_first_par_list)
		{
			auto rit = std::reverse_iterator(par_it);

			string_skip_space(rit, rend);

			if (*rit == ']')
			{
				it++;

				end = std::prev(par_end);

				continue;
			}
		}

		auto rit = std::reverse_iterator(it);

		while (rit != rend)
		{
			string_skip_space(rit, rend);

			if (*rit == ']')
			{
				string_skip_array(rit, rend);

				continue;
			}

			auto name_end = rit.base();

			string_skip_word(rit, rend);

			auto name_start = rit.base();

			bool found_name = name_start != name_end;

			if (found_name == false)
			{
				return block_start;
			}

			block.name = { name_start, name_end };

			block.content = { block_start, block_end };

			return block_end;
		}
	}

	return block_start;
}

template<typename It>
It parser_parse_typedef(It start, It end, Block& block)
{
	auto it = start;

	auto rend = std::reverse_iterator(start);

	bool is_typedef = string_continues_with(it, end, "typedef");

	if (is_typedef == false)
	{
		return start;
	}

	auto block_start = it;

	string_skip_word(it, end);

	string_skip_space(it, end);

	bool is_struct_typedef = parser_continues_with_struct(it, end);

	if (is_struct_typedef)
	{
		string_skip_word(it, end);

		string_skip_space(it, end);

		string_skip_word(it, end);

		string_skip_space(it, end);

		string_skip_block(it, end);
	}

	string_skip_space(it, end);

	auto block_end = it;

	string_skip_statement(block_end, end);

	if (block_end == end)
	{
		return start;
	}

	auto declaration_end = block_end;

	block_end++;

	return parser_parse_declarator(it, declaration_end, block_start, block_end, block);
}

template<typename It>
It parser_parse_function(It start, It end, Block& block)
{
	auto it = start;

	auto rend = std::reverse_iterator(start);

	if (it == end)
	{
		return start;
	}

	bool is_word = string_is_word(*it);

	if (is_word == false)
	{
		return start;
	}

	auto block_start = it;

	auto declaration_end = it;

	auto found_one = string_find(declaration_end, end, "{;=");

	if (found_one == false)
	{
		return start;
	}

	if (*declaration_end != '{')
	{
		return start;
	}

	auto opening_count = std::count(it, declaration_end, '(');

	auto closing_count = std::count(it, declaration_end, ')');

	if (opening_count == 0 || opening_count != closing_count)
	{
		return start;
	}

	auto block_end = declaration_end;

	string_skip_block(block_end, end);

	if (block_end == end)
	{
		return start;
	}

	return parser_parse_declarator(it, declaration_end, block_start, block_end, block);
}

template<typename It>
It parser_parse_block(It start, It end, Block& block)
{
	It it = start;

	if (it == end)
	{
		return start;
	}

	bool is_word = string_is_word(*it);

	if (is_word == false)
	{
		return start;
	}

	while (true)
	{
		auto start_it = it;

		auto found_one = string_find(it, end, "{;=");

		if (found_one == false)
		{
			return start;
		}

		if (*it == '{')
		{
			string_skip_block(it, end);

			continue;
		}

		if (*it == ';')
		{
			auto block_end = it;

			block_end++;

			return parser_parse_declarator(start, it, start, block_end, block);
		}

		if (*it == '=')
		{
			auto block_end = it;

			if (string_find(block_end, end, ';'))
			{
				block_end++;
			}

			return parser_parse_declarator(start, it, start, block_end, block);
		}
	}
}

template<typename It>
It parser_parse(It start, It end, Block& block)
{
	auto result = start;

	if (result == start)
	{
		result = parser_parse_struct(start, end, block);
	}

	if (result == start)
	{
		result = parser_parse_typedef(start, end, block);
	}

	if (result == start)
	{
		result = parser_parse_function(start, end, block);
	}

	if (result == start)
	{
		result = parser_parse_block(start, end, block);
	}

	return result;
}
