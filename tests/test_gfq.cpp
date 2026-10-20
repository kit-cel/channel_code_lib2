#include <cstdint>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include "gfq.hpp"


using GF4   = channel_coding::GFq<4,   0b111>;
using GF8   = channel_coding::GFq<8,   0b1011>;
using GF64  = channel_coding::GFq<64,  0b1000011>;
using GF256 = channel_coding::GFq<256, 0b100011101>;//x^8 + x^4 + x^3 + x^2 + 1

template <typename GF>
static void test_field()
{
    using elem_t = typename GF::value_type;
    constexpr elem_t q = GF::field_size;

    for (elem_t a = 0; a < q; ++a) {
        REQUIRE(GF::add(a, a) == 0);

        REQUIRE(GF::mul(a, 0) == 0);
        REQUIRE(GF::mul(0, a) == 0);

        REQUIRE(GF::mul(a, 1) == a);
        REQUIRE(GF::mul(1, a) == a);
    }

    for (elem_t a = 1; a < q; ++a) {
        auto ai = GF::inv(a);
        auto prod = GF::mul(a, ai);
        REQUIRE(prod == 1);
    }

    std::cout << "GF(q=" << q << ") passed basic tests.\n";
}

TEST_CASE("GF4 basic field test", "[gfq][gf4]")
{
    test_field<GF4>();
}

TEST_CASE("GF8 basic field test", "[gfq][gf8]")
{
    test_field<GF8>();
}

TEST_CASE("GF64 basic field test", "[gfq][gf64]")
{
    test_field<GF64>();
}

TEST_CASE("GF256 basic field test", "[gfq][gf256]")
{
    test_field<GF256>();
}
