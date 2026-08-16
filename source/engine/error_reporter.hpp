#pragma once

class Error_Reporter
{
public:

	virtual ~Error_Reporter() = default;

	virtual void report_error(size_t location, std::string_view message) = 0;
};
