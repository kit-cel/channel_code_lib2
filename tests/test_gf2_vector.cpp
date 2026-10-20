#include <cassert>
#include <iostream>

#include <GF2/Vector.h>

int main() {
    using GF2::Vector;

    // ---- basic size ----
    Vector v(10);
    assert(v.size() == 10);

    // ---- zeros / ones ----
    v.zeros();
    for (size_t i = 0; i < v.size(); ++i) assert(v[i] == 0);

    v.ones();
    for (size_t i = 0; i < v.size(); ++i) assert(v[i] == 1);

    // ---- flipBit ----
    v.zeros();
    v.flipBit(3);
    assert(v[3] == 1);
    v.flipBit(3);
    assert(v[3] == 0);

    // ---- parity ----
    v.zeros();
    assert(v.parity() == 0);
    v.flipBit(0);
    assert(v.parity() == 1);
    v.flipBit(1);
    assert(v.parity() == 0);

    // ---- append + slice ----
    Vector a(4); a.zeros(); a[1] = 1; // 0100
    Vector b(3); b.zeros(); b[0] = 1; // 100
    a.append(b);                      // 0100 100
    assert(a.size() == 7);
    assert(a[1] == 1);
    assert(a[4] == 1);

    Vector s = a.slice(1, 5);         // bits [1..4]
    assert(s.size() == 4);
    assert(s[0] == 1);

    std::cout << "[OK] test_gf2_vector passed\n";
    return 0;
}
