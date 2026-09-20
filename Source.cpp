#include "source/source.hpp"

int main1(int argc, char** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

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


#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// #include your headers here: Block, Template_Block, Template_Registry
//
// Assumptions:
//  - Block is default constructible and has assignable members `content` and `name`
//    (name is a view into content).
//  - first element of a par list is (base name, top-level argument count).
//  - element `second`: 0 = generic slot (patterns only), otherwise subtree span.
//
// Model used by the reference (all brute force, no trie):
//  - a type is a tree; node text is '$' * (arity + 1) + name; flattened in prefix order.
//  - pattern: generic node ("$T", leaf) matches any subtree; literal node ("$$int" in a
//    definition, key "$int") matches an equal node with pairwise matching children.
//  - find_special: among matching patterns the one whose literal/generic sequence is
//    lexicographically smallest (literal < generic) wins.
//  - find_specials: wildcards enumerate literal keys only (never generic edges):
//      "$fl*" / "$$$Li*"  prefix wildcard, arity = number of following child elements
//      "$*"               any complete literal subtree
//    a wildcard nested inside a subtree that a generic slot covers yields no output.
//
// Failures are non-fatal; the first 3 per category are printed. Each iteration is seeded
// with its index, so a failing iteration is reproducible.

int main()
{
	using Par = std::pair<std::string_view, std::ptrdiff_t>;

	using Set = std::set<std::string>;

	enum class Kind { literal, generic, wild_prefix, wild_any };

	struct Tree
	{
		Kind kind = Kind::literal;

		std::string name; // literal: name, generic: parameter name, wild_prefix: prefix

		std::vector<Tree> kids;
	};

	struct Elem
	{
		std::string text;

		std::ptrdiff_t span;
	};

	struct Ref_Pattern
	{
		std::size_t id;

		std::string base;

		std::vector<Tree> args;

		std::string name; // full definition text
	};

	// ---- random helpers ----------------------------------------------------

	std::mt19937_64 rng;

	auto rnd = [&](std::size_t n)
	{
		return static_cast<std::size_t>(rng() % n);
	};

	auto chance = [&](std::size_t percent)
	{
		return rnd(100) < percent;
	};

	const std::vector<std::vector<std::string>> names =
	{
		{ "int", "i64", "float", "flo", "double", "dbl", "bool" }, // arity 0
		{ "List", "Lisp", "Vec", "Opt" },                          // arity 1
		{ "Map", "Mat", "Pair" },                                  // arity 2
	};

	const std::vector<std::string> bases = { "foo_", "bar_", "baz_" }; // baz_ is never registered

	auto pick = [&](const std::vector<std::string>& v) -> const std::string&
	{
		return v[rnd(v.size())];
	};

	// ---- tree <-> text -------------------------------------------------------

	auto key_of = [](const Tree& t)
	{
		return std::string(t.kids.size() + 1, '$') + t.name;
	};

	std::function<void(const Tree&, std::vector<Elem>&)> flatten = [&](const Tree& t, std::vector<Elem>& out)
	{
		const std::size_t at = out.size();

		std::string text;

		switch (t.kind)
		{
			case Kind::literal:
				text = key_of(t);
				break;
			case Kind::generic:
				text = "$" + t.name;
				break;
			case Kind::wild_prefix:
				text = std::string(t.kids.empty() ? std::size_t(1) : t.kids.size() + 2, '$') + t.name + "*";
				break;
			case Kind::wild_any:
				text = "$*";
				break;
		}

		out.push_back({ text, 0 });

		for (const auto& kid : t.kids)
		{
			flatten(kid, out);
		}

		out[at].span = (t.kind == Kind::generic) ? 0 : static_cast<std::ptrdiff_t>(out.size() - at);
	};

	std::function<bool(const Tree&)> has_wild = [&](const Tree& t) -> bool
	{
		if (t.kind == Kind::wild_prefix || t.kind == Kind::wild_any)
		{
			return true;
		}

		for (const auto& kid : t.kids)
		{
			if (has_wild(kid))
			{
				return true;
			}
		}

		return false;
	};

	std::function<bool(const Tree&)> all_literal = [&](const Tree& t) -> bool
	{
		if (t.kind != Kind::literal)
		{
			return false;
		}

		for (const auto& kid : t.kids)
		{
			if (all_literal(kid) == false)
			{
				return false;
			}
		}

		return true;
	};

	std::function<std::string(const Tree&)> flat_text = [&](const Tree& t) -> std::string
	{
		std::string s = key_of(t);

		for (const auto& kid : t.kids)
		{
			s += flat_text(kid);
		}

		return s;
	};

	// ---- generators ------------------------------------------------------------

	std::size_t generic_counter = 0;

	std::function<Tree(std::size_t)> gen_concrete = [&](std::size_t depth) -> Tree
	{
		Tree t;

		std::size_t arity = 0;

		if (depth > 0 && chance(45))
		{
			arity = 1 + rnd(2);
		}

		t.name = pick(names[arity]);

		for (std::size_t i = 0; i < arity; ++i)
		{
			t.kids.push_back(gen_concrete(depth - 1));
		}

		return t;
	};

	std::function<Tree(std::size_t)> gen_pattern = [&](std::size_t depth) -> Tree
	{
		Tree t;

		if (chance(35))
		{
			t.kind = Kind::generic;

			t.name = "T" + std::to_string(generic_counter++);

			return t;
		}

		std::size_t arity = 0;

		if (depth > 0 && chance(45))
		{
			arity = 1 + rnd(2);
		}

		t.name = pick(names[arity]);

		for (std::size_t i = 0; i < arity; ++i)
		{
			t.kids.push_back(gen_pattern(depth - 1));
		}

		return t;
	};

	std::function<void(Tree&)> rename_generics = [&](Tree& t)
	{
		if (t.kind == Kind::generic)
		{
			t.name = "U" + std::to_string(generic_counter++);
		}

		for (auto& kid : t.kids)
		{
			rename_generics(kid);
		}
	};

	std::function<Tree(const Tree&)> instantiate = [&](const Tree& p) -> Tree
	{
		if (p.kind == Kind::generic)
		{
			return gen_concrete(2);
		}

		Tree t;

		t.name = p.name;

		for (const auto& kid : p.kids)
		{
			t.kids.push_back(instantiate(kid));
		}

		return t;
	};

	std::function<void(Tree&, std::vector<Tree*>&)> collect = [&](Tree& t, std::vector<Tree*>& out)
	{
		out.push_back(&t);

		for (auto& kid : t.kids)
		{
			collect(kid, out);
		}
	};

	auto mutate = [&](std::vector<Tree>& args)
	{
		std::vector<Tree*> nodes;

		for (auto& arg : args)
		{
			collect(arg, nodes);
		}

		Tree& t = *nodes[rnd(nodes.size())];

		t.name = pick(names[t.kids.size()]);
	};

	std::function<void(Tree&)> add_wildcards = [&](Tree& t)
	{
		if (t.kind == Kind::literal)
		{
			if (chance(12))
			{
				t.kind = Kind::wild_any;

				t.kids.clear();

				return;
			}

			if (chance(20))
			{
				t.kind = Kind::wild_prefix;

				t.name = t.name.substr(0, 1 + rnd(t.name.size()));

				if (chance(10))
				{
					t.name += "z"; // prefix that matches nothing
				}
			}
		}

		for (auto& kid : t.kids)
		{
			add_wildcards(kid);
		}
	};

	// ---- state and reporting -----------------------------------------------------

	std::vector<Ref_Pattern> ref;

	std::map<std::string, std::size_t> first_id; // canonical pattern -> id of first registration

	std::size_t next_id = 1;

	std::size_t iteration = 0;

	std::map<std::string, std::size_t> failures;

	auto report = [&](const std::string& category, const std::string& detail)
	{
		const std::size_t count = ++failures[category];

		if (count > 3)
		{
			return;
		}

		std::printf("FAIL [%s] iteration %zu: %s\n", category.c_str(), iteration, detail.c_str());

		if (count == 1)
		{
			for (const auto& p : ref)
			{
				std::printf("    pattern %zu: %s\n", p.id, p.name.c_str());
			}
		}
	};

	// ---- reference implementation ---------------------------------------------------

	std::function<bool(const Tree&, const Tree&)> matches = [&](const Tree& p, const Tree& q) -> bool
	{
		if (p.kind == Kind::generic)
		{
			return true;
		}

		if (p.name != q.name || p.kids.size() != q.kids.size())
		{
			return false;
		}

		for (std::size_t i = 0; i < p.kids.size(); ++i)
		{
			if (matches(p.kids[i], q.kids[i]) == false)
			{
				return false;
			}
		}

		return true;
	};

	auto expected_find = [&](const std::string& base, const std::vector<Tree>& args, bool& exists) -> const Ref_Pattern*
	{
		const Ref_Pattern* best = nullptr;

		std::vector<int> best_kinds;

		exists = false;

		for (const auto& p : ref)
		{
			if (p.base != base || p.args.size() != args.size())
			{
				continue;
			}

			exists = true;

			bool all = true;

			for (std::size_t i = 0; i < args.size(); ++i)
			{
				if (matches(p.args[i], args[i]) == false)
				{
					all = false;

					break;
				}
			}

			if (all == false)
			{
				continue;
			}

			std::vector<Elem> elems;

			for (const auto& arg : p.args)
			{
				flatten(arg, elems);
			}

			std::vector<int> kinds;

			for (const auto& e : elems)
			{
				kinds.push_back(e.span == 0 ? 1 : 0); // literal (0) beats generic (1)
			}

			if (best == nullptr || kinds < best_kinds)
			{
				best = &p;

				best_kinds = kinds;
			}
		}

		return best;
	};

	std::function<Set(const Tree&, const Tree&)> expand;

	auto cross = [&](const std::string& head, const std::vector<Tree>& pattern_kids, const std::vector<Tree>& query_kids) -> Set
	{
		Set acc = { head };

		for (std::size_t i = 0; i < pattern_kids.size() && acc.empty() == false; ++i)
		{
			const Set sub = expand(pattern_kids[i], query_kids[i]);

			Set next;

			for (const auto& a : acc)
			{
				for (const auto& s : sub)
				{
					next.insert(a + s);
				}
			}

			acc = std::move(next);
		}

		return acc;
	};

	// all strings a query subtree q can produce when aligned with pattern subtree p
	expand = [&](const Tree& p, const Tree& q) -> Set
	{
		switch (q.kind)
		{
			case Kind::literal:

				if (p.kind == Kind::generic)
				{
					if (has_wild(q))
					{
						return {}; // cannot enumerate below a generic slot
					}

					return { flat_text(q) };
				}

				if (p.name != q.name || p.kids.size() != q.kids.size())
				{
					return {};
				}

				return cross(key_of(p), p.kids, q.kids);

			case Kind::wild_prefix:

				if (p.kind != Kind::literal || p.kids.size() != q.kids.size() || p.name.starts_with(q.name) == false)
				{
					return {};
				}

				return cross(key_of(p), p.kids, q.kids);

			case Kind::wild_any:

				if (all_literal(p) == false)
				{
					return {};
				}

				return { flat_text(p) };

			default:

				return {};
		}
	};

	auto expected_specials = [&](const std::string& base, const std::vector<Tree>& args) -> Set
	{
		Set result;

		for (const auto& p : ref)
		{
			if (p.base != base || p.args.size() != args.size())
			{
				continue;
			}

			const Set s = cross(base, p.args, args);

			result.insert(s.begin(), s.end());
		}

		return result;
	};

	auto pattern_key = [&](const std::string& base, const std::vector<Tree>& args)
	{
		std::vector<Elem> elems;

		for (const auto& arg : args)
		{
			flatten(arg, elems);
		}

		std::string key = base + "/" + std::to_string(args.size()) + ":";

		for (const auto& e : elems)
		{
			key += (e.span == 0 ? std::string("@") : e.text) + ",";
		}

		return key;
	};

	// ---- glue to the registry ---------------------------------------------------------

	auto make_pars = [&](const std::string& base, const std::vector<Elem>& elems, std::size_t argc)
	{
		std::vector<Par> pars;

		pars.emplace_back(std::string_view(base), static_cast<std::ptrdiff_t>(argc));

		for (const auto& e : elems)
		{
			pars.emplace_back(std::string_view(e.text), e.span);
		}

		return pars;
	};

	auto describe = [&](const std::string& base, const std::vector<Elem>& elems)
	{
		std::string s = base;

		for (const auto& e : elems)
		{
			s += e.text;
		}

		return s;
	};

	auto join = [&](const Set& set)
	{
		std::string s;

		for (const auto& x : set)
		{
			s += "\n      " + x;
		}

		return s.empty() ? std::string(" (none)") : s;
	};

	auto register_pattern = [&](Template_Registry& registry, const std::string& base, const std::vector<Tree>& args)
	{
		const std::size_t id = next_id++;

		std::vector<Elem> elems;

		for (const auto& arg : args)
		{
			flatten(arg, elems);
		}

		// definition text: literals get one extra '$', generics stay as they are
		std::string name = base;

		std::vector<std::size_t> offsets;

		std::vector<std::size_t> lengths;

		for (const auto& e : elems)
		{
			const std::string token = (e.span == 0) ? e.text : "$" + e.text;

			offsets.push_back(name.size());

			lengths.push_back(token.size());

			name += token;
		}

		const std::string prefix = "int ";

		const std::string suffix = "() { return " + std::to_string(id) + "; }";

		const std::string content = prefix + name + suffix;

		Block block;

		block.content = content;

		{
			const std::string_view whole = block.content;

			block.name = whole.substr(prefix.size(), name.size());
		}

		const std::string_view name_view = block.name;

		std::vector<Par> pars;

		pars.emplace_back(name_view.substr(0, base.size()), static_cast<std::ptrdiff_t>(args.size()));

		for (std::size_t i = 0; i < elems.size(); ++i)
		{
			const std::size_t strip = (elems[i].span == 0) ? 0 : 1; // literal key = token without first '$'

			pars.emplace_back(name_view.substr(offsets[i] + strip, lengths[i] - strip), elems[i].span);
		}

		Template_Block template_block(id, name, block);

		// Template_Block checks
		if (template_block.get_id() != id || template_block.get_name() != name || template_block.get_pattern() != name)
		{
			report("template_block: accessors", name);
		}

		if (template_block.translate(block, std::string_view()).empty() == false)
		{
			report("template_block: translate of empty part", name);
		}

		if (template_block.instantiate("X") != prefix + "X" + suffix || template_block.instantiate(name) != content)
		{
			report("template_block: instantiate", name);
		}

		for (auto& par : pars)
		{
			const std::string_view original = par.first;

			par.first = template_block.translate(block, original);

			const bool same_text = par.first == original;

			const bool same_offset = (par.first.data() - template_block.get_name().data()) == (original.data() - name_view.data());

			if (same_text == false || same_offset == false)
			{
				report("template_block: translate", name + " part '" + std::string(original) + "'");
			}
		}

		// registry
		const std::string key = pattern_key(base, args);

		const auto known = first_id.find(key);

		const Template_Block* previous = registry.add_template_special(pars.begin(), pars.end(), std::move(template_block));

		if (known == first_id.end())
		{
			first_id.emplace(key, id);

			ref.push_back({ id, base, args, name });

			if (previous != nullptr)
			{
				report("add: new pattern reported as duplicate", name);
			}
		}
		else if (previous == nullptr)
		{
			report("add: duplicate not detected", name);
		}
		else if (previous->get_id() != known->second)
		{
			report("add: duplicate returned the wrong block", name);
		}
	};

	auto id_text = [](const Ref_Pattern* p)
	{
		return p != nullptr ? std::to_string(p->id) : std::string("none");
	};

	auto check_find = [&](const Template_Registry& registry, const std::string& base, const std::vector<Tree>& args)
	{
		std::vector<Elem> elems;

		for (const auto& arg : args)
		{
			flatten(arg, elems);
		}

		const auto pars = make_pars(base, elems, args.size());

		const std::string query = describe(base, elems);

		bool want_exists = false;

		const Ref_Pattern* want = expected_find(base, args, want_exists);

		bool exists = false;

		const Template_Block* got = registry.find_special(pars.begin(), pars.end(), exists);

		const std::string got_id = got != nullptr ? std::to_string(got->get_id()) : std::string("none");

		const std::string detail = query + ": expected id " + id_text(want) + ", got id " + got_id;

		if (exists != want_exists)
		{
			report("find_special: template_exists", query);
		}
		else if (got == nullptr && want != nullptr)
		{
			report("find_special: missed match", detail);
		}
		else if (got != nullptr && want == nullptr)
		{
			report("find_special: unexpected match", detail);
		}
		else if (got != nullptr)
		{
			if (got->get_id() != want->id)
			{
				report("find_special: wrong template chosen", detail);
			}
			else if (got->get_pattern() != want->name || got->get_name() != want->name)
			{
				report("find_special: returned block has wrong contents", detail);
			}
			else if (got->instantiate("Q") != "int Q() { return " + std::to_string(want->id) + "; }")
			{
				report("find_special: returned block instantiates wrongly", detail);
			}
		}
	};

	auto check_specials = [&](const Template_Registry& registry, const std::string& base, const std::vector<Tree>& args)
	{
		std::vector<Elem> elems;

		for (const auto& arg : args)
		{
			flatten(arg, elems);
		}

		const auto pars = make_pars(base, elems, args.size());

		const std::string query = describe(base, elems);

		std::vector<std::string> out;

		registry.find_specials(pars.begin(), pars.end(), std::back_inserter(out));

		for (const auto& s : out)
		{
			if (s.find('*') != std::string::npos)
			{
				report("find_specials: wildcard leaked into output", query + " -> " + s);

				return;
			}
		}

		const Set got(out.begin(), out.end());

		const Set want = expected_specials(base, args);

		if (got != want)
		{
			report("find_specials: result mismatch", query + "\n    expected:" + join(want) + "\n    got:" + join(got));
		}
	};

	auto gen_query = [&](std::string& base, std::vector<Tree>& args)
	{
		args.clear();

		const std::size_t mode = rnd(10);

		if (mode < 6 && ref.empty() == false)
		{
			const Ref_Pattern& p = ref[rnd(ref.size())];

			base = p.base;

			for (const auto& arg : p.args)
			{
				args.push_back(instantiate(arg));
			}

			if (mode == 5)
			{
				mutate(args); // near miss
			}
		}
		else
		{
			base = bases[rnd(bases.size())];

			const std::size_t argc = 1 + rnd(3);

			for (std::size_t i = 0; i < argc; ++i)
			{
				args.push_back(gen_concrete(2));
			}
		}
	};

	// ---- main loop -----------------------------------------------------------------------

	for (iteration = 0; iteration < 1500; ++iteration)
	{
		rng.seed(iteration);

		Template_Registry registry;

		ref.clear();

		first_id.clear();

		next_id = 1;

		const std::size_t pattern_count = rnd(40);

		for (std::size_t n = 0; n < pattern_count; ++n)
		{
			std::string base = bases[rnd(2)];

			std::vector<Tree> args;

			generic_counter = 0;

			if (ref.empty() == false && chance(20))
			{
				// duplicate of an existing pattern, with different generic parameter names
				const Ref_Pattern& src = ref[rnd(ref.size())];

				base = src.base;

				args = src.args;

				for (auto& arg : args)
				{
					rename_generics(arg);
				}
			}
			else
			{
				const std::size_t argc = 1 + rnd(3);

				for (std::size_t i = 0; i < argc; ++i)
				{
					args.push_back(gen_pattern(2));
				}
			}

			register_pattern(registry, base, args);
		}

		for (std::size_t n = 0; n < 150; ++n)
		{
			std::string base;

			std::vector<Tree> args;

			gen_query(base, args);

			check_find(registry, base, args);

			check_specials(registry, base, args);

			if (chance(80))
			{
				for (auto& arg : args)
				{
					add_wildcards(arg);
				}
			}

			check_specials(registry, base, args);
		}
	}

	if (failures.empty())
	{
		std::printf("all %zu iterations passed\n", iteration);
	}
	else
	{
		for (const auto& [category, count] : failures)
		{
			std::printf("%zu x [%s]\n", count, category.c_str());
		}
	}

	return failures.empty();
}
