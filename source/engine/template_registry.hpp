#pragma once
#ifdef _WIN32_DEBUGDIS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

#pragma comment(lib, "dbghelp.lib")

namespace AllocTrace
{
	constexpr size_t MAX_STACKS = 65536;
	constexpr size_t MAX_DEPTH  = 24;

	struct Entry
	{
		void*  stack[MAX_DEPTH];
		uint32_t depth;

		uint64_t count;
		uint64_t bytes;

		bool used;
	};

	// Fixed storage: NEVER allocate from inside the allocation hook.
	static Entry entries[MAX_STACKS];

	static std::atomic<bool> enabled = false;

	// Per-thread reentrancy guard.
	static thread_local bool inside = false;

	static uint64_t hash_stack(void* const* stack, uint32_t depth)
	{
		uint64_t h = 1469598103934665603ull;

		for (uint32_t i = 0; i < depth; ++i)
		{
			uint64_t x =
				reinterpret_cast<uintptr_t>(stack[i]);

			h ^= x;
			h *= 1099511628211ull;
		}

		return h;
	}

	static void record(size_t size)
	{
		if (!enabled.load(std::memory_order_relaxed) || inside)
			return;

		inside = true;

		void* stack[MAX_DEPTH];

		// Skip:
		//   CaptureStackBackTrace
		//   record
		//   operator new
		//
		// Adjust this if you add another wrapper.
		USHORT depth = CaptureStackBackTrace(
			3,
			MAX_DEPTH,
			stack,
			nullptr);

		uint64_t h = hash_stack(stack, depth);

		size_t index = static_cast<size_t>(h % MAX_STACKS);

		// Fixed open-addressing table.
		for (size_t probe = 0; probe < MAX_STACKS; ++probe)
		{
			Entry& e = entries[index];

			if (!e.used)
			{
				e.used = true;
				e.depth = depth;

				for (USHORT i = 0; i < depth; ++i)
					e.stack[i] = stack[i];

				e.count = 1;
				e.bytes = size;

				break;
			}

			if (e.depth == depth)
			{
				bool same = true;

				for (USHORT i = 0; i < depth; ++i)
				{
					if (e.stack[i] != stack[i])
					{
						same = false;
						break;
					}
				}

				if (same)
				{
					++e.count;
					e.bytes += size;
					break;
				}
			}

			index = (index + 1) % MAX_STACKS;
		}

		inside = false;
	}


	// -----------------------------------------------------------------
	// Global new/delete
	// -----------------------------------------------------------------

	void* operator_new(size_t size)
	{
		void* p = std::malloc(size);

		if (p)
			record(size);

		return p;
	}

	void operator_delete(void* p) noexcept
	{
		std::free(p);
	}


	// -----------------------------------------------------------------
	// Enable / disable
	// -----------------------------------------------------------------

	static void enable()
	{
		enabled.store(true, std::memory_order_release);
	}

	static void disable()
	{
		enabled.store(false, std::memory_order_release);
	}


	// -----------------------------------------------------------------
	// Printing
	//
	// IMPORTANT:
	// This happens AFTER tracing is disabled, so allocations caused
	// by symbol lookup / printf / DbgHelp don't get recorded.
	// -----------------------------------------------------------------

