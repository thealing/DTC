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

	for (std::string_view argument : arguments)
	{
		if (argument == "-e")
		{
			compiler_arguments.stop_on_error = true;

			continue;
		}

		if (argument == "-l")
		{
			compiler_arguments.add_line_directives = true;

			continue;
		}

		if (argument.starts_with('-'))
		{
			std::cerr << "invalid argument: " << argument << std::endl;

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

		return;
	}

	if (source_path_views.empty())
	{
		std::cerr << "missing source files" << std::endl;

		return;
	}
	
	fs::path output_path = output_path_view;

	fs::path output_dir = output_path.parent_path();

	cli_create_directories(output_dir);

	std::ofstream output_file(output_path);

	if (output_file.is_open() == false)
	{
		std::cerr << "invalid output file: " << cli_path_to_string(output_path) << std::endl;

		return;
	}

	std::cout << "output file: " << cli_path_to_string(output_path) << std::endl;

	Compiler compiler;

	for (auto source_path_view : source_path_views)
	{
		fs::path source_path = source_path_view;

		std::ifstream source_file(source_path);

		if (source_file.is_open() == false)
		{
			std::cerr << "invalid source file: " << cli_path_to_string(source_path) << std::endl;

			return;
		}

		std::cout << "compiling dtl: " << cli_path_to_string(source_path) << std::endl;

		std::ostringstream source_stream;

		source_stream << source_file.rdbuf();

		std::string source = std::move(source_stream).str();

		File_Error_Reporter reporter(source_path.generic_string(), source);

		compiler.set_error_reporter(&reporter);

		auto file_name = source_path.generic_string();

		auto output = compiler.compile(file_name, source);

		output_file << output;

		output_file << "\n\n";
	}
}
