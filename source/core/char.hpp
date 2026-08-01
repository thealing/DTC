#pragma once

bool char_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool char_is_symbol(char c)
{
	return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '_' || c == '$';
}

bool char_is_template_name(std::string_view name)
{
	return std::count(name.begin(), name.end(), '$') > 0;
}
