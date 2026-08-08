#include "source/source.hpp"

int main(int argc, char** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	cli_run(arguments);

	return 0;
}
