#pragma once

class Namespace_Manager
{
private:

	Namespace_Registry _registry;

	std::set<std::string> _namespaces;

	std::string _namespace_builder;

	std::vector<size_t> _namespace_offsets;

	std::string_view _current_namespace;

public:

	Namespace_Manager()
	{
	}

	bool enter(std::string_view namespace_name)
	{
		if (namespace_name.empty())
		{
			return false;
		}

		_namespace_offsets.push_back(_namespace_builder.size());

		if (_namespace_builder.empty() == false)
		{
			_namespace_builder += '$';
		}

		_namespace_builder += namespace_name;

		auto namespace_result = _namespaces.insert(_namespace_builder);

		_current_namespace = *namespace_result.first;

		return true;
	}

	bool leave()
	{
		if (_namespace_offsets.empty())
		{
			return false;
		}

		_namespace_builder.resize(_namespace_offsets.back());

		_namespace_offsets.pop_back();

		auto namespace_result = _namespaces.insert(_namespace_builder);

		_current_namespace = *namespace_result.first;

		return true;
	}

	std::string_view register_symbol(std::string_view symbol)
	{
		if (_current_namespace.empty() == false)
		{
			_registry.add_symbol(_current_namespace, symbol);
		}

		return _current_namespace;
	}

	std::string replace_namespaces(std::string_view& block) const
	{
		std::string replace_buffer;

		if (_current_namespace.empty() == false)
		{
			auto block_start = block.begin();

			auto block_end = block.end();

			auto it = block_start;

			while (true)
			{
				string_skip_non_word(it, block_end);

				if (it == block_end)
				{
					break;
				}

				auto symbol_start = it;

				string_skip_word(it, block_end);

				std::string_view symbol(symbol_start, it);

				auto symbol_namespace = _registry.find_symbol_namespace(_current_namespace, symbol);

				if (symbol_namespace.empty())
				{
					continue;
				}

				replace_buffer.append(block_start, symbol_start);

				replace_buffer += symbol_namespace;

				replace_buffer += '$';

				replace_buffer += symbol;

				block_start = it;
			}

			if (replace_buffer.empty() == false)
			{
				replace_buffer.append(block_start, block_end);

				block = replace_buffer;
			}
		}

		return replace_buffer;
	}

	size_t find_namespace_length(std::string_view symbol) const
	{
		auto namespace_prefix = _registry.find_namespace_prefix(symbol);

		return namespace_prefix.size();
	}
};
