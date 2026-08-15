#pragma once

struct Template_Block
{
	std::string pattern;

	std::string content;

	size_t name_index;

	size_t name_size;

	Template_Block()
	{
	}

	Template_Block(std::string_view p, Block block)
	{
		pattern = p;

		content = block.content;

		name_index = block.name.data() - block.content.data();

		name_size = block.name.size();
	}

	std::string_view translate(Block block, std::string_view block_part) const
	{
		auto part_index = block_part.data() - block.name.data();

		auto part_size = block_part.size();

		if (part_index < 0 || part_index >= block.name.size())
		{
			return block_part;
		}

		std::string_view template_part(content.data() + name_index + part_index, part_size);

		return template_part;
	}

	std::string instantiate(std::string_view instance) const
	{
		std::string renamed_context = content;

		renamed_context.replace(name_index, name_size, instance);

		return renamed_context;
	}
};
