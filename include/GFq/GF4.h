#pragma once
#include <cstdint>
#include <stdexcept>

namespace GF4 {

// GF(4) symbol stored as 2 bits: values 0..3
using Symbol = uint8_t;

// Addition in GF(2^m) is XOR
inline Symbol add(Symbol a, Symbol b) {
    return static_cast<Symbol>((a ^ b) & 0x3);
}

// Multiplication table for GF(4) with primitive poly p(x)=x^2 + x + 1
// Elements: 0, 1, α, α+1  -> 0, 1, 2, 3
inline Symbol mul(Symbol a, Symbol b) {
    static constexpr Symbol T[4][4] = {
        {0,0,0,0},
        {0,1,2,3},
        {0,2,3,1},
        {0,3,1,2}
    };
    return T[a & 0x3][b & 0x3];
}

// Inverse table in GF(4): inv(1)=1, inv(2)=3, inv(3)=2
inline Symbol inv(Symbol a) {
    a &= 0x3;
    if (a == 0) throw std::invalid_argument("GF4: 0 has no inverse");
    static constexpr Symbol invT[4] = {0,1,3,2};
    return invT[a];
}

inline Symbol div(Symbol a, Symbol b) {
    return mul(a, inv(b));
}

inline bool is_valid(Symbol a) {
    return (a & 0xFC) == 0; // only 2-bit values
}

} // namespace GF4