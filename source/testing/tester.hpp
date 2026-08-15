#pragma once

constexpr int TESTER_TEST_COUNT = 200000;

void tester_run(const std::vector<std::string>& arguments)
{
	if (arguments.size() != 1)
	{
		std::cout << "Tester: missing output path" << std::endl;

		return;
	}

	std::string test_output_path = arguments[0];

	std::ostringstream source_stream;

	std::ostringstream val_stream;

	Code_Generator code_generator;

	code_generator.generate_source(source_stream, val_stream, TESTER_TEST_COUNT);

	auto source_string = std::move(source_stream).str();

	auto val_string = std::move(val_stream).str();

	Compiler compiler;

	auto compiled_code = compiler.compile(source_string);

	std::ofstream test_output_file(test_output_path);

	test_output_file << compiled_code;

	test_output_file << "\n\n";

	test_output_file << val_string;
}
