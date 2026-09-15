#include "source/source.hpp"

int main(int argc, char** argv)
{

	{

		Namespace_Registry nr;

		nr.add_symbol("a$b", "foo");
		nr.add_symbol("a$b$c", "foo");
		nr.add_symbol("a$c", "foo");
		nr.add_symbol("", "foo");
		nr.add_symbol("e$f", "foo");

		auto find = [&](std::string_view ns)
		{
			return nr.find_symbol_namespace(ns, "foo");
		};

		std::cout << find("a") << std::endl;
		std::cout << find("a$b") << std::endl;
		std::cout << find("a$b$c$d$e") << std::endl;
		std::cout << find("a$b$d$e") << std::endl;
		std::cout << find("a$c") << std::endl;
		std::cout << find("a$c$d$e") << std::endl;
		std::cout << find("e") << std::endl;
		std::cout << find("e$f") << std::endl;
		std::cout << find("e$f$g") << std::endl;
		std::cout << find("") << std::endl;



		std::cout << nr.find_symbol_namespace("a", "b$foo") << std::endl;
		std::cout << nr.find_symbol_namespace("a", "c$foo") << std::endl;
		std::cout << nr.find_symbol_namespace("a", "b$c$foo") << std::endl;
		std::cout << nr.find_symbol_namespace("a", "d$foo") << std::endl;

		return 0;
	}

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
