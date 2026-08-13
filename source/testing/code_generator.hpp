#pragma once

class Code_Generator
{
private:

	std::mt19937 _gen;

	int _name_id;

public:

	Code_Generator() : _gen(1), _name_id(1)
	{
	}

	void generate_typedef(std::ostream& source_stream, std::ostream& val_stream)
	{
		source_stream << "typedef ";

		auto struct_type = get_random_bool();

		if (struct_type)
		{
			auto struct_type = get_random_struct_type();

			source_stream << struct_type;
		}
		else
		{
			auto base_type = get_random_base_type();

			source_stream << base_type;
		}

		source_stream << ' ';

		Declarator_Parameter declarator_parameter;

		declarator_parameter.name = get_random_string("type");

		auto declarator = get_random_declarator(declarator_parameter);

		source_stream << declarator;

		source_stream << ';';

		source_stream << '\n';

		auto val_name = get_random_string("val");

		val_stream << declarator_parameter.name;
		
		val_stream << ' ';

		val_stream << val_name;

		val_stream << ';';

		val_stream << '\n';
	}

	void generate_source(std::ostream& source_stream, std::ostream& val_stream, int block_count)
	{
		// TODO: others

		for (int block_index = 0; block_index < block_count; block_index++)
		{
			generate_typedef(source_stream, val_stream);

			source_stream << '\n';
		}
	}

private:

	struct Declarator_Parameter
	{
		std::string name;

		bool direct_type = false;
	};

	std::string get_random_declarator(Declarator_Parameter parameter)
	{
		auto result = parameter.name;

		auto shell_count = get_random_int(0, 8);

		bool added_block = false;

		for (int shell_index = 0; shell_index < shell_count; shell_index++)
		{
			auto choice = get_random_real();

			if (choice < 0.2)
			{
				result = "const " + result;

				continue;
			}

			if (choice < 0.4)
			{
				result = "volatile " + result;

				continue;
			}

			if (choice < 0.6)
			{
				result = '*' + result;

				continue;
			}

			if (choice < 0.8)
			{
				auto array = get_random_array();

				if (added_block && result.starts_with('*') == false)
				{
					result = '*' + result;
				}

				if (result.starts_with('*'))
				{
					result = '(' + result + ')';
				}

				result += array;

				added_block = true;

				continue;
			}

			if (choice < 1.0)
			{
				auto function = get_random_function();

				if (parameter.direct_type)
				{
					added_block = true;
				}

				if (added_block && result.starts_with('*') == false)
				{
					result = '*' + result;
				}

				if (result.starts_with('*'))
				{
					result = '(' + result + ')';
				}

				result += function;

				added_block = true;

				continue;
			}
		}

		return result;
	}

	std::string get_random_array()
	{
		std::string result;

		result += '[';

		auto size = get_random_int(1, 99);

		result += std::to_string(size);

		result += ']';

		return result;
	}

	std::string get_random_function()
	{
		std::string result;

		result += '(';

		auto par_count = get_random_int(5);

		for (int par_index = 0; par_index < par_count; par_index++)
		{
			auto type = get_random_base_type();

			result += type;

			if (par_index + 1 != par_count)
			{
				result += ", ";
			}
		}

		result += ')';

		return result;
	}

	std::string get_random_string(std::string_view label)
	{
		std::string result;

		result += label;

		result += '_';

		auto id = _name_id;

		_name_id++;

		result += std::to_string(id);

		return result;
	}

	std::string get_random_base_type()
	{
		static constexpr std::string_view int_bases[] = { "int", "char", "short", "long", "long long" };

		static constexpr std::string_view float_bases[] = { "float", "double", "long double" };

		auto int_type = get_random_bool();

		std::string result;

		if (int_type)
		{
			auto base_index = get_random_size(std::size(int_bases));

			auto base = int_bases[base_index];

			auto add_signedness = get_random_bool();

			if (add_signedness)
			{
				auto add_signed = get_random_bool();

				if (add_signed)
				{
					result += "signed ";
				}
				else
				{
					result += "unsigned ";
				}
			}
			
			result += base;
		}
		else
		{
			auto base_index = get_random_size(std::size(float_bases));

			auto base = float_bases[base_index];

			result += base;
		}

		return result;
	}

	std::string get_random_struct_type()
	{
		static constexpr std::string_view struct_bases[] = { "struct", "union", "enum" };

		auto base_index = get_random_size(std::size(struct_bases));

		auto base = struct_bases[base_index];

		std::string result;

		result += base;

		result += ' ';

		auto name = get_random_string("struct");

		result += name;

		result += ' ';

		result += '{';

		result += '\n';

		if (base == "enum")
		{
			auto value_count = get_random_int(1, 5);

			for (int value_index = 0; value_index < value_count; value_index++)
			{
				auto value = get_random_string("value");

				result += '\t';

				result += value;

				auto explicit_value = get_random_bool();

				if (explicit_value)
				{
					result += ' ';

					result += '=';

					result += ' ';

					auto value = get_random_int(1, 99);

					result += std::to_string(value);
				}

				if (value_index + 1 != value_count)
				{
					result += ',';
				}

				result += '\n';
			}
		}
		else
		{
			auto field_count = get_random_int(1, 5);

			for (int field_index = 0; field_index < field_count; field_index++)
			{
				result += '\t';

				auto type = get_random_base_type();

				result += type;

				result += ' ';

				Declarator_Parameter declarator_parameter;

				declarator_parameter.name = get_random_string("field");

				declarator_parameter.direct_type = true;

				auto decorator = get_random_declarator(declarator_parameter);

				result += decorator;

				result += ';';

				result += '\n';
			}
		}

		result += '}';

		result += ' ';

		return result;
	}

	int get_random_int(int max)
	{
		std::uniform_int_distribution<int> d(0, max - 1);

		return d(_gen);
	}

	size_t get_random_size(size_t size)
	{
		std::uniform_int_distribution<size_t> d(0, size - 1);

		return d(_gen);
	}

	int get_random_int(int min, int max)
	{
		std::uniform_int_distribution<int> d(min, max);

		return d(_gen);
	}

	double get_random_real()
	{
		std::uniform_real_distribution<double> d;

		return d(_gen);
	}

	bool get_random_bool()
	{
		std::uniform_real_distribution<double> d;

		return get_random_int(2);
	}
};
