#pragma once

// TODO: Efficient implementation
class Template_Registry
{
private:

	std::vector<std::pair<std::string, Template_Block>> _specials;

public:

	Template_Registry()
	{
	}

	template<typename Key, typename Block>
	bool add_template_special(Key&& key, Block&& block)
	{
		for (const auto& [k, b] : _specials)
		{
			if (k == key)
			{
				return false;
			}
		}

		_specials.emplace_back(std::forward<Key>(key), std::forward<Block>(block));

		return true;
	}

	template<typename It>
	bool find_special(It arg_start, It arg_end, const Template_Block*& block_ptr) const
	{
		for (auto special_it = _specials.rbegin(); special_it != _specials.rend(); special_it++)
		{
			const auto& [key, block] = *special_it;

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

			if (key_it != key_end)
			{
				match_found = false;
			}

			if (match_found)
			{
				block_ptr = &block;

				return true;
			}
		}

		return false;
	}
};
