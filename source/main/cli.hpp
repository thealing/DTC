#pragma once

// DEBUG //
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <sstream>

class CTypedefGenerator
{
public:
	std::vector<std::string> generated_names;

	int _counter = 1;

	explicit CTypedefGenerator(uint32_t seed = 1)
		: rng_(seed) {}

	// Generates a single random C typedef statement
	std::string generate_one()
	{
		name_counter_ = 0;
		std::ostringstream oss;
		oss << "typedef ";

		// 1. Roll for base type strategy
		int base_type_choice = roll_int(1, 100);

		if (base_type_choice <= 40)
		{
			// Simple primitive / modifier types (e.g., unsigned long long, const int)
			oss << random_primitive_type();
		}
		else if (base_type_choice <= 75)
		{
			// Struct or Union (with optional inline body definition)
			oss << generate_struct_or_union(0);
		}
		else
		{
			// Existing typedef alias name as base type
			oss << "Type" << roll_int(1, 5);
		}

		oss << " ";

		// 2. Roll for declarator complexity
		// We pass depth=0 to control recursion and avoid infinite nestings
		std::string name = "Alias_" +std::to_string( _counter++);
		std::string declarator = build_declarator(name, 0);

		generated_names.push_back(name);

		oss << declarator << ";";
		return oss.str();
	}

	// Generates a batch of N random typedef statements separated by newlines
	std::string generate_batch(size_t count)
	{
		std::ostringstream oss;
		for (size_t i = 0; i < count; ++i)
		{
			oss << generate_one() << "\n\n";
		}
		return oss.str();
	}

private:
	std::mt19937 rng_;
	int name_counter_ = 0;

	int roll_int(int min, int max)
	{
		std::uniform_int_distribution<int> dist(min, max);
		return dist(rng_);
	}

	bool roll_chance(int percentage)
	{
		return roll_int(1, 100) <= percentage;
	}

	std::string random_primitive_type()
	{
		static const std::vector<std::string> primitives = {
			"int", "char", "double", "float", "void", "short", "long long", "unsigned int", "unsigned char"
		};
		static const std::vector<std::string> qualifiers = {
			"", "const ", "volatile ", "const volatile "
		};

		return qualifiers[roll_int(0, (int)qualifiers.size() - 1)] +
			primitives[roll_int(0, (int)primitives.size() - 1)];
	}

	std::string generate_struct_or_union(int depth)
	{
		bool is_struct = roll_chance(70);
		std::string tag = is_struct ? "struct" : "union";

		bool has_tag_name = roll_chance(60);
		if (has_tag_name)
		{
			tag += " Tag_" + std::to_string(++name_counter_);
		}

		// 30% chance to be just a forward declaration / reference (struct TagName)
		// unless it's anonymous (which must have a body)
		if (has_tag_name && roll_chance(30))
		{
			return tag;
		}

		// Inline definition body: { ... }
		std::ostringstream body;
		body << tag << " {\n";

		int field_count = roll_int(1, 3);
		for (int i = 0; i < field_count; ++i)
		{
			body << "    ";
			if (depth < 2 && roll_chance(30))
			{
				// Nested struct/union member
				body << generate_struct_or_union(depth + 1) << " member_" << i;
			}
			else if (roll_chance(25))
			{
				// Function pointer member: int (*cb)(void)
				body << "int (*cb_" << i << ")(" << random_param_list(depth + 1) << ")";
			}
			else
			{
				// Standard member
				body << random_primitive_type() << " field_" << i;
				if (roll_chance(30))
				{
					body << "[" << roll_int(1, 100) << "]";
				}
			}
			body << ";\n";
		}
		body << "}";
		return body.str();
	}

	std::string random_param_list(int depth)
	{
		if (roll_chance(20)) return "void";
		if (roll_chance(15)) return "";

		int count = roll_int(1, 3);
		std::string params;
		for (int i = 0; i < count; ++i)
		{
			if (i > 0) params += ", ";

			// Abstract parameter or named parameter
			params += random_primitive_type();

			if (roll_chance(30) && depth < 2)
			{
				// Parameter is a function pointer: void (*fn)(int)
				params += " (*";
				if (roll_chance(50)) params += "p" + std::to_string(i);
				params += ")(" + random_param_list(depth + 1) + ")";
			}
			else
			{
				// Optional pointer level & optional param name
				if (roll_chance(40)) params += "*";
				if (roll_chance(50)) params += " arg" + std::to_string(i);
				if (roll_chance(20)) params += "[" + std::to_string(roll_int(1, 10)) + "]";
			}
		}
		return params;
	}

