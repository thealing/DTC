#pragma once

// DEBUG //

std::string load_file_to_string(const std::string& path) {
	std::ifstream file(path);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Usage in your parser test:
std::string C_PARSER_TEST_CASES = load_file_to_string(R"(C:\Users\user\source\repos\DTC\source\main\_example.c)");

 int testparse()
{
	Compiler c;

	c.compile(C_PARSER_TEST_CASES);

	return 0;
}
auto c = testparse();
// END DEBUG //

void client_run(const std::vector<std::string>& arguments)
{
}
