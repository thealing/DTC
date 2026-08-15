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
std::string template_replace(It source_start, It source_end, It par_start, It par_end, It arg_start, It arg_end)
{
	std::string result;

	It it = source_start;

	while (it != source_end)
	{
		It source_it = it;

		It par_it = par_start;

		bool match_found = false;

		while (*source_it == *par_it)
		{
			source_it++;

			par_it++;

			if (source_it == source_end)
			{
				if (par_it == par_end)
				{
					match_found = true;
				}

				break;
			}

			if (par_it == par_end)
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
			bool trim_start = false;

			bool concat_word = false;

			if (result.empty())
			{
				trim_start = true;
			}
			else
			{
				auto front = result.back();

				if (string_is_word(front) == false)
				{
					trim_start = true;
				}

				if (front == '$')
				{
					trim_start = true;

					concat_word = true;
				}
			}

			It arg_it = arg_start;

			if (trim_start)
			{
				string_skip(arg_it, arg_end, '$');
			}

			if (concat_word)
			{
				result.pop_back();

				if (source_it != source_end && *source_it == '$')
				{
					source_it++;
				}
			}

			while (arg_it != arg_end)
			{
				result += *arg_it;

				arg_it++;
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

std::string template_replace(std::string_view source, std::string_view par, std::string_view arg)
{
	return template_replace(source.begin(), source.end(), par.begin(), par.end(), arg.begin(), arg.end());
}

template<typename It>
void template_replace(std::string& content, std::string_view pattern, It arg_start, It arg_end)
{
	auto it = pattern.begin();

	auto end = pattern.end();

	for (It arg_it = arg_start; arg_it != arg_end && it != end; arg_it++)
	{
		auto par_end = template_get_start(it + 1, end);

		std::string_view par(it, par_end);
		
		if (par[1] != '*')
		{
			content = template_replace(content, par, *arg_it);
		}

		it = par_end;
	}
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

	if (arg_start != end)
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
bool template_split_special(It start, It end, std::string& base, std::string& key, std::string& pattern)
{
	ptrdiff_t level = 0;

	auto it = start;

	auto part_start = it;

	while (it != end)
	{
		string_find(it, end, '$');

		if (level == 0)
		{
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

					pattern += "$*";
				}
				else
				{
					if (string_is_digit(part[1]))
					{
						return false;
					}

					key += '*';

					pattern += part;
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

	if (part_start != end)
	{
		return false;
	}

	return true;
}

bool template_split_special(std::string_view string, std::string& base, std::string& key, std::string& pattern)
{
	return template_split_special(string.begin(), string.end(), base, key, pattern);
}
