#pragma once

class Compiler_Arguments
{
public:

	bool stop_on_error;

	bool insert_line_directives;

	bool conformance_mode;

	bool expand_macros_in_definitions;

public:

	Compiler_Arguments()
	{
	}
};

Compiler_Arguments compiler_arguments;
