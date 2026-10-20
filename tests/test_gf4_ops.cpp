#include <cassert>
#include <iostream>
#include <GFq/GF4.h>

int main() {
    using namespace GF4;

    // XOR-add sanity
    assert(add(1,1) == 0);
    assert(add(2,2) == 0);
    assert(add(3,3) == 0);

    // Multiplicative identity
    for (Symbol a = 0; a < 4; ++a) {
        assert(mul(a, 1) == a);
        assert(mul(1, a) == a);
    }

    // Inverses: a * inv(a) = 1 for a != 0
    for (Symbol a = 1; a < 4; ++a) {
        assert(mul(a, inv(a)) == 1);
    }

    // Distributive law: a(b+c) = ab + ac
    for (Symbol a = 0; a < 4; ++a)
    for (Symbol b = 0; b < 4; ++b)
    for (Symbol c = 0; c < 4; ++c) {
        Symbol left  = mul(a, add(b, c));
        Symbol right = add(mul(a, b), mul(a, c));
        assert(left == right);
    }

    std::cout << "PASS test_gf4_ops\n";
    return 0;
}
