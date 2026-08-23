#pragma once

class Compiler_Arguments
{
public:

	bool stop_on_error;

	bool insert_line_directives;

public:

	Compiler_Arguments()
	{
	}
};

Compiler_Arguments compiler_arguments;
