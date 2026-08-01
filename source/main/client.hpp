#pragma once

// DEBUG //

 std::string C_PARSER_TEST_CASES = R"c(


struct _A$Type$$Type2$Type3
{
	int a;
	char b;
};

typedef struct _B$Type
{
	int a;
	struct {
		Type e;
	} f;
	union {
		Type g;
	} h;
} B;

)c";
int testparse()
{
	Compiler c;

	c.compile(C_PARSER_TEST_CASES);

	return 0;
}
auto c = testparse();
// END DEBUG //

void client_run(const std::vector<std::string>& arguments)
{
}
