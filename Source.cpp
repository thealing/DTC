#include "source/source.hpp"

int main1(int argc, char** argv)
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

int main()
{
	for (int i = 0; i < 10; i++) {
		int x = clock();
		std::vector<std::string> arguments;

		arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\build.c)");
		arguments.push_back(R"(C:\Users\user\source\repos\DTC\_\benchmark.c)");
		cli_run(arguments);
		std::cout << "time " << clock() - x << std::endl;
	}
}
