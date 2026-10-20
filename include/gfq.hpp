#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace channel_coding
{

template <std::uint32_t Q, std::uint32_t PrimitivePolynomial>
class GFq
{
public:
    using value_type = std::uint32_t;

    static_assert(Q >= 2, "Q must be at least 2.");
    static_assert((Q & (Q - 1)) == 0, "Q must be a power of 2 for GF(2^m).");

    static constexpr value_type field_size = Q;
    static constexpr value_type order = Q - 1;

private:
    static constexpr value_type compute_m()
    {
        value_type m = 0;
        value_type tmp = Q;
        while (tmp > 1)
        {
            tmp >>= 1;
            ++m;
        }
        return m;
    }

public:
    static constexpr value_type m = compute_m();
    static constexpr value_type mask = Q - 1;

private:
    static_assert((PrimitivePolynomial >> m) == 1,
                  "PrimitivePolynomial must have degree m, i.e. its highest set bit must be at position m.");

    struct Tables
    {
        std::array<value_type, 2 * order> exp{};
        std::array<value_type, Q> log{};
    };

    static constexpr value_type xtime(value_type x)
    {
        // Multiply by alpha (the polynomial indeterminate x)
        x <<= 1;
        if (x & Q)
        {
            x ^= PrimitivePolynomial;
        }
        return x & mask;
    }

    static constexpr Tables build_tables()
    {
        Tables t{};

        // log[0] is undefined mathematically; keep as 0 and guard against use.
        t.log[0] = 0;

        value_type x = 1;
        for (value_type i = 0; i < order; ++i)
        {
            t.exp[i] = x;
            t.log[x] = i;
            x = xtime(x);
        }

        // Duplicate the exponent table so we can index without modulo in multiplication.
        for (value_type i = order; i < 2 * order; ++i)
        {
            t.exp[i] = t.exp[i - order];
        }

        return t;
    }

    static inline constexpr Tables tables_ = build_tables();

public:
    static constexpr bool valid(value_type a)
    {
        return a < Q;
    }

    static constexpr value_type add(value_type a, value_type b)
    {
        return (a ^ b) & mask;
    }

    static constexpr value_type sub(value_type a, value_type b)
    {
        // Same as add in characteristic 2
        return add(a, b);
    }

    static constexpr value_type neg(value_type a)
    {
        // In characteristic 2, -a = a
        return a & mask;
    }

    static constexpr value_type mul(value_type a, value_type b)
    {
        if (a == 0 || b == 0)
        {
            return 0;
        }

        const value_type la = tables_.log[a];
        const value_type lb = tables_.log[b];
        return tables_.exp[la + lb];
    }

    static constexpr value_type inv(value_type a)
    {
        if (a == 0)
        {
            throw std::invalid_argument("GFq::inv(0) is undefined.");
        }

        const value_type la = tables_.log[a];
        return tables_.exp[order - la];
    }

    static constexpr value_type div(value_type a, value_type b)
    {
        if (b == 0)
        {
            throw std::invalid_argument("GFq::div by zero.");
        }
        if (a == 0)
        {
            return 0;
        }

        const value_type la = tables_.log[a];
        const value_type lb = tables_.log[b];
        const value_type idx = (la + order - lb) % order;
        return tables_.exp[idx];
    }

    static constexpr value_type pow(value_type a, std::int32_t n)
    {
        if (n == 0)
        {
            return 1;
        }
        if (a == 0)
        {
            if (n < 0)
            {
                throw std::invalid_argument("GFq::pow(0, negative) is undefined.");
            }
            return 0;
        }

        if (n < 0)
        {
            a = inv(a);
            n = -n;
        }

        const value_type la = tables_.log[a];
        const value_type idx = static_cast<value_type>((static_cast<std::uint64_t>(la) * static_cast<std::uint64_t>(n)) % order);
        return tables_.exp[idx];
    }

    static constexpr value_type alpha_pow(value_type i)
    {
        return tables_.exp[i % order];
    }

    static constexpr value_type log(value_type a)
    {
        if (a == 0)
        {
            throw std::invalid_argument("GFq::log(0) is undefined.");
        }
        return tables_.log[a];
    }

    static constexpr const std::array<value_type, 2 * order>& exp_table()
    {
        return tables_.exp;
    }

    static constexpr const std::array<value_type, Q>& log_table()
    {
        return tables_.log;
    }
};

}