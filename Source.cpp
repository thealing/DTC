#include "source/source.hpp"

int main1(int argc, char** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	//arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\build.c)");
	//arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\demo\main.dtl1.i)");
	//arguments.push_back(R"(C:\Users\user\source\repos\DTC\build\x64-debug\generated\demo\main.dtl2.i)");

	try
	{
		cli_run(arguments);

		return 0;
	}
	catch (int exit_code)
	{
		return exit_code;
	}
}

// Tests for Preprocessor. Not standalone: include the project headers that define
// the string_* helpers, Line_Iterator, Definition_Stack and Preprocessor before this.
//
// Assumptions (the helpers were not shown):
//  - '$' is a word character, string_skip_word_part stops at '$'
//  - par_list looks like "$T$U"; template_replace_macro replaces each "$T" part of the
//    replacement with the argument text
//  - Process_Content is callable as f(std::string&); line numbers are 0-based
// Expected outputs were traced by hand from the code, not run.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main()
{
	struct Error
	{
		std::size_t line;

		std::string base;

		std::string args;
	};

	struct Result
	{
		std::string output;

		std::vector<Error> errors;
	};

	// Definitions are added at increasing counters, like the real pragma processing.
	struct Env
	{
		Macro_Definition_Map macros;

		Quote_Definition_Map quotes;

		std::size_t counter = 0;

		void define(const std::string& name, std::string par_list, std::string replacement, bool replace_content, std::size_t version = 1)
		{
			macros[name].add_definition(Macro_Definition{ std::move(par_list), std::move(replacement), replace_content }, version, counter);
		}

		void undefine(const std::string& name, std::size_t version)
		{
			(void)macros.at(name).remove_definition(version, counter);
		}

		void quote(const std::string& word, std::size_t version)
		{
			quotes[word].add_definition(Quote_Definition{}, version, counter);
		}

		void unquote(const std::string& word, std::size_t version)
		{
			(void)quotes.at(word).remove_definition(version, counter);
		}
	};

	int failures = 0;

	auto check = [&](const std::string& name, const std::string& got, const std::string& want)
	{
		if (got != want)
		{
			std::printf("FAIL %s\n  got:  [%s]\n  want: [%s]\n", name.c_str(), got.c_str(), want.c_str());

			++failures;
		}
	};

	auto check_errors = [&](const std::string& name, const std::vector<Error>& got, const std::vector<Error>& want)
	{
		bool same = got.size() == want.size();

		for (std::size_t i = 0; same && i < got.size(); ++i)
		{
			same = got[i].line == want[i].line && got[i].base == want[i].base && got[i].args == want[i].args;
		}

		if (same == false)
		{
			std::printf("FAIL %s: errors differ (got %zu, want %zu)\n", name.c_str(), got.size(), want.size());

			for (const auto& e : got)
			{
				std::printf("  got: line %zu base [%s] args [%s]\n", e.line, e.base.c_str(), e.args.c_str());
			}

			++failures;
		}
	};

	auto run = [](Env& env, std::string input, std::size_t version)
	{
		Result result;

		result.output = std::move(input);

		Definition_State state{ &env.macros, &env.quotes, version };

		auto print_error = [&result](std::size_t line, std::string_view base, std::string_view args)
		{
			result.errors.push_back({ line, std::string(base), std::string(args) });
		};

		Preprocessor preprocessor(print_error, nullptr, state);

		preprocessor.preprocess(result.output);

		return result;
	};

	auto run_process = [](Env& env, std::string input, std::vector<std::string>& seen)
	{
		Definition_State state{ &env.macros, &env.quotes, 1 };

		auto print_error = [](std::size_t, std::string_view, std::string_view)
		{
		};

		auto process = [&seen](std::string& content)
		{
			seen.push_back(content);

			content += '!';
		};

		Preprocessor preprocessor(print_error, process, state);

		preprocessor.preprocess(input);

		return input;
	};

	// output must match and no errors may be reported
	auto expect = [&](Env& env, const std::string& input, const std::string& want, std::size_t version = 1)
	{
		Result result = run(env, input, version);

		check("[" + input + "] v" + std::to_string(version), result.output, want);

		check_errors("[" + input + "] v" + std::to_string(version), result.errors, {});
	};

	// ---- environment: the pragma chain from the review discussion ---------------

	Env base;

	base.define("A", "", "$$List$int", true);

	base.define("B", "", "ruct", true);

	base.define("C", "", "$$B$A", true);

	base.define("D", "", "$$A", true);  // $$ keeps the inner macro's leading $s

	base.define("D2", "", "$A", true);  // single $ lets the inner macro's leading $s be trimmed

	base.define("E1", "", "$B", false); // content is not rescanned

	base.define("E2", "", "$B", true);  // content is rescanned

	base.define("Pair", "$T$U", "pair$$T$$U", false);

	base.define("Wrap", "$T", "wrap$$T", false);

	// nothing to expand: must come back unchanged, with no errors
	for (const char* s : { "", "plain text", "$", "$$", "$$$", "abc$", "$ $", "$$x", "x$$", "$Unknown", "a$Unknown(b)", "\n$\n", "$B_" })
	{
		expect(base, s, s);
	}

	const std::pair<const char*, const char*> cases[] =
	{
		// plain expansion, word completion, leftover arguments
		{ "$B", "ruct" },
		{ "Const$B", "Construct" },
		{ "Const$B$tail", "Construct$tail" },
		{ "Const$C(&list);", "Construct$$List$int(&list);" },
		{ "$B $A", "ruct List$int" },
		{ "$B\n$A", "ruct\nList$int" },
		{ "f($B)", "f(ruct)" },

		// leading $ trimming depends on the preceding character
		{ "$A", "List$int" },
		{ "x $A", "x List$int" },
		{ "x$A", "x$$List$int" },
		{ "$D", "List$int" },
		{ "x$D", "x$$List$int" },
		{ "$D2", "List$int" },
		{ "x$D2", "xList$int" },

		// replace_content flag
		{ "Const$E1", "Const$B" },
		{ "Const$E2", "Construct" },

		// parameters, template arguments, leftover arguments, macros inside arguments
		{ "$Pair$int$float", "pair$int$float" },
		{ "$Pair$$List$int$float", "pair$$List$int$float" },
		{ "$Pair$int$$List$float", "pair$int$$List$float" },
		{ "$Pair$int$float$extra", "pair$int$float$extra" },
		{ "$Wrap$A", "wrap$$List$int" },
		{ "$Wrap$$List$int", "wrap$$List$int" },
		{ "x$Wrap$int", "xwrap$int" },
	};

	for (const auto& [input, want] : cases)
	{
		expect(base, input, want);
	}

	// ---- errors -----------------------------------------------------------------

	{
		Result r = run(base, "$Pair$int", 1);

		check("missing argument: output", r.output, "$Pair$int");

		check_errors("missing argument", r.errors, { { 0, "Pair", "$int" } });
	}

	{
		Result r = run(base, "$Wrap", 1);

		check("no arguments: output", r.output, "$Wrap");

		check_errors("no arguments", r.errors, { { 0, "Wrap", "" } });
	}

	{
		Result r = run(base, "a\nb\n$Pair$int", 1);

		check("error line: output", r.output, "a\nb\n$Pair$int");

		check_errors("error line", r.errors, { { 2, "Pair", "$int" } });
	}

	{
		Result r = run(base, "$B\n$Pair$int", 1);

		check("replace then error: output", r.output, "ruct\n$Pair$int");

		check_errors("replace then error", r.errors, { { 1, "Pair", "$int" } });
	}

	{
		Result r = run(base, "$Pair$int $B", 1);

		check("error then replace: output", r.output, "$Pair$int ruct");

		check_errors("error then replace", r.errors, { { 0, "Pair", "$int" } });
	}

	// ---- definition versions / removal / redefinition ---------------------------

	{
		Env ver;

		ver.define("X", "", "a", false, 1);

		ver.define("X", "", "b", false, 2);

		ver.define("Y", "", "y", false, 2);

		ver.undefine("X", 3); // reveals "a" again

		ver.undefine("X", 4); // nothing left

		expect(ver, "$X", "$X", 0);
		expect(ver, "$X", "a", 1);
		expect(ver, "$X", "b", 2);
		expect(ver, "$X", "a", 3);
		expect(ver, "$X", "$X", 4);
		expect(ver, "$Y", "$Y", 1);
		expect(ver, "$Y", "y", 2);
	}

	// ---- self reference and redefinition using the previous definition ----------

	{
		Env self;

		self.define("S", "", "pre$S", true); // must not recurse

		self.define("R", "", "1", true);

		self.define("R", "", "[$R]", true);

		self.define("R", "", "<$R>", true);

		expect(self, "$S", "pre$S");
		expect(self, "x$S", "xpre$S");
		expect(self, "$R", "<[1]>");
	}

	// ---- quoted words -----------------------------------------------------------

	{
		Env quoted;

		quoted.define("B", "", "ruct", true);

		quoted.quote("Const$B", 1);

		quoted.unquote("Const$B", 2);

		expect(quoted, "Const$B", "Const$B", 1);
		expect(quoted, "Other$B", "Otherruct", 1);
		expect(quoted, "Const$B Const$B", "Const$B Const$B", 1);
		expect(quoted, "Const$B", "Construct", 2);
	}

	// ---- string_view overload ---------------------------------------------------

	{
		Definition_State state{ &base.macros, &base.quotes, 1 };

		auto print_error = [](std::size_t, std::string_view, std::string_view)
		{
		};

		Preprocessor preprocessor(print_error, nullptr, state);

		std::string_view view = "Const$B";

		std::string buffer = preprocessor.preprocess(view);

		check("view overload: returned buffer", buffer, "Construct");

		check("view overload: view", std::string(view), "Construct"); // fails / trips ASan if NRVO was not applied

		std::string_view untouched = "Const$Unknown";

		std::string empty = preprocessor.preprocess(untouched);

		check("view overload: no change returns empty", empty, "");

		check("view overload: no change leaves view", std::string(untouched), "Const$Unknown");
	}

	// ---- Process_Content callback -----------------------------------------------

	{
		std::vector<std::string> seen;

		std::string out = run_process(base, "Const$B", seen);

		check("process: output", out, "Construct!");

		check("process: call count", std::to_string(seen.size()), "1");

		check("process: content", seen.empty() ? "" : seen[0], "ruct");

		seen.clear();

		out = run_process(base, "$Pair$int$float$extra", seen);

		check("process: params output", out, "pair$int$float$extra!");

		check("process: params call count", std::to_string(seen.size()), "1");

		check("process: params content", seen.empty() ? "" : seen[0], "pair$int$float$extra");
	}

	// ---- random robustness (run under ASan/UBSan) -------------------------------
	// Properties: no crash, error lines never exceed the newline count, and the
	// text is unchanged when no macro is visible (version 0) or none is defined.

	{
		Env fuzz;

		fuzz.define("a", "", "xa", true);

		fuzz.define("B", "$T", "b$T", true);

		fuzz.define("c", "$T$U", "c$T$U", false);

		fuzz.define("d", "", "$a$B", true);

		fuzz.define("e", "", "$$a", false);

		fuzz.quote("a$a", 1);

		Env none;

		std::mt19937 rng(1);

		const std::string alphabet = "$$$aaBcde _\n(";

		for (int i = 0; i < 20000 && failures < 10; ++i)
		{
			std::string input;

			const auto length = rng() % 25;

			for (unsigned j = 0; j < length; ++j)
			{
				input += alphabet[rng() % alphabet.size()];
			}

			const auto newlines = static_cast<std::size_t>(std::count(input.begin(), input.end(), '\n'));

			Result r = run(fuzz, input, 1);

			for (const auto& e : r.errors)
			{
				if (e.line > newlines)
				{
					std::printf("FAIL fuzz: error line %zu > %zu newlines in [%s]\n", e.line, newlines, input.c_str());

					++failures;
				}
			}

			check("fuzz: version 0 sees nothing [" + input + "]", run(fuzz, input, 0).output, input);

			check("fuzz: empty environment [" + input + "]", run(none, input, 1).output, input);
		}
	}

	if (failures == 0)
	{
		std::printf("test_preprocessor: all passed\n");
	}

	return failures == 0;
}
