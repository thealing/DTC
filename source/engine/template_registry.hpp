#pragma once

class Template_Registry
{
private:

	std::deque<Template_Block> _template_blocks;

	std::deque<String_Map<size_t>> _trie;

	String_Pair_Map<size_t, size_t> _trie_map;

public:

	Template_Registry()
	{
	}

	template<typename It, typename Block>
	bool add_template_special(std::string_view base, It par_start, It par_end, Block&& block)
	{
		auto par_count = std::distance(par_start, par_end);

		auto trie_key = std::make_pair(base, par_count);

		auto trie_index = _trie.size();

		auto trie_map_result = _trie_map.emplace(trie_key, trie_index);

		if (trie_map_result.second)
		{
			_trie.emplace_back();
		}
		else
		{
			trie_index = trie_map_result.first->second;
		}

		auto par_it = par_start;

		while (true)
		{
			auto& trie_node = _trie[trie_index];

			auto par = *par_it;

			par_it++;

			if (par_it == par_end)
			{
				auto template_index = _template_blocks.size();

				auto trie_result = trie_node.emplace(par, template_index);

				if (trie_result.second)
				{
					_template_blocks.emplace_back(std::forward<Block>(block));
				}
				
				return trie_result.second;
			}

			trie_index = _trie.size();

			auto trie_result = trie_node.emplace(par, trie_index);

			if (trie_result.second)
			{
				_trie.emplace_back();
			}
			else
			{
				trie_index = trie_result.first->second;
			}
		}
	}

	template<typename It>
	const Template_Block* find_special(std::string_view base, It arg_start, It arg_end) const
	{
		auto arg_count = std::distance(arg_start, arg_end);

		auto trie_key = std::make_pair(base, arg_count);

		auto trie_map_it = _trie_map.find(trie_key);

		if (trie_map_it == _trie_map.end())
		{
			return nullptr;
		}

		auto trie_index = trie_map_it->second;

		auto arg_it = arg_start;
		
		while (true)
		{
			const auto& trie_node = _trie[trie_index];

			auto arg = *arg_it;

			auto trie_it = trie_node.find(arg);

			auto trie_end = trie_node.end();

			if (trie_it == trie_end)
			{
				if (arg == "*")
				{
					return nullptr;
				}

				trie_it = trie_node.find("*");

				if (trie_it == trie_end)
				{
					return nullptr;
				}
			}

			arg_it++;

			if (arg_it == arg_end)
			{
				auto template_index = trie_it->second;

				return &_template_blocks[template_index];
			}

			trie_index = trie_it->second;
		}
	}
};
