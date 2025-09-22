#include <criterion/criterion.h>

Test(misc, failing) { cr_assert(0, "hello"); }
