#pragma once

bool string_is_inline_space(char c)
{
	return c == ' ' || c == '\t';
}

bool string_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n';
}

bool string_is_word(char c)
{
	return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '_' || c == '$';
}

bool string_is_word_part(char c)
{
	return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '_';
}

bool string_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

template<typename It>
bool string_continues_with(It it, It end, It pattern_it, It pattern_end)
{
	while (it != end && pattern_it != pattern_end)
	{
		if (*it != *pattern_it)
		{
			return false;
		}

		it++;

		pattern_it++;
	}

	return pattern_it == pattern_end;
}

template<typename It>
bool string_continues_with(It it, It end, std::string_view pattern)
{
	return string_continues_with(it, end, pattern.begin(), pattern.end());
}

template<typename It>
bool string_skip_string(It& it, It end)
{
	if (it == end)
	{
		return false;
	}

	if (*it != '\"' && *it != '\'')
	{
		return false;
	}

	auto quote = *it;

	bool escaped = false;

	while (true)
	{
		it++;

		if (it == end)
		{
			return true;
		}

		if (escaped)
		{
			escaped = false;

			continue;
		}

		if (*it == '\\')
		{
			escaped = true;

			continue;
		}

		if (*it == quote)
		{
			it++;

			return true;
		}
	}
}

template<typename It>
bool string_find(It& it, It end, char c)
{
	while (it != end && *it != c)
	{
		it++;
	}

	return it != end;
}

template<bool Skip_Strings = false, typename It>
bool string_find(It& it, It end, const char* s)
{
	while (it != end)
	{
		for (auto p = s; *p; p++)
		{
			if (*it == *p)
			{
				return true;
			}
		}

		if constexpr (Skip_Strings)
		{
			if (string_skip_string(it, end))
			{
				continue;
			}
		}

		it++;
	}

	return it != end;
}

template<typename It>
void string_skip(It& it, It end, char c)
{
	while (it != end && *it == c)
	{
		it++;
	}
}

template<typename It, typename Predicate>
void string_skip(It& it, It end, Predicate predicate)
{
	while (it != end && predicate(*it))
	{
		it++;
	}
}

template<typename It>
void string_skip_word(It& it, It end)
{
	while (it != end && string_is_word(*it))
	{
		it++;
	}
}

template<typename It>
void string_skip_word_part(It& it, It end)
{
	while (it != end && string_is_word_part(*it))
	{
		it++;
	}
}

template<typename It>
void string_skip_space(It& it, It end)
{
	while (it != end && string_is_space(*it))
	{
		it++;
	}
}

template<typename It>
void string_skip_inline_space(It& it, It end)
{
	while (it != end && string_is_inline_space(*it))
	{
		it++;
	}
}

template<bool Skip_Strings = false, typename It>
void string_skip_block(It& it, It end, char c1, char c2)
{
	if (it == end || *it != c1 && *it != c2)
	{
		return;
	}

	int level = 0;

	while (it != end)
	{
		if (*it == c1)
		{
			level++;
		}

		if (*it == c2)
		{
			level--;
		}

		if constexpr (Skip_Strings)
		{
			if (string_skip_string(it, end))
			{
				continue;
			}
		}

		it++;

		if (level == 0)
		{
			break;
		}
	}
}

template<bool Skip_Strings = false, typename It>
void string_skip_block(It& it, It end)
{
	string_skip_block<Skip_Strings>(it, end, '{', '}');
}

template<typename It>
void string_skip_par_list(It& it, It end)
{
	string_skip_block(it, end, '(', ')');
}

template<typename It>
void string_skip_array(It& it, It end)
{
	string_skip_block(it, end, '[', ']');
}

template<typename It>
void string_skip_statement(It& it, It end)
{
	while (it != end && *it != ';')
	{
		it++;
	}
}

template<typename It>
bool string_skip_template(It& it, It end)
{
	ptrdiff_t level = 1;

	while (it != end && *it == '$')
	{
		It start = it;

		string_skip(it, end, '$');

		auto sub_count = it - start;

		level += sub_count - 2;

		string_skip_word_part(it, end);

		if (level == 0)
		{
			return true;
		}
	}

	return false;
}

template<typename It, typename Inserter>
void string_copy_block(It start, It end, Inserter inserter)
{
	auto it = start;

	while (it != end)
	{
		auto start_it = it;

		string_skip_inline_space(it, end);

		bool is_line_directive = false;

		if (it != end && *it == '#')
		{
			it++;

			string_skip_inline_space(it, end);

			if (string_continues_with(it, end, "line"))
			{
				is_line_directive = true;
			}
		}

		bool found_newline = string_find(it, end, '\n');

		if (is_line_directive == false)
		{
			std::copy(start_it, it, inserter);
		}

		if (found_newline)
		{
			*inserter = *it;

			it++;
		}
	}
}

template<typename Inserter>
void string_copy_block(std::string_view string, Inserter inserter)
{
	string_copy_block(string.begin(), string.end(), inserter);
}
