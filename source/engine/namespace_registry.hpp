#pragma once

class Namespace_Registry
{
private:

	struct Trie_Node
	{
		std::set<std::string_view> symbols;

		std::map<std::string_view, ptrdiff_t> map;

		std::string_view path;
	};

	std::vector<Trie_Node> _trie;

public:

	Namespace_Registry() : _trie(1)
	{
	}

	void add_symbol(std::string_view symbol_namespace, std::string_view symbol_name)
	{
		auto trie_node = _trie.begin();

		auto start = symbol_namespace.begin();

		auto end = symbol_namespace.end();

		auto it = start;

		while (it != end)
		{
			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			auto trie_index = _trie.size();

			auto trie_result = trie_node->map.emplace(part, trie_index);

			if (trie_result.second)
			{
				_trie.emplace_back();

				trie_node = _trie.end() - 1;
			}
			else
			{
				trie_node = _trie.begin() + trie_result.first->second;
			}

			trie_node->path = { start, it };

			if (it != end)
			{
				it++;
			}
		}

		trie_node->symbols.emplace(symbol_name);
	}

	std::string_view find_symbol_namespace(std::string_view current_namespace, std::string_view string) const
	{
		auto start = current_namespace.begin();

		auto end = current_namespace.end();

		auto it = start;

		auto trie_node = _trie.begin();

		find_symbol_namespace<false>(it, start, end, trie_node, string);

		std::string_view result(start, it);

		if (result.ends_with('$'))
		{
			result.remove_suffix(1);
		}

		return result;
	}

private:

	template<bool Strict, typename It, typename Trie_Node>
	void find_symbol_namespace(It& result_it, It it, It end, Trie_Node trie_node, std::string_view string) const
	{
		auto string_start = string.begin();

		auto string_end = string.end();

		auto string_it = string_start;

		string_find(string_it, string_end, '$');

		std::string string_prefix(string_start, string_it);

		if (trie_node->symbols.contains(string_prefix))
		{
			result_it = it;
		}

		auto part_start = it;

		string_find(it, end, '$');

		std::string_view part(part_start, it);

		if constexpr (Strict)
		{
			if (part != string_prefix)
			{
				return;
			}
		}

		auto trie_it = trie_node->map.find(part);

		auto trie_end = trie_node->map.end();

		if (trie_it == trie_end)
		{
			return;
		}

		trie_node = _trie.begin() + trie_it->second;

		if (it != end)
		{
			it++;
		}

		find_symbol_namespace<Strict>(result_it, it, end, trie_node, string);

		if constexpr (Strict == false)
		{
			if (string_it != string_end)
			{
				string_it++;

				std::string_view string_suffix(string_it, string_end);

				find_symbol_namespace<true>(result_it, it, end, trie_node, string_suffix);
			}
		}
	}
};
