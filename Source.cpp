#include "source/source.hpp"

int main(int argc, char** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	try
	{
		cli_run(arguments);

		return 0;
	}
	catch (int exit_code)
	{
		return exit_code;
	}
}
