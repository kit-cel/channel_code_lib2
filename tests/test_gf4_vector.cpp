#include <cassert>
#include <iostream>

#include <GFq/GF4.h>
#include <GFq/Vector4.h>

int main() {
    using namespace GF4;

    Vector4 a(5), b(5);

    a.zeros();
    b.zeros();

    // set a = [1 2 3 0 1]
    a.set(0, 1);
    a.set(1, 2);
    a.set(2, 3);
    a.set(3, 0);
    a.set(4, 1);

    // set b = [3 1 2 1 0]
    b.set(0, 3);
    b.set(1, 1);
    b.set(2, 2);
    b.set(3, 1);
    b.set(4, 0);

    // Check addition elementwise XOR
    Vector4 c = a + b;

    // expected XOR:
    // [1^3, 2^1, 3^2, 0^1, 1^0] = [2,3,1,1,1]
    assert(c.get(0) == 2);
    assert(c.get(1) == 3);
    assert(c.get(2) == 1);
    assert(c.get(3) == 1);
    assert(c.get(4) == 1);

    // Dot product sanity (just compute and ensure it's a valid symbol 0..3)
    Symbol d = a.dot(b);
    assert(d < 4);

    std::cout << "a=" << a << "\n";
    std::cout << "b=" << b << "\n";
    std::cout << "a+b=" << c << "\n";
    std::cout << "dot(a,b)=" << int(d) << "\n";

    std::cout << "PASS test_gf4_vector\n";
    return 0;
}
