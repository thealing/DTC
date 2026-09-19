#pragma once

class Namespace_Registry
{
private:

	struct Trie_Node
	{
		std::set<std::string_view> symbols;

		std::map<std::string_view, ptrdiff_t> map;
	};

	std::vector<Trie_Node> _trie;

public:

	Namespace_Registry() : _trie(1)
	{
	}

	void add_symbol(std::string_view symbol_namespace, std::string_view symbol_name)
	{
		auto trie_it = _trie.begin();

		auto it = symbol_namespace.begin();

		auto end = symbol_namespace.end();

		while (it != end)
		{
			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			auto trie_index = _trie.end() - _trie.begin();

			auto trie_result = trie_it->map.try_emplace(part, trie_index);

			if (trie_result.second)
			{
				_trie.emplace_back();

				trie_it = _trie.end() - 1;
			}
			else
			{
				trie_it = _trie.begin() + trie_result.first->second;
			}

			if (it != end)
			{
				it++;
			}
		}

		trie_it->symbols.insert(symbol_name);
	}

	std::string_view find_symbol_namespace(std::string_view current_namespace, std::string_view string) const
	{
		auto start = current_namespace.begin();

		auto end = current_namespace.end();

		auto it = start;

		auto trie_it = _trie.begin();

		std::string_view result;

		while (it != end)
		{
			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			auto trie_map_it = trie_it->map.find(part);

			auto trie_map_end = trie_it->map.end();

			if (trie_map_it == trie_map_end)
			{
				break;
			}

			trie_it = _trie.begin() + trie_map_it->second;

			if (match_symbol(trie_it, string))
			{
				result = { start, it };
			}

			if (it != end)
			{
				it++;
			}
		}

		return result;
	}

	std::string_view find_namespace_prefix(std::string_view string) const
	{
		auto start = string.begin();

		auto end = string.end();

		auto it = start;

		auto trie_it = _trie.begin();

		std::string_view result;

		while (it != end)
		{
			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			auto trie_map_it = trie_it->map.find(part);

			auto trie_map_end = trie_it->map.end();

			if (trie_map_it == trie_map_end)
			{
				break;
			}

			trie_it = _trie.begin() + trie_map_it->second;

			if (it != end)
			{
				it++;

				result = { start, it };
			}
		}

		return result;
	}

private:

	template<typename Trie_It>
	bool match_symbol(Trie_It trie_it, std::string_view string) const
	{
		auto it = string.begin();

		auto end = string.end();

		while (it != end)
		{
			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			if (trie_it->symbols.contains(part))
			{
				return true;
			}

			auto trie_map_it = trie_it->map.find(part);

			auto trie_map_end = trie_it->map.end();

			if (trie_map_it == trie_map_end)
			{
				break;
			}

			trie_it = _trie.begin() + trie_map_it->second;

			if (it != end)
			{
				it++;
			}
		}

		return false;
	}
};
