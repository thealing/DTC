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

		val_stream << declarator_parameter.name;
		
		val_stream << ' ';

		auto val_name = get_random_string("val");

		val_stream << val_name;

		val_stream << ';';

		val_stream << '\n';
	}

	void generate_struct(std::ostream& source_stream, std::ostream& val_stream)
	{
		auto type = get_random_struct_type();

		source_stream << type;

		source_stream << ';';

		source_stream << '\n';

		auto name_length = type.find(" {");

		std::string_view name(type.data(), name_length);

		val_stream << name;

		val_stream << ' ';

		auto val_name = get_random_string("val");

		val_stream << val_name;

		val_stream << ';';

		val_stream << '\n';
	}

	void generate_function(std::ostream& source_stream, std::ostream& val_stream)
	{
		auto base_type = get_random_base_type();

		source_stream << base_type;

		source_stream << ' ';

		auto name = get_random_string("function");

		source_stream << name;

		auto par_count = get_random_int(5);

		auto function = get_random_function(par_count, true);

		source_stream << function;

		source_stream << '\n';

		source_stream << '{';

		source_stream << '\n';

		int line_count = get_random_int(2, 4);

		for (int line_index = 0; line_index < line_count; line_index++)
		{
			source_stream << '\t';

			auto type = get_random_base_type();

			source_stream << type;

			source_stream << ' ';

			auto local = get_random_string("local");

			source_stream << local;

			source_stream << ';';

			source_stream << '\n';
		}

		source_stream << '\t';

		source_stream << "return 0;";

		source_stream << '\n';

		source_stream << '}';

		source_stream << '\n';

		val_stream << "void *";

		auto val_name = get_random_string("val");

		val_stream << val_name;

		val_stream << ' ';

		val_stream << '=';

		val_stream << ' ';

		val_stream << name;

		val_stream << ';';

		val_stream << '\n';
	}

	void generate_global(std::ostream& source_stream, std::ostream& val_stream)
	{
		auto value_choice = get_random_int(6);

		auto struct_type = get_random_bool();

		if (struct_type && value_choice >= 3)
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

		auto name = get_random_string("global");

		Declarator_Parameter declarator_parameter;

		declarator_parameter.name = name;

		declarator_parameter.direct_type = true;

		if (value_choice == 1)
		{
			auto array = get_random_array();

			declarator_parameter.name += array;
		}

		if (value_choice == 2)
		{
			declarator_parameter.name = '*' + declarator_parameter.name;
		}

		if (value_choice == 0)
		{
			source_stream << declarator_parameter.name;

			source_stream << " = ";

			auto value = get_random_int(10, 99);

			source_stream << value;
		}
		else
		{
			auto declarator = get_random_declarator(declarator_parameter);

			source_stream << declarator;
		}

		if (value_choice == 1)
		{
			source_stream << " = { ";

			auto value = get_random_int(10, 99);

			source_stream << value;

			source_stream << " }";
		}

		if (value_choice == 2)
		{
			source_stream << " = 0";
		}

		source_stream << ';';

		source_stream << '\n';

		val_stream << "char ";

		auto val_name = get_random_string("val");

		val_stream << val_name;

		val_stream << "[sizeof(";

		val_stream << name;

		val_stream << ")]";

		val_stream << ';';

		val_stream << '\n';
	}

	void generate_block(std::ostream& source_stream, std::ostream& val_stream)
	{
		auto choice = get_random_real();

		if (choice < 0.2)
		{
			generate_typedef(source_stream, val_stream);

			return;
		}

		if (choice < 0.4)
		{
			generate_struct(source_stream, val_stream);

			return;
		}

		if (choice < 0.6)
		{
			generate_function(source_stream, val_stream);

			return;
		}

		if (choice < 1.0)
		{
			generate_global(source_stream, val_stream);

			return;
		}

		
	}

	void generate_source(std::ostream& source_stream, std::ostream& val_stream, int block_count)
	{
		for (int block_index = 0; block_index < block_count; block_index++)
		{
			generate_block(source_stream, val_stream);

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

		bool is_pointer = false;

		bool is_array = false;

		bool is_function = false;

		for (int shell_index = 0; shell_index < shell_count; shell_index++)
		{
			if (result.starts_with('*'))
			{
				is_pointer = true;
			}

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
				if (is_function)
				{
					is_pointer = true;
				}

				if (is_pointer)
				{
					if (result.starts_with('*') == false)
					{
						result = '*' + result;
					}

					result = '(' + result + ')';
				}

				auto array = get_random_array();

				result += array;

				is_pointer = false;

				is_array = true;

				is_function = false;

				continue;
			}

			if (choice < 1.0)
			{
				auto par_count = get_random_int(5);

				auto function = get_random_function(par_count, false);

				if (parameter.direct_type)
				{
					is_pointer = true;
				}

				if (is_array || is_function)
				{
					is_pointer = true;
				}

				if (is_pointer)
				{
					if (result.starts_with('*') == false)
					{
						result = '*' + result;
					}

					result = '(' + result + ')';
				}

				result += function;

				is_pointer = false;

				is_array = false;

				is_function = true;

				continue;
			}
		}

		return result;
	}

	std::string get_random_array()
	{
		std::string result;

		result += '[';

		auto size = get_random_int(1, 9);

		result += std::to_string(size);

		result += ']';

		return result;
	}

	std::string get_random_function(int par_count, bool definition)
	{
		std::string result;

		result += '(';

		for (int par_index = 0; par_index < par_count; par_index++)
		{
			auto type = get_random_base_type();

			result += type;

			if (definition)
			{
				result += ' ';

				auto par_name = get_random_string("par");

				result += par_name;
			}

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

		auto name = get_random_string(base);

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

					auto value = get_random_int(10, 99);

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
