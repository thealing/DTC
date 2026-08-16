#pragma once

class Compiler_Arguments
{
public:

	bool stop_on_error;

	bool add_line_directives;

public:

	Compiler_Arguments()
	{
		stop_on_error = false;

		add_line_directives = false;
	}
};

Compiler_Arguments compiler_arguments;
