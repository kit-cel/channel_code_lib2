#include <iostream>
#include <random>
#include <cassert>

// Use your project headers (adjust include path if your new structure requires <...>)
#include <GF2/GF2.h>
#include <channel_coding_lib/Encoder.h>

// Create random GF2 message bits
static GF2::Vector random_bits(size_t k) {
    GF2::Vector v(k);
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 1);
    for (size_t i = 0; i < k; ++i) v(i) = dist(rng);
    return v;
}

int main() {
    // -------------------------------
    // Toy parity-check matrix H (m x n)
    // n = 6, k = 3, m = 3
    // -------------------------------
    GF2::Matrix H(3, 6);

    int h[3][6] = {
        {1,0,1,1,0,0},
        {0,1,1,0,1,0},
        {1,1,0,0,0,1}
    };

    for (size_t r = 0; r < 3; ++r)
        for (size_t c = 0; c < 6; ++c)
            H(r, c) = h[r][c];

    size_t n = 6;
    size_t k = 3;

    // min_col is used for punctured systematic bits; keep 0 for this toy test
    Encoder enc(H, k, n, 0);

    // This is the "preprocessing" step you discussed (ALT/SALT + precompute matrices)
    enc.preprocess();

    // Encode a random message
    GF2::Vector u = random_bits(k);
    GF2::Vector cw = enc.encode(u);

    // Verify H * cw^T = 0
    bool ok = H.isOrthogonalTo(cw);
    std::cout << "syndrome zero? " << (ok ? "YES" : "NO") << "\n";
    assert(ok && "Encoded codeword does not satisfy H*c^T=0");

    std::cout << "PASS test_encoder_gf2\n";
    return 0;
}