	// Recursively constructs pointers, parentheses, function signatures, and arrays
	// Recursively constructs pointers, minimal mandatory parentheses, function signatures, and arrays
	std::string build_declarator(const std::string& inner_expr, int depth)
	{
		std::string current = inner_expr;

		// 1. Add Pointers (e.g., ***inner or * const)
		bool has_pointers = false;
		if (roll_chance(60))
		{
			has_pointers = true;
			int ptrs = roll_int(1, 3);
			std::string ptr_str;
			for (int p = 0; p < ptrs; ++p)
			{
				ptr_str += "*";
				if (roll_chance(25)) ptr_str += "const ";
				if (roll_chance(15)) ptr_str += "volatile ";
			}
			current = ptr_str + current;
		}

		// 2. Add Array Suffixes (e.g., [10][20])
		bool has_array = false;
		if (roll_chance(40))
		{
			// MINIMAL RULE: If we have prefix pointers AND postfix arrays, 
			// parentheses are strictly REQUIRED around the pointer group: (*arr)[10]
			if (has_pointers)
			{
				current = "(" + current + ")";
				has_pointers = false; // Outer level no longer exposes unparenthesized pointers
			}

			has_array = true;
			int dims = roll_int(1, 2);
			for (int d = 0; d < dims; ++d)
			{
				current += "[" + std::to_string(roll_int(1, 50)) + "]";
			}
		}

		// 3. Add Function Signature Suffix (e.g., (int, char))
		if (depth < 3 && roll_chance(45))
		{
			// MINIMAL RULE: If we have prefix pointers AND/OR arrays before a function call suffix,
			// parens are strictly REQUIRED: (*func)(args) or ((*func[10]))(args)
			if (has_pointers || has_array)
			{
				current = "(" + current + ")";
			}

			current += "(" + random_param_list(depth + 1) + ")";

			// 4. DEEP MINIMAL NESTING (Function returning function pointer)
			if (depth < 2 && roll_chance(40))
			{
				// FIX: A function returning a function pointer MUST wrap the inner 
				// function in a pointer: (*func(args1))(args2)
				current = "(*" + current + ")";
				current += "(" + random_param_list(depth + 1) + ")";

				// Mark pointers as present for outer recursive layers if any
				has_pointers = true;
			}
		}

		return current;
	}
};
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

void print_discrepancies_stl( std::vector<std::string> generated,
	 std::vector<std::string> found)
{
	std::sort(generated.begin(), generated.end());
	std::sort(found.begin(), found.end());
	std::vector<std::string> missing_in_found;
	std::vector<std::string> extra_in_found;

	// Names in generated but NOT in found
	std::set_difference(generated.begin(), generated.end(),
		found.begin(), found.end(),
		std::back_inserter(missing_in_found));

	// Names in found but NOT in generated
	std::set_difference(found.begin(), found.end(),
		generated.begin(), generated.end(),
		std::back_inserter(extra_in_found));

	// Print results
	std::cout << "--- MISSING IN FOUND (" << missing_in_found.size() << ") ---\n";
	for (const auto& name : missing_in_found) {
		std::cout << "  - " << name << "\n";
	}

	std::cout << "\n--- EXTRA IN FOUND (" << extra_in_found.size() << ") ---\n";
	for (const auto& name : extra_in_found) {
		std::cout << "  + " << name << "\n";
	}
}





typedef int const *Alias_998(int**);

typedef int const(*Alias_999(int**))[24][24];

typedef int const(* const**Alias_1000)[24][24];

typedef int * const**Alias_1001[24][24];

//static_assert(std::is_same_v< Alias_1000, Alias_1001>);


std::string load_file_to_string(const std::string& path) {
	std::ifstream file(path);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Usage in your parser test:
std::string C_PARSER_TEST_CASES = load_file_to_string(R"(C:\Users\user\source\repos\DTC\source\main\_example.c)");

 int testparse()
{
	CTypedefGenerator generator;

	auto gc = generator.generate_batch(10);

	bool compare = 0;
	if (false) {//use generated
		compare = 1;
		std::cout << " ----- GENERATED -----" << std::endl;

		std::cout << gc << std::endl;

		std::cout << " ----- END GENERATED -----" << std::endl;
	}
	else
	{
		gc = C_PARSER_TEST_CASES;

		auto pa = gc.find("//// BEFORE COMPILE END"), pb= gc.find("//// AFTER COMPILE");

		gc = gc.substr(pa,pb-pa);
	}

	Compiler c;

	c.compile(gc);

	std::cout << "ENDLINE\n" << std::endl;

	std::cout << c.get_result() << std::endl;

	return 0;
}
auto c = testparse();
// END DEBUG //

void cli_run(const std::vector<std::string>& arguments)
{
}
