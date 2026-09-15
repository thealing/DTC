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

			auto trie_index = std::distance(_trie.begin(), trie_node);

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
		}

		trie_node->symbols.emplace(symbol_name);
	}

	std::string_view find_symbol_namespace(std::string_view current_namespace, std::string_view string) const
	{
		std::string_view result;

		auto trie_node = _trie.begin();

		find_symbol_namespace<false>(result, trie_node, current_namespace, string);

		return result;
	}

private:

	template<bool Strict, typename Trie_Node>
	void find_symbol_namespace(std::string_view& result, Trie_Node trie_node, std::string_view current_namespace, std::string_view string) const
	{
		auto it = current_namespace.begin();

		auto end = current_namespace.end();

		while (it != end)
		{
			auto string_start = string.begin();

			auto string_end = string.end();

			auto string_it = string_start;

			string_find(string_it, string_end, '$');

			std::string string_prefix(string_start, string_it);

			bool string_prefix_found = trie_node->symbols.contains(string_prefix);

			if (string_prefix_found)
			{
				result = string;
			}

			if constexpr (Strict)
			{
				if (string_prefix_found == false)
				{
					break;
				}
			}

			auto part_start = it;

			string_find(it, end, '$');

			std::string_view part(part_start, it);

			auto trie_it = trie_node->map.find(part);

			auto trie_end = trie_node->map.end();

			if (trie_it == trie_end)
			{
				break;
			}

			trie_node = _trie.begin();

			std::advance(trie_node, trie_it->second);

			if constexpr (Strict == false)
			{
				if (string_prefix_found)
				{
					std::string_view inner_namespace(it, end);

					std::string_view string_suffix(string_it, string_end);

					find_symbol_namespace<true>(result, trie_node, inner_namespace, string_suffix);
				}
			}
		}
	}
};
