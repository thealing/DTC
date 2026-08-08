
#define Type void*

//// BEFORE COMPILE

#define TYPE_$object 0

#define $object BANANA

typedef struct _A$Type$$Type2$Type3 _A$int$$Type2$float;

typedef struct BANANA BANANA;

typedef struct _Container Container;

BANANA container_get_$$$Pair$$Array$Int$$$Pair$$Ref$Char$$Ref$Float(Container* container, int index);
BANANA container_get_$BANANA(Container* container, int index);

//// BEFORE COMPILE END

// STRUCT PARSING

struct A { int i; };

struct		  B		{	int    
	
	i; }   

;  

struct C {
	struct C_A {
		int i;
		int j;
	} a;
	struct C_B {
		int i;
		int j;
	} b;
};

typedef struct X {
	int i;
};

typedef struct Y {
	int i;
} Y_t;

struct Z {
	int i;
} a;

struct {
	int i3;
	struct W {
		int i;
		int j;
	};
};

// TYPEDEF PARSING

typedef int**** NewInt;

typedef int***   * NewInt2  [    10]    [2]  ;

typedef void** NewFunction(int, int);

typedef char;

typedef void * * * Function2(int, char);

typedef void * * * Array[10];


//BAD

typedef struct F { double d; } *PF;

typedef struct F *P2;

typedef unsigned char * * (* PNewFn[40][50])(char e(int, float), PF f, int gg[100][300], int (*h)(int, float(float))   );


typedef struct TypedefSt_tag {
	struct {
		int i;
	} j;
	int k[10];
	int (*l)(int);
} *(*(*TypedefSt)[100])(unsigned char (*)(float));

typedef struct TypedefSt2_tag {
	struct {
		int i;
	} j;
	int k[10];
	int (*l)(int);
} (*TypedefSt2)(float);

typedef int (*TypedefSt3)(int, int());

typedef struct TypedefSt4_tag {
	struct {
		int i;
	} j;
	int k[10];
	int (*l)(int);
} const volatile (*TypedefSt4)(float);

typedef struct TypedefSt5_tag {
	struct {
		int i;
	} j;
	int k[10];
	int (*l)(int);
} *(* const volatile*const(*TypedefSt5[100])(float, unsigned(*)(int)))(unsigned char (*)(float));

typedef int* (* const volatile* const(*TypedefSt6[100])(float, unsigned(*)(int)))(unsigned char (*)(float));

typedef int* (* const volatile* const* TypedefSt7[100]) (unsigned char (*)(float));

typedef int* (* const volatile* const* **( *  * (* const volatile* const* TypedefSt8[100]) (unsigned char (*)(float)) )  (int)) (unsigned char (*)(float));



typedef unsigned long long*** const*** (*(*TNewPointer_WRONG))();

typedef unsigned long long*** const*** (**TNewPointer_RIGHT)();


typedef unsigned long long **   *const * * * TNewPointer2();

typedef unsigned long long ***const*** TNewPointer3[100][200];

// DECORATORS
typedef void FuncWithArray(int arr[ 10]);
typedef void FuncWithConstArray(int arr[ 10]);
typedef int(__stdcall* WinApiFunc)(int a, double b);

// ARRAY SPECIAL

typedef int const* Alias_0998(int**);

typedef int const(*Alias_0999(int**))[24][24];

typedef int const(* const** Alias_1000)[24][24];

typedef int* const** Alias_1001[24][24];

typedef int* (*const** Alias_1002)[24][24];

typedef int* (*const** Alias_1003_BAD);

// MULTI

typedef   int const He1, He2, He3;


typedef   int F1(int);

typedef   int const(F2);


typedef Type* Ref$Type;

// FUNCTIONS

void fn1() {}

unsigned long long fn2(int i, volatile char (*c)(float), int e[10])
{
	return 0;
}

unsigned long long (*fn3(int i)) [10]
{
	return 0;
}
unsigned long long   *fn4    (   int i   )   
{
	return 0;
}

unsigned long long* fn_bad_kr(i)
int i;
{
	return 0;
}

 long  fn_bad91(int i);


int(*fn_bad)[10] = {
	0
};
int (*fn_bad2[5])(int) = { 0 };

void (*fn_bad3)(void) = (void(*)(void))0;

int(fn_bad4) = (int)1;

double (*fn_bad5[2][3])(double) = { { 0 } };

struct Point { int x; int y; } fn_bad6 = { 10, 20 };

// Pointer initialized to a compound literal array
int* fn_bad7 = (int[]){ 1, 2, 3, 4, 5 };

// Function pointer assigned using a cast expression
void (*fn_bad8)(int) = (void (*)(int))0;

// Const struct initialized with nested compound literal inside parens
struct Vector { int data[2]; } fn_bad9 = { { 1, 2 } };

// ETC

typedef void (*(*ComplexFunc)())();
typedef char(* const (*(*name)(char))[10])(char);

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
} _B$Type;

_B$gfugfuzgfgfgus globalB;

int main() {}

// SOA <-> AOS

enum
{
	TYPE_APPLE,

	TYPE_ORANGE,

	TYPE_BANANA,

	TYPE_COUNT
};

struct _Container
{
	void** arrays[TYPE_COUNT];
};

struct APPLE { int i; };
struct ORANGE { int i; };
struct BANANA { int i; };

typedef struct _Container Container;

$object* container_get_$object(Container* container, int index)
{
	return container->arrays[TYPE_$object][index];
}

void boo()
{
	container_get_$$$Pair$$Array$Int$$$Pair$$Ref$Char$$Ref$Float(0, 0);

	BANANA b = container_get_$BANANA(0, 0);

	b.i++;

	_A$int$$Type2$float am;
}

// $Int

/*

Zr$$Array$Int

Zr$$$Pair$Float$Double

Zr$$$Pair$$Array$Int$$$Pair$$Ref$Char$$Ref$Float

Pair< Array<Int> , Pair< Ref<Char> , Ref<Float> > >



*/



//// AFTER COMPILE
BANANA container_get_$BANANA(Container* container, int index) {}
BANANA container_get_$$$Pair$$Array$Int$$$Pair$$Ref$Char$$Ref$Float(Container* container, int index) {}
