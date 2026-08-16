#pragma once

class Template_Block
{
private:

	size_t _id;

	std::vector<char> _pattern;

	std::vector<char> _content;

	size_t _name_index;

	size_t _name_size;

public:

	Template_Block()
	{
	}

	Template_Block(size_t id, std::string_view pattern, Block block)
	{
		_id = id;

		_pattern.assign(pattern.begin(), pattern.end());

		_content.assign(block.content.begin(), block.content.end());

		_name_index = block.name.data() - block.content.data();

		_name_size = block.name.size();
	}

	size_t get_id() const
	{
		return _id;
	}

	std::string_view get_name() const
	{
		std::string_view name(_content.data() + _name_index, _name_size);

		return name;
	}

	std::string_view translate(Block block, std::string_view block_part) const
	{
		if (block_part.empty())
		{
			return block_part;
		}

		auto part_index = block_part.data() - block.name.data();

		auto part_size = block_part.size();

		std::string_view template_part(_content.data() + _name_index + part_index, part_size);

		return template_part;
	}

	std::string instantiate(std::string_view instance) const
	{
		std::string renamed_context(_content.begin(), _content.end());

		renamed_context.replace(_name_index, _name_size, instance);

		return renamed_context;
	}

	std::string_view get_pattern() const
	{
		std::string_view pattern_view(_pattern.begin(), _pattern.end());

		return pattern_view;
	}
};
