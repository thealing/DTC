#pragma once

struct Location
{
	std::string_view file_name;

	size_t line_number;
};

struct Template_Location : Location
{
	size_t name_line_number;
};

struct Origin
{
	Location template_location;

	Location instance_location;

	std::string_view instance_name;
};
