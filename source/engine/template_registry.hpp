#pragma once

class Template_Registry
{
private:

	std::vector<std::pair<std::string, String_Block>> _specials;

public:

	Template_Registry()
	{
	}

	void add_template_special(std::string_view key, Block block)
	{
		_specials.emplace_back(key, block);
	}

	template<typename It>
	bool find_special(It arg_start, It arg_end, Block& block) const
	{
		for (auto special_it = _specials.rbegin(); special_it != _specials.rend(); special_it++)
		{
			const auto& key = special_it->first;

			auto key_it = key.begin();

			auto key_end = key.end();

			bool match_found = true;

			for (auto arg_it = arg_start; arg_it != arg_end; arg_it++)
			{
				std::string_view key_slice(key_it, key_end);

				if (key_slice.starts_with(*arg_it))
				{
					key_it += arg_it->size();

					continue;
				}

				if (key_it != key_end && *key_it == '*')
				{
					key_it++;

					continue;
				}

				match_found = false;

				break;
			}

			if (match_found)
			{
				block = special_it->second;

				return true;
			}
		}

		return false;
	}
};
