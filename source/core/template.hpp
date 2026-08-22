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

	auto distance = std::distance(source_start, source_end);

	result.reserve(distance * 2);

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
	auto par_it = pattern.begin();

	auto par_end = pattern.end();

	auto arg_it = arg_start;

	while (par_it != par_end)
	{
		auto par_start = par_it;

		string_skip(par_it, par_end, '$');

		string_find(par_it, par_end, '$');

		std::string_view par(par_start, par_it);
		
		if (par[1] == '$')
		{
			arg_it++;
			
			continue;
		}
		
		auto arg_data = arg_it->first.data();

		arg_it += arg_it->second;

		auto arg_size = arg_it->first.data() - arg_data;
		
		std::string_view arg(arg_data, arg_size);
		
		content = template_replace(content, par, arg);
	}
}

template<typename It, typename Container, bool Special>
ptrdiff_t template_split_template_part(It& it, It end, Container& container)
{
	if (it == end)
	{
		return -1;
	}

	auto start = it;

	string_skip(it, end, '$');

	auto sub_count = it - start;

	string_find(it, end, '$');

	auto part_index = container.size();

	std::string_view part(start, it);

	container.emplace_back(part, 0);

	if constexpr (Special)
	{
		if (sub_count == 1)
		{
			return 1;
		}

		sub_count--;

		container[part_index].first.remove_prefix(1);
	}

	ptrdiff_t part_count = 1;

	for (ptrdiff_t sub_index = 0; sub_index < sub_count - 1; sub_index++)
	{
		auto result = template_split_template_part<It, Container, Special>(it, end, container);

		if (result == -1)
		{
			return -1;
		}

		part_count += result;
	}

	container[part_index].second = part_count;

	return part_count;
}

template<typename It, typename Container, bool Special = false>
bool template_split_template(It start, It end, Container& container)
{
	auto it = start;

	string_find(it, end, '$');

	auto base_index = container.size();

	std::string_view base(start, it);

	container.emplace_back(base, 0);

	ptrdiff_t base_count = 0;

	while (it != end)
	{
		auto result = template_split_template_part<It, Container, Special>(it, end, container);

		if (result == -1)
		{
			return false;
		}

		base_count++;
	}

	container[base_index].second = base_count;

	return true;
}

template<typename It, typename Container>
bool template_split_instance(It start, It end, Container& container)
{
	return template_split_template<It, Container, false>(start, end, container);
}

template<typename Container>
bool template_split_instance(std::string_view string, Container& container)
{
	return template_split_instance(string.begin(), string.end(), container);
}

template<typename It, typename Container>
bool template_split_special(It start, It end, Container& container)
{
	return template_split_template<It, Container, true>(start, end, container);
}

template<typename Container>
bool template_split_special(std::string_view string, Container& container)
{
	return template_split_special(string.begin(), string.end(), container);
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
