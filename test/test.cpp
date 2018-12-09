
#include "../src/boa_core_cpp.h"
#include "../src/boa_core_impl.h"
#include <stdio.h>

int main()
{
	const char *str = boa::format(buf, "Hello %s!", "World");
	puts(str);
}
