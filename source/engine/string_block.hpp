#pragma once

struct String_Block
{
	std::string name;

	std::string content;

	String_Block()
	{
	}

	String_Block(Block block)
	{
		name = block.name;

		content = block.content;
	}

	operator Block() const
	{
		Block block;

		block.name = name;

		block.content = content;

		return block;
	}
};
