
#define Type void*

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

typedef unsigned long long*** const*** (*(*TNewPointer))();


typedef unsigned long long **   *const * * * ((TNewPointer2))();

typedef unsigned long long ***const*** TNewPointer3[100][200];


typedef   int const He1, He2, He3;


typedef   int F1(int);

typedef   int const(F2);


typedef Type* Ref$Type;

// ETC

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
} B_t;

int main() {}
