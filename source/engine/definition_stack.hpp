#pragma once

using Definition_Time = std::pair<size_t, size_t>;

template<typename Definition>
class Definition_Stack
{
private:

	std::vector<Definition> _definitions;

	std::vector<std::pair<Definition_Time, size_t>> _event_history;

	std::vector<Definition_Time> _definition_times;

public:

	template<typename Forward_Definition>
	void add_definition(Forward_Definition&& definition, size_t version, size_t& counter)
	{
		_definitions.push_back(std::forward<Forward_Definition>(definition));

		Definition_Time time(version, counter);

		_definition_times.push_back(time);

		auto depth = _definitions.size();

		_event_history.emplace_back(time, depth);

		counter++;
	}

	bool remove_definition(size_t version, size_t& counter)
	{
		if (_event_history.empty())
		{
			return false;
		}

		const auto& event = _event_history.back();

		auto depth = event.second;

		if (depth == 0)
		{
			return false;
		}

		Definition_Time time(version, counter);

		auto compare = [depth](const auto& event)
		{
			return event.second < depth;
		};

		auto it = std::partition_point(_event_history.begin(), _event_history.end(), compare);

		if (it == _event_history.begin())
		{
			_event_history.emplace_back(time, 0);
		}
		else
		{
			it--;

			auto previous_depth = it->second;

			_event_history.emplace_back(time, previous_depth);
		}

		counter++;

		return true;
	}

	const Definition* get_definition(Definition_Time& time) const
	{
		auto compare = [time](const auto& event)
		{
			return event.first < time;
		};

		auto it = std::partition_point(_event_history.begin(), _event_history.end(), compare);

		if (it == _event_history.begin())
		{
			return nullptr;
		}

		it--;

		auto depth = it->second;

		if (depth == 0)
		{
			return nullptr;
		}

		time = _definition_times[depth - 1];

		const auto& definition = _definitions[depth - 1];

		return &definition;
	}
};