	static void print()
	{
		disable();

		HANDLE process = GetCurrentProcess();

		SymSetOptions(
			SYMOPT_LOAD_LINES |
			SYMOPT_UNDNAME |
			SYMOPT_DEFERRED_LOADS);

		if (!SymInitialize(process, nullptr, TRUE))
		{
			std::printf(
				"SymInitialize failed: %lu\n",
				GetLastError());

			return;
		}

		std::printf(
			"\n========== ALLOCATION STACKS ==========\n");

		struct SYMBOL_INFO_BUFFER
		{
			SYMBOL_INFO symbol;
			char name[1024];
		};

		SYMBOL_INFO_BUFFER buffer{};

		std::sort(
			std::begin(entries),
			std::begin(entries) + MAX_STACKS,
			[](const Entry& a, const Entry& b)
			{
				return a.count > b.count;
			});

		for (size_t n = 0; n < MAX_STACKS; ++n)
		{
			Entry& e = entries[n];

			if (!e.used)
				continue;

			std::printf(
				"\n%llu allocs, %llu bytes\n",
				static_cast<unsigned long long>(e.count),
				static_cast<unsigned long long>(e.bytes));

			for (uint32_t i = 0; i < e.depth; ++i)
			{
				DWORD64 address =
					reinterpret_cast<DWORD64>(e.stack[i]);

				buffer.symbol.SizeOfStruct =
					sizeof(SYMBOL_INFO);

				buffer.symbol.MaxNameLen =
					sizeof(buffer.name);

				DWORD64 displacement = 0;

				if (SymFromAddr(
					process,
					address,
					&displacement,
					&buffer.symbol))
				{
					IMAGEHLP_LINE64 line{};

					line.SizeOfStruct =
						sizeof(line);

					DWORD line_displacement = 0;

					if (SymGetLineFromAddr64(
						process,
						address,
						&line_displacement,
						&line))
					{
						std::printf(
							"    %s + 0x%llx  [%s:%lu]\n",
							buffer.symbol.Name,
							static_cast<unsigned long long>(
								displacement),
							line.FileName,
							line.LineNumber);
					}
					else
					{
						std::printf(
							"    %s + 0x%llx\n",
							buffer.symbol.Name,
							static_cast<unsigned long long>(
								displacement));
					}
				}
				else
				{
					std::printf(
						"    0x%p\n",
						e.stack[i]);
				}
			}
		}

		std::printf(
			"\n=======================================\n");

		SymCleanup(process);
	}
}


// ---------------------------------------------------------------------
// Global operators
// ---------------------------------------------------------------------

void* operator new(std::size_t size)
{
	if (void* p = std::malloc(size))
	{
		AllocTrace::record(size);
		return p;
	}

	throw std::bad_alloc();
}

void* operator new[](std::size_t size)
{
	if (void* p = std::malloc(size))
	{
		AllocTrace::record(size);
		return p;
	}

	throw std::bad_alloc();
}

void operator delete(void* p) noexcept
{
	std::free(p);
}

void operator delete[](void* p) noexcept
{
	std::free(p);
}

