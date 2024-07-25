#include <iostream>
#include "BigInt.h"

int main()
{
	BigInt x(42);
	std::cout << x.toString() << '\n';
}
