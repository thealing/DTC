#pragma once

class File_Error_Reporter : public Error_Reporter
{
private:

	std::string _file_name;

	std::string _file_content;

	size_t _seek_location;

	size_t _line;

public:

	File_Error_Reporter(std::string_view file_name, std::string_view file_content)
	{
		_file_name = file_name;

		_file_content = file_content;

		_seek_location = 0;

		_line = 1;
	}

	void report_error(size_t location, std::string_view message) override
	{
		while (_seek_location < location)
		{
			if (_file_content[_seek_location] == '\n')
			{
				_line++;
			}

			_seek_location++;
		}

		std::cerr << _file_name << "(" << _line << "): error: " << message << std::endl;
	}
};
