
// g++ -std=c++14 scope_exit_test.cpp -o scope_exit_test

#include <iostream>
#include <type_traits>
#include <typeinfo>

#include "scope_exit.hpp"



namespace mpl
{
	
}


void f1()
{
	std::cout << "========xxx1========\n";
}


struct one1
{
	int operator () ()
	{
		std::cout << "========yyy1========\n";
		return 1;
	}
};


struct except
{
	~except() noexcept(false)
	{
		throw 1;
	}
};


struct except1
{
	except1 (int) noexcept(false) { throw 1; }
	except1 (double) noexcept(true) {}
};


const char* f2(int a, double b, const char* c)
{
	std::cout << "========xxx2========: ";
	std::cout << "a: " << a << ", b: " << b << ", c: " << c << "\n";
	return "";
}


struct one2
{
	int operator () ()
	{
		std::cout << "========yyy2========\n";
		return 2;
	}
};


struct one3
{
	void operator() () { std::cout << "one3::operator()\n"; }
	void operator() () const { std::cout << "one3::operator() const\n"; }
	void operator () (int) { std::cout << "one3::operator(int)\n"; }
};


int main ()
{
	using namespace mpl;
	
	auto se1 = make_scope_exit( [] () { std::cout << "========zzz1========\n"; } );
	auto se2 = make_scope_exit( &f1 );
	auto se3 = make_scope_exit( one1() );
	
	// static_assert fails compilation because std::string is not exception safe at construction
	//auto se4 = make_scope_exit(
	//	[] (std::string) {
	//		std::cout << "========zzz2========\n";
	//	},
	//	""
	//);
	
	// static_assert fails compilation because except is not exception safe at destruction
	//auto se5 = make_scope_exit(
	//	[] () -> except {
	//		std::cout << "========zzz2========\n";
	//		return except();
	//	}
	//);
	auto se6 = make_scope_exit( &f2, 1, 2.2, "qwerty" );
	auto se7 = make_scope_exit( one2() );
	
	// ok, we can construct exception safe except1 from double
	auto se8 = make_scope_exit(
		[] (except1) {
			std::cout << "========zzz2========\n";
		}, 1.1
	);
	
	// static_assert fails compilation because except1 is not exception safe at construction from int
	//auto se9 = make_scope_exit(
	//	[] (except1) {
	//		std::cout << "========zzz2========\n";
	//	}, 1
	//);
	
	auto se10 = make_scope_exit(
		[] (int, double) mutable {
			std::cout << "========zzz2========\n";
		}, 1.1, 123
	);
	
	// if you use overloaded functor, you must specify type of 'operator()' in first template argument
	const one3 o31;
	one3 o32;
	auto se11 = make_scope_exit<void()>( o32 );
	auto se12 = make_scope_exit<void() const>( o32 );
	auto se13 = make_scope_exit<void() const>( one3() );
	auto se14 = make_scope_exit<void()>( one3() );
	auto se15 = make_scope_exit<void() const>( o31 );
	// You can't call non const method for const object
	//auto se16 = make_scope_exit<void()>( o31 );
	
	return 0;
}




