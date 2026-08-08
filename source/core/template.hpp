#pragma once

template<typename It>
It template_get_start(It start, It end)
{
	return std::find(start, end, '$');
}

std::string_view::const_iterator template_get_start(std::string_view string)
{
	return template_get_start(string.begin(), string.end());
}

template<typename It>
std::string template_replace(It source_start, It source_end, It pattern_start, It pattern_end, It rep_start, It rep_end)
{
	std::string result;

	It it = source_start;

	while (it != source_end)
	{
		It source_it = it;

		It pattern_it = pattern_start;

		bool match_found = false;

		while (*source_it == *pattern_it)
		{
			source_it++;

			pattern_it++;

			if (source_it == source_end)
			{
				match_found = true;

				break;
			}

			if (pattern_it == pattern_end)
			{
				if (string_is_word(*source_it) == false)
				{
					match_found = true;
				}

				if (*source_it == '$')
				{
					match_found = true;
				}

				break;
			}
		}

		if (match_found)
		{
			It rep_it = rep_start;

			if (result.empty() || string_is_word(result.back()) == false)
			{
				string_skip(rep_it, rep_end, '$');
			}

			while (rep_it != rep_end)
			{
				result += *rep_it;

				rep_it++;
			}

			it = source_it;
		}
		else
		{
			result += *it;

			it++;
		}
	}

	return result;
}

std::string template_replace(std::string_view source, std::string_view pattern, std::string_view rep)
{
	return template_replace(source.begin(), source.end(), pattern.begin(), pattern.end(), rep.begin(), rep.end());
}

template<typename It, typename Inserter>
bool template_split_instance(It start, It end, Inserter inserter)
{
	ptrdiff_t level = 0;

	auto it = start;

	while (it != end)
	{
		string_find(it, end, '$');

		if (level == 0)
		{
			std::string_view arg(start, it);

			start = it;

			*inserter = arg;

			level++;
		}

		auto dollar_start = it;

		string_skip(it, end, '$');

		auto dollar_count = it - dollar_start;

		level += dollar_count - 2;
	}

	if (level != -1)
	{
		return false;
	}

	return true;
}

template<typename Inserter>
bool template_split_instance(std::string_view string, Inserter inserter)
{
	return template_split_instance(string.begin(), string.end(), inserter);
}

template<typename It, typename Inserter>
void template_get_instances(It start, It end, Inserter inserter)
{
	auto it = start;

	while (it != end)
	{
		if (string_is_word(*it))
		{
			auto word_start = it;

			auto word_end = it;

			string_skip_word(word_end, end);

			auto template_start = std::find(it, word_end, '$');

			if (template_start != word_end)
			{
				std::string_view instance(word_start, word_end);

				*inserter = instance;
			}

			it = word_end;

			continue;
		}

		it++;
	}
}

template<typename Inserter>
void template_get_instances(std::string_view string, Inserter inserter)
{
	template_get_instances(string.begin(), string.end(), inserter);
}

template<typename It>
void template_get_key(It base_start, It base_end, size_t arg_count, std::ostream& key)
{
	std::string_view base(base_start, base_end);

	key << base;

	key << '$';

	key << arg_count;
}

void template_get_key(std::string_view string, size_t arg_count, std::ostream& key)
{
	return template_get_key(string.begin(), string.end(), arg_count, key);
}

template<typename It>
bool template_get_key(It start, It end, std::ostream& key)
{
	auto template_start = template_get_start(start, end);

	if (template_start == end)
	{
		return false;
	}

	auto arg_count = std::count(template_start, end, '$');

	template_get_key(start, template_start, arg_count, key);

	return true;
}

bool template_get_key(std::string_view string, std::ostream& key)
{
	return template_get_key(string.begin(), string.end(), key);
}
