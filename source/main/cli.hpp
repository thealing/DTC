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
		std::cout << "DTC: missing output file" << std::endl;

		return;
	}

	if (source_path_views.empty())
	{
		std::cout << "DTC: missing source files" << std::endl;

		return;
	}
	
	fs::path output_path = output_path_view;

	fs::path output_dir = output_path.parent_path();

	cli_create_directories(output_dir);

	std::ofstream output_file(output_path);

	if (output_file.is_open() == false)
	{
		std::cout << "DTC: invalid output file: " << cli_path_to_string(output_path) << std::endl;

		return;
	}

	Compiler compiler;

	for (auto source_path_view : source_path_views)
	{
		fs::path source_path = source_path_view;

		std::ifstream source_file(source_path);

		if (source_file.is_open() == false)
		{
			std::cout << "DTC: invalid source file: " << cli_path_to_string(source_path) << std::endl;

			return;
		}

		std::cout << "DTC: compiling dtl: " << cli_path_to_string(source_path) << std::endl;

		std::ostringstream source_stream;

		source_stream << source_file.rdbuf();

		std::string source = std::move(source_stream).str();

		auto output = compiler.compile(source);

		output_file << output;

		output_file << "\n\n";
	}
}
