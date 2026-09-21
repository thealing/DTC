#pragma once

namespace fs = std::filesystem;

void cli_create_directories(const fs::path& path)
{
	std::error_code ec;

	fs::create_directories(path, ec);
}

std::string cli_path_to_string(const fs::path& path)
{
	return "\"" + path.generic_string() + "\"";
}

void cli_run(const std::vector<std::string>& arguments)
{
	std::string_view output_path_view;

	std::vector<std::string_view> source_path_views;

	Compiler_Arguments compiler_arguments = {};

	for (std::string_view argument : arguments)
	{
		if (argument.starts_with('-'))
		{
			for (auto c : argument.substr(1))
			{
				if (c == 'e')
				{
					compiler_arguments.stop_on_error = true;

					continue;
				}

				if (c == 'l')
				{
					compiler_arguments.insert_line_directives = true;

					continue;
				}

				if (c == 'c')
				{
					compiler_arguments.conformance_mode = true;

					continue;
				}

				if (c == 'n')
				{
					compiler_arguments.expand_macros_in_definitions = true;

					continue;
				}

				std::cerr << "invalid flag: " << c << std::endl;

				throw 1;
			}

			continue;
		}

		if (output_path_view.empty())
		{
			output_path_view = argument;
		}
		else
		{
			source_path_views.push_back(argument);
		}
	}

	if (output_path_view.empty())
	{
		std::cerr << "missing output file" << std::endl;

		throw 1;
	}

	if (source_path_views.empty())
	{
		std::cerr << "missing source files" << std::endl;

		throw 1;
	}

	fs::path output_path = output_path_view;

	fs::path output_dir = output_path.parent_path();

	cli_create_directories(output_dir);

	std::ofstream output_file(output_path);

	if (output_file.is_open() == false)
	{
		std::cerr << "invalid output file: " << cli_path_to_string(output_path) << std::endl;

		throw 1;
	}

	std::cout << "output file: " << cli_path_to_string(output_path) << std::endl;

	Compiler compiler(compiler_arguments);

	for (auto source_path_view : source_path_views)
	{
		fs::path source_path = source_path_view;

		std::ifstream source_file(source_path);

		if (source_file.is_open() == false)
		{
			std::cerr << "invalid source file: " << cli_path_to_string(source_path) << std::endl;

			throw 1;
		}

		std::cout << "compiling: " << cli_path_to_string(source_path) << std::endl;

		std::ostringstream source_stream;

		source_stream << source_file.rdbuf();

		std::string source = std::move(source_stream).str();

		auto file_name = source_path.generic_string();

		auto output = compiler.compile(file_name, source);

		output_file << output;
	}
}
