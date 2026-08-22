#pragma once

class Template_Registry
{
private:

	struct Trie_Node
	{
		size_t generic_index = 0;

		size_t end_index = 0;

		std::map<std::string_view, size_t> map;
	};

	std::vector<Template_Block> _template_blocks;

	std::vector<Trie_Node> _trie;

	std::map<std::pair<std::string_view, ptrdiff_t>, size_t> _trie_map;

	mutable std::vector<std::pair<size_t, ptrdiff_t>> _backtrack_buffer;

public:

	Template_Registry() : _template_blocks(1), _trie(1)
	{
	}

	template<typename It, typename Block>
	const Template_Block* add_template_special(It par_start, It par_end, Block&& block)
	{
		auto trie_key = *par_start;
		
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
		
		for (auto par_it = par_start + 1; par_it != par_end; par_it++)
		{
			auto& trie_node = _trie[trie_index];

			trie_index = _trie.size();
			
			if (par_it->second == 0)
			{
				if (trie_node.generic_index == 0)
				{
					trie_node.generic_index = trie_index;
					
					_trie.emplace_back();
				}
				else
				{
					trie_index = trie_node.generic_index;
				}
			}
			else
			{
				auto trie_result = trie_node.map.emplace(par_it->first, trie_index);

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

		auto& trie_node = _trie[trie_index];
		
		if (trie_node.end_index != 0)
		{
			auto template_index = trie_node.end_index;

			return &_template_blocks[template_index];
		}
		
		trie_node.end_index = _template_blocks.size();

		_template_blocks.emplace_back(std::forward<Block>(block));

		return nullptr;
	}

	template<typename It>
	const Template_Block* find_special(It arg_start, It arg_end, bool& template_exists) const
	{
		auto trie_key = *arg_start;

		auto trie_map_it = _trie_map.find(trie_key);

		if (trie_map_it == _trie_map.end())
		{
			return nullptr;
		}

		template_exists = true;

		auto trie_index = trie_map_it->second;

		_backtrack_buffer.clear();

		auto& branches = _backtrack_buffer;
		
		auto arg_it = arg_start + 1;
		
		while (arg_it != arg_end)
		{
			const auto& trie_node = _trie[trie_index];

			auto trie_it = trie_node.map.find(arg_it->first);

			auto trie_end = trie_node.map.end();
			
			if (trie_it != trie_end)
			{
				if (trie_node.generic_index != 0)
				{
					auto arg_index = std::distance(arg_start, arg_it);
					
					arg_index += arg_it->second;

					branches.emplace_back(trie_node.generic_index, arg_index);
				}

				trie_index = trie_it->second;
				
				arg_it++;

				continue;
			}

			if (trie_node.generic_index != 0)
			{
				trie_index = trie_node.generic_index;
				
				arg_it += arg_it->second;

				continue;
			}

			if (branches.empty())
			{
				return nullptr;
			}

			auto [backtrack_index, arg_index] = branches.back();

			branches.pop_back();

			trie_index = backtrack_index;

			arg_it = arg_start;
			
			std::advance(arg_it, arg_index);
		}
		
		auto& trie_node = _trie[trie_index];

		if (trie_node.end_index != 0)
		{
			auto template_index = trie_node.end_index;

			return &_template_blocks[template_index];
		}

		return nullptr;
	}
};
