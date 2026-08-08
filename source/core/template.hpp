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

	auto arg_start = it;

	while (it != end)
	{
		string_find(it, end, '$');

		if (level == 0)
		{
			if (it == arg_start)
			{
				return false;
			}

			std::string_view arg(arg_start, it);

			*inserter = arg;

			arg_start = it;

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
bool template_split_special(It start, It end, std::string& base, std::string& key)
{
	ptrdiff_t level = 0;

	auto it = start;

	auto part_start = it;

	while (it != end)
	{
		string_find(it, end, '$');

		if (level == 0)
		{
			if (it == part_start)
			{
				return false;
			}

			std::string_view part(part_start, it);

			if (part_start == start)
			{
				base += part;
			}
			else
			{
				if (part[1] == '$')
				{
					key += part.substr(1);
				}
				else
				{
					key += '*';
				}
			}

			part_start = it;

			level++;
		}

		auto dollar_start = it;

		string_skip(it, end, '$');

		auto dollar_count = it - dollar_start;

		if (dollar_start == part_start && dollar_count > 1)
		{
			dollar_count--;
		}

		level += dollar_count - 2;
	}

	if (level != -1)
	{
		return false;
	}

	return true;
}

bool template_split_special(std::string_view string, std::string& base, std::string& key)
{
	return template_split_special(string.begin(), string.end(), base, key);
}
