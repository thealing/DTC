#pragma once

template<typename It>
class Line_Iterator
{
private:

	It _line_it;

	size_t _line_number;

public:

	Line_Iterator(It it, size_t number) : _line_it(it), _line_number(number)
	{
	}

	void set_line_number(It it, size_t number)
	{
		_line_it = it;

		_line_number = number;
	}

	size_t get_line_number(It it)
	{
		while (string_find(_line_it, it, '\n'))
		{
			_line_number++;

			_line_it++;
		}

		return _line_number;
	}
};