void operator delete(void* p, std::size_t) noexcept
{
	std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept
{
	std::free(p);
}


// ---------------------------------------------------------------------
// Usage:
//
// AllocTrace::enable();
// compile_everything();
// AllocTrace::print();
// ---------------------------------------------------------------------

#endif
template<
	typename Key,
	typename T,
	typename Compare = std::less<Key>,
	size_t Stack_Size = 6
>
class Stack_Map
{
private:

	struct Stack_Value
	{
		Key first;
		T second;
		Stack_Value(){}
		Stack_Value(auto&& a,auto&& b):first(a),second(b){}
	};

	using Map = std::map<Key, T, Compare>;

	alignas(Stack_Value) unsigned char _stack[sizeof(Stack_Value) * Stack_Size];

	size_t _size = 0;

	std::unique_ptr<Map> _map;

	Stack_Value* stack_data()
	{
		return reinterpret_cast<Stack_Value*>(_stack);
	}

	const Stack_Value* stack_data() const
	{
		return reinterpret_cast<const Stack_Value*>(_stack);
	}

	void destroy_stack()
	{
		for (size_t i = 0; i < _size; ++i)
		{
			stack_data()[i].~Stack_Value();
		}

		_size = 0;
	}

	template<typename K>
	Stack_Value* find_stack(const K& key)
	{
		Compare compare;

		for (size_t i = 0; i < _size; ++i)
		{
			auto& value = stack_data()[i];

			if (!compare(value.first, key) &&
				!compare(key, value.first))
			{
				return &value;
			}
		}

		return nullptr;
	}

	template<typename K>
	const Stack_Value* find_stack(const K& key) const
	{
		Compare compare;

		for (size_t i = 0; i < _size; ++i)
		{
			auto const& value = stack_data()[i];

			if (!compare(value.first, key) &&
				!compare(key, value.first))
			{
				return &value;
			}
		}

		return nullptr;
	}

	void promote()
	{
		auto map = std::make_unique<Map>();

		for (size_t i = 0; i < _size; ++i)
		{
			map->emplace(
				std::move(stack_data()[i].first),
				std::move(stack_data()[i].second)
			);
		}

		destroy_stack();

		_map = std::move(map);
	}

public:

	class iterator
	{
	private:

		struct Reference
		{
			const Key& first;
			T& second;
		};

		Stack_Value* _stack_value = nullptr;

		typename Map::iterator _map_iterator;

		bool _is_map = false;

		mutable std::optional<Reference> _reference;

		iterator(Stack_Value* stack_value)
			: _stack_value(stack_value)
		{
		}

		iterator(typename Map::iterator map_iterator)
			: _map_iterator(map_iterator)
			, _is_map(true)
		{
		}

		friend class Stack_Map;

		Reference& reference() const
		{
			_reference.reset();

			if (_is_map)
			{
				_reference.emplace(
					_map_iterator->first,
					_map_iterator->second
				);
			}
			else
			{
				_reference.emplace(
					_stack_value->first,
					_stack_value->second
				);
			}

			return *_reference;
		}

	public:

		iterator() = default;

		iterator(iterator const& other)
			: _stack_value(other._stack_value)
			, _map_iterator(other._map_iterator)
			, _is_map(other._is_map)
		{
		}

		iterator& operator=(iterator const& other)
		{
			if (this != &other)
			{
				_reference.reset();

				_stack_value = other._stack_value;
				_map_iterator = other._map_iterator;
				_is_map = other._is_map;
			}

			return *this;
		}

		Reference& operator*() const
		{
			return reference();
		}

		Reference* operator->() const
		{
			return &reference();
		}

		bool operator==(iterator const& other) const
		{
			if (_is_map != other._is_map)
			{
				return false;
			}

			if (_is_map)
			{
				return _map_iterator == other._map_iterator;
			}

			return _stack_value == other._stack_value;
		}

		bool operator!=(iterator const& other) const
		{
			return !(*this == other);
		}
	};


	class const_iterator
	{
	private:

		struct Reference
		{
			const Key& first;
			const T& second;
		};

		const Stack_Value* _stack_value = nullptr;

		typename Map::const_iterator _map_iterator;

		bool _is_map = false;

		mutable std::optional<Reference> _reference;

		const_iterator(const Stack_Value* stack_value)
			: _stack_value(stack_value)
		{
		}

		const_iterator(typename Map::const_iterator map_iterator)
			: _map_iterator(map_iterator)
			, _is_map(true)
		{
		}

		friend class Stack_Map;

		Reference& reference() const
		{
			_reference.reset();

			if (_is_map)
			{
				_reference.emplace(
					_map_iterator->first,
					_map_iterator->second
				);
			}
			else
			{
				_reference.emplace(
					_stack_value->first,
					_stack_value->second
				);
			}

			return *_reference;
		}

	public:

		const_iterator() = default;

		const_iterator(const_iterator const& other)
			: _stack_value(other._stack_value)
			, _map_iterator(other._map_iterator)
			, _is_map(other._is_map)
		{
		}

		const_iterator& operator=(const_iterator const& other)
		{
			if (this != &other)
			{
				_reference.reset();

				_stack_value = other._stack_value;
				_map_iterator = other._map_iterator;
				_is_map = other._is_map;
			}

			return *this;
		}

		Reference& operator*() const
		{
			return reference();
		}

		Reference* operator->() const
		{
			return &reference();
		}

		bool operator==(const_iterator const& other) const
		{
			if (_is_map != other._is_map)
			{
				return false;
			}

			if (_is_map)
			{
				return _map_iterator == other._map_iterator;
			}

			return _stack_value == other._stack_value;
		}

		bool operator!=(const_iterator const& other) const
		{
			return !(*this == other);
		}
	};


	struct Emplace_Result
	{
		iterator first;
		bool second;
	};


	Stack_Map() = default;

	~Stack_Map()
	{
		if (!_map)
		{
			destroy_stack();
		}
	}

	Stack_Map(Stack_Map const&) = delete;

	Stack_Map& operator=(Stack_Map const&) = delete;

	Stack_Map(Stack_Map&& other) noexcept
	{
		if (other._map)
		{
			_map = std::move(other._map);
		}
		else
		{
			for (size_t i = 0; i < other._size; ++i)
			{
				new (&stack_data()[i]) Stack_Value{
					std::move(other.stack_data()[i].first),
					std::move(other.stack_data()[i].second)
				};
			}

			_size = other._size;

			other.destroy_stack();
		}
	}

	Stack_Map& operator=(Stack_Map&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		if (_map)
		{
			_map.reset();
		}
		else
		{
			destroy_stack();
		}

		if (other._map)
		{
			_map = std::move(other._map);
		}
		else
		{
			for (size_t i = 0; i < other._size; ++i)
			{
				new (&stack_data()[i]) Stack_Value{
					std::move(other.stack_data()[i].first),
					std::move(other.stack_data()[i].second)
				};
			}

			_size = other._size;

			other.destroy_stack();
		}

		return *this;
	}


	template<typename K, typename V>
	Emplace_Result emplace(K&& key, V&& value)
	{
		if (_map)
		{
			auto result = _map->emplace(
				std::forward<K>(key),
				std::forward<V>(value)
			);

			return {
				iterator(result.first),
				result.second
			};
		}

		if (auto* existing = find_stack(key))
		{
			return {
				iterator(existing),
				false
			};
		}

		if (_size == Stack_Size)
		{
			promote();

			auto result = _map->emplace(
				std::forward<K>(key),
				std::forward<V>(value)
			);

			return {
				iterator(result.first),
				result.second
			};
		}

		auto* entry = &stack_data()[_size];

		new (entry) Stack_Value{
			std::forward<K>(key),
			std::forward<V>(value)
		};

		++_size;

		return {
			iterator(entry),
			true
		};
	}


	template<typename K>
	iterator find(const K& key)
	{
		if (_map)
		{
			return iterator(_map->find(key));
		}

		return iterator(find_stack(key));
	}


	template<typename K>
	const_iterator find(const K& key) const
	{
		if (_map)
		{
			return const_iterator(_map->find(key));
		}

		return const_iterator(find_stack(key));
	}


	iterator end()
	{
		if (_map)
		{
			return iterator(_map->end());
		}

		return iterator(nullptr);
	}


	const_iterator end() const
	{
		if (_map)
		{
			return const_iterator(_map->end());
		}

		return const_iterator(nullptr);
	}


	size_t size() const
	{
		if (_map)
		{
			return _map->size();
		}

		return _size;
	}
};


class Template_Registry
{
private:

	std::deque<Template_Block> _template_blocks;

	std::deque<Stack_Map<std::string_view, size_t>> _trie;

	Stack_Map<std::pair<std::string_view, size_t>, size_t> _trie_map;

public:

	Template_Registry()
	{
	}

	template<typename It, typename Block>
	bool add_template_special(std::string_view base, It par_start, It par_end, Block&& block)
	{
		auto par_count = std::distance(par_start, par_end);

		auto trie_key = std::make_pair(base, par_count);

		auto trie_index = _trie.size();

		auto trie_map_result = _trie_map.emplace(trie_key, trie_index);

		if (trie_map_result.second)
		{
			_trie.emplace_back();
		}
		else
		{
			trie_index = trie_map_result.first->second;
		}

		auto par_it = par_start;

		while (true)
		{
			auto& trie_node = _trie[trie_index];

			auto par = *par_it;

			par_it++;

			if (par_it == par_end)
			{
				auto template_index = _template_blocks.size();

				auto trie_result = trie_node.emplace(par, template_index);

				if (trie_result.second)
				{
					_template_blocks.emplace_back(std::forward<Block>(block));
				}
				
				return trie_result.second;
			}

			trie_index = _trie.size();

			auto trie_result = trie_node.emplace(par, trie_index);

			if (trie_result.second)
			{
				_trie.emplace_back();
			}
			else
			{
				trie_index = trie_result.first->second;
			}
		}
	}

	template<typename It>
	const Template_Block* find_special(std::string_view base, It arg_start, It arg_end) const
	{
		auto arg_count = std::distance(arg_start, arg_end);

		auto trie_key = std::make_pair(base, arg_count);

		auto trie_map_it = _trie_map.find(trie_key);

		if (trie_map_it == _trie_map.end())
		{
			return nullptr;
		}

		auto trie_index = trie_map_it->second;

		auto arg_it = arg_start;
		
		while (true)
		{
			const auto& trie_node = _trie[trie_index];

			auto arg = *arg_it;

			auto trie_it = trie_node.find(arg);

			auto trie_end = trie_node.end();

			if (trie_it == trie_end)
			{
				if (arg == "*")
				{
					return nullptr;
				}

				trie_it = trie_node.find("*");

				if (trie_it == trie_end)
				{
					return nullptr;
				}
			}

			arg_it++;

			if (arg_it == arg_end)
			{
				auto template_index = trie_it->second;

				return &_template_blocks[template_index];
			}

			trie_index = trie_it->second;
		}
	}
};
