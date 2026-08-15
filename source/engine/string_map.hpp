#pragma once

struct String_Less
{
	using is_transparent = void;

	bool operator()(std::string_view a, std::string_view b) const
	{
		return a < b;
	}
};

template<typename Second>
struct String_Pair_Less
{
	using is_transparent = void;

	using String_Key = std::pair<std::string, Second>;

	using View_Key = std::pair<std::string_view, Second>;

	bool operator()(const String_Key& a, const String_Key& b) const
	{
		return compare(a, b);
	}

	bool operator()(const String_Key& a, const View_Key& b) const
	{
		return compare(a, b);
	}

	bool operator()(const View_Key& a, const String_Key& b) const
	{
		return compare(a, b);
	}

	bool operator()(const View_Key& a, const View_Key& b) const
	{
		return compare(a, b);
	}

private:

	template<typename A, typename B>
	bool compare(const A& a, const B& b) const
	{
		auto order = a.first.compare(b.first);

		if (order != 0)
		{
			return order < 0;
		}

		return a.second < b.second;
	}
};

template<typename Value>
using String_Map = std::map<std::string, Value, String_Less>;

template<typename Second, typename Value>
using String_Pair_Map = std::map<std::pair<std::string, Second>, Value, String_Pair_Less<Second>>;
