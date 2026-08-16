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

bool string_continues_with(std::string_view::const_iterator it, std::string_view::const_iterator end, std::string_view pattern)
{
	return string_continues_with(it, end, pattern.begin(), pattern.end());
}

bool string_continues_with(std::string_view::const_reverse_iterator it, std::string_view::const_reverse_iterator end, std::string_view pattern)
{
	return string_continues_with(it, end, pattern.rbegin(), pattern.rend());
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

template<typename It>
void string_skip_word(It& it, It end)
{
	while (it != end && string_is_word(*it))
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
