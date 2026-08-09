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

	std::string instantiate(std::string_view instance) const
	{
		std::string ic = content;

		ic.replace(name_index, name_size, instance);

		return ic;
	}
};
