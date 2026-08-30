# Dollar Template Compiler
A generic template and macro pre-compiler that generates C language output from **Dollar Template Language (DTL)** code.

Powerful **DTL** constructs include: generic containers and algorithms, reference counting, dependent names, external polymorphism, and more...

## Features
- **Simplicity**: The only extensions used beyond regular C syntax are `$` characters and pragmas.
- **Flexibility**: Template arguments can be anything: a type, a number, or even a method or field name.
- **Expressiveness**: Nested templates and specializations reach C++-level generality while staying fully explicit.
- **Ergonomics**: Friendly error messages help to find template syntax errors easily.
- **Compatibility**: Any conformant C code can be run through the pre-compiler.

## Usage
`dtc [flags] <output file> <input files...>`

### Additional Flags
- -e : Stop compilation when the first error is detected, and return with exit code 1.
- -l : Emit line directives, so compilation errors map into the original **DTL** files, instead of the generated C code.
- -c : Replace `$` signs with `_` in the output, for compilers that don't support `$` characters within symbols.
### Input Requirements
The input files must be pre-processed, syntactically valid C files, which may include pragmas and line directives.

Template blocks can be defined and instantiated by using `$` characters in identifiers.
### Output Guarantees
The output file contains standard C code, which can be compiled with any C compiler.

## Language Mechanics
### Template System
In **DTL**, template parameters are part of the block's name, which makes instance name generation straightforward (template parameters are just replaced with the given arguments).

The name of each block is the primary symbol that is used when referring to it:
- For struct, union, and typedef templates, it is the introduced type name.
- For function templates, it is the function name.
- For global variable templates, it is the variable name.

Template definitions consist of a base name and the parameter list (including wildcards and specializations):
```c
// a list with a generic item type
typedef struct {
	$Item* items;
	Count item_count;
} List$Item;

// a create method for generic lists
void List$Item$$Create(List$Item* list, Count item_count)
{
	list->items = Allocate$Item(item_count);
	list->item_count = item_count;
}

// a pair with two template parameters
typedef struct {
	$First first;
	$Second second;
} Pair$First$Second;

// a generic print function
void Print$T($T* value)
{
	printf("$T(%p)\n", value);
}

// a print function specialization for strings
void Print$$String(String* string)
{
	printf("%s\n", string->content);
}

// a print function specialization for lists of any type
void Print$$$List$Item(List$Item* list)
{
	for (Index i = 0; i < list->item_count; i++) {
		Print$Item(&list->items[i]);
	}
}

// a print function specialization for integer lists
void Print$$$List$$int(List$int* list)
{
	for (Index i = 0; i < list->item_count; i++) {
		printf("%i\n", list->items[i]);
	}
}
```

Template instantiations consist of a base name and the argument list:
```c
void example()
{
	// an array of 5 integers
	Array$int$5 numbers;

	// a pointer to a read-only integer
	Ref$$Const$int pointer;

	// a generic construct method called with a string-to-float map
	Map$String$float map;
	Construct$$$Map$String$float(&map);
}
```

### Pragmas
Some advanced constructs require token concatenation and other forms of text manipulation that templates aren’t designed to achieve.
To facilitate these use cases, the macro system can be used with the following pragmas:
```c
// define a macro with optional template parameters (no specialization allowed)
#pragma DTC push <base name>[parameter list] [replacement]
// undefine a macro
#pragma DTC pop <base name>
```

A macro can be used by prepending `$` to its name, which works within other symbols as well. Some examples:
```c
#pragma DTC push Concat$X$Y $X$Y
#pragma DTC push Swap$X$Y $$Y$$X

void example()
{
	const char* name = Get$Concat$Na$me(); // expands to: GetName
	Pair$int$float pair = GetPair$Swap$float$int(); // expands to: GetPair$int$float
}
```

A global region can be escaped by pragmas (for code using compiler extensions or `$` characters not intended to be part of a template):
```c
#pragma DTC disable
#include <stdio.h>
const char* string_10$ = "10 dollars";
#pragma DTC enable
```

Templates can be explicitly instantiated (for header declarations or just to organize the generated code):
```c
typedef struct {
	$First first;
	$Second second;
} Pair$First$Second;

// instantiate some concrete pair structures
#pragma DTC instantiate Pair$int$int
#pragma DTC instantiate Pair$float$double

typedef struct {
	int values[2];
} Vector$$64;

typedef struct {
	int values[4];
} Vector$$128;

typedef struct {
	int values[8];
} Vector$$256;

void Vector$$64$$Add(Vector$$64* result, Vector$$64* a, Vector$$64* b)
{
	for (Index i = 0; i < 2; i++) {
		result->values[i] = a->values[i] + b->values[i];
	}
}

void Vector$$128$$Add(Vector$$128* result, Vector$$128* a, Vector$$128* b)
{
	for (Index i = 0; i < 4; i++) {
		result->values[i] = a->values[i] + b->values[i];
	}
}

void Vector$$256$$Add(Vector$$256* result, Vector$$256* a, Vector$$256* b)
{
	for (Index i = 0; i < 8; i++) {
		result->values[i] = a->values[i] + b->values[i];
	}
}

// instantiate all Vector specializations
#pragma DTC instantiate Vector$*

// instantiate all Vector methods
#pragma DTC instantiate Vector$*$*
```

## Examples
Example code for all language features can be found within the `demo` folder, compiled together as a CMake target.

Observe the generated `build.c` file to see how the templates got instantiated.
