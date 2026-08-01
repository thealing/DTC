#pragma once

struct Block
{
	std::string_view name;

	std::string_view content;
};

class Parser
{
private:

	using It = std::string_view::iterator;

	std::string_view _source;

	size_t _end_index;

	Block _block;

public:

	Parser(std::string_view source)
	{
		_source = source;

		_end_index = 0;
	}

	void parse()
	{
		if (_source.starts_with("struct"))
		{
			parse_struct();
		}
	}

	size_t get_end_index() const
	{
		return _end_index;
	}

	Block get_block() const
	{
		return _block;
	}

private:

	void parse_struct()
	{
		It it = _source.begin();

		It block_start = it;

		skip_symbol(it);

		skip_space(it);

		It name_start = it;

		skip_symbol(it);

		It name_end = it;

		skip_block(it);

		skip_statement(it);

		It block_end = it;

		if (it != _source.end())
		{
			_end_index = it - _source.begin();

			_block.name = { name_start, name_end };

			_block.content = { block_start, block_end };
		}
	}

	void skip_symbol(It& it) const
	{
		while (it != _source.end() && char_is_symbol(*it))
		{
			it++;
		}
	}

	void skip_space(It& it) const
	{
		while (it != _source.end() && char_is_space(*it))
		{
			it++;
		}
	}

	void skip_block(It& it) const
	{
		while (it != _source.end() && *it != '{')
		{
			it++;
		}

		size_t level = 0;

		while (it != _source.end())
		{
			if (*it == '{')
			{
				level++;
			}

			if (*it == '}')
			{
				level--;
			}

			it++;

			if (level == 0)
			{
				break;
			}
		}
	}

	void skip_statement(It& it) const
	{
		while (it != _source.end() && *it != ';')
		{
			it++;
		}

		if (it != _source.end())
		{
			it++;
		}
	}
};
