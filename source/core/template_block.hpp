#pragma once

struct Template_Block
{
	std::vector<char> pattern;

	std::vector<char> content;

	size_t name_index;

	size_t name_size;

	Template_Block()
	{
	}

	Template_Block(std::string_view pattern, Block block) : pattern(pattern.begin(), pattern.end()), content(block.content.begin(), block.content.end())
	{
		name_index = block.name.data() - block.content.data();

		name_size = block.name.size();
	}

	std::string_view translate(Block block, std::string_view block_part) const
	{
		if (block_part.empty())
		{
			return block_part;
		}

		auto part_index = block_part.data() - block.name.data();

		auto part_size = block_part.size();

		std::string_view template_part(content.data() + name_index + part_index, part_size);

		return template_part;
	}

	std::string instantiate(std::string_view instance) const
	{
		std::string renamed_context(content.begin(), content.end());

		renamed_context.replace(name_index, name_size, instance);

		return renamed_context;
	}

	std::string_view get_pattern() const
	{
		std::string_view pattern_view(pattern.begin(), pattern.end());

		return pattern_view;
	}
};
