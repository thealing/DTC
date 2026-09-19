#pragma once

class Namespace_Manager
{
private:

	Namespace_Registry _registry;

	std::set<std::string> _namespaces;

	std::string _namespace_builder;

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
		if (_namespace_builder.empty())
		{
			return false;
		}

		while (true)
		{
			_namespace_builder.pop_back();

			if (_namespace_builder.empty())
			{
				break;
			}

			if (_namespace_builder.back() == '$')
			{
				_namespace_builder.pop_back();

				break;
			}
		}

		auto namespace_result = _namespaces.insert(_namespace_builder);

		_current_namespace = *namespace_result.first;

		return true;
	}

	std::string register_block(Block& block)
	{
		if (_current_namespace.empty())
		{
			return "";
		}

		_registry.add_symbol(_current_namespace, block.name);

		auto start = block.content.begin();

		auto end = block.content.end();

		auto it = start;

		std::string buffer;

		ptrdiff_t name_start = 0;

		ptrdiff_t name_end = 0;

		while (true)
		{
			auto segment_start = it;

			string_skip_non_word(it, end);

			buffer.append(segment_start, it);

			if (it == end)
			{
				break;
			}

			auto symbol_start = it;

			string_skip_word(it, end);

			std::string_view symbol(symbol_start, it);

			auto symbol_namespace = _registry.find_symbol(_current_namespace, symbol);
			
			if (symbol_namespace.empty())
			{
				buffer += symbol;

				continue;
			}

			if (symbol == block.name)
			{
				name_start = buffer.end() - buffer.begin();
			}

			buffer += symbol_namespace;

			buffer += '$';

			buffer += symbol;

			if (symbol == block.name)
			{
				name_end = buffer.end() - buffer.begin();
			}
		}

		if (name_start == name_end)
		{
			return "";
		}

		block.content = buffer;

		block.name = { buffer.begin() + name_start, buffer.begin() + name_end };

		return buffer;
	}

	size_t find_namespace_length(std::string_view symbol) const
	{
		auto namespace_prefix = _registry.find_namespace_prefix(symbol);

		return namespace_prefix.size();
	}
};
