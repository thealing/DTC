#include "source/source.hpp"

int main(int argc, char** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\build.c)");
	arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\demo\main.dtl.i)");

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
