#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

class GFqField {
public:
    using elem_t = uint16_t;

    GFqField(int m, uint32_t prim_poly);

    int m() const { return m_; }
    int q() const { return q_; }              // q = 2^m
    int order() const { return q_ - 1; }      // multiplicative group size

    elem_t add(elem_t a, elem_t b) const { return a ^ b; }
    elem_t sub(elem_t a, elem_t b) const { return a ^ b; }

    elem_t mul(elem_t a, elem_t b) const;
    elem_t inv(elem_t a) const;
    elem_t div(elem_t a, elem_t b) const;

    elem_t pow(elem_t a, int e) const;

    const std::vector<elem_t>& exp_table() const { return exp_; }
    const std::vector<int>& log_table() const { return log_; }

private:
    int m_;
    int q_;
    uint32_t prim_poly_;

    std::vector<elem_t> exp_; // size: 2*(q-1) for easy mod
    std::vector<int> log_;    // size: q, log_[0] = -1

    elem_t mul_noLUT(elem_t a, elem_t b) const; // fallback/debug
    void build_tables();
};
