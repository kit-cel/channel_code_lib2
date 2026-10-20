#pragma once
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <iostream>

#include "GFQ/GF4.h"

namespace GF4 {

// Simple reference implementation: store one GF4 symbol per element.
// (Correct first, optimize later.)
class Vector4 {
private:
    std::vector<Symbol> data_;

public:
    explicit Vector4(size_t n = 0) : data_(n, 0) {}

    size_t size() const { return data_.size(); }

    // read
    Symbol get(size_t i) const {
        if (i >= data_.size()) throw std::out_of_range("Vector4 index out of range");
        return data_[i] & 0x3;
    }

    // write
    void set(size_t i, Symbol v) {
        if (i >= data_.size()) throw std::out_of_range("Vector4 index out of range");
        data_[i] = (v & 0x3);
    }

    // Convenience operators
    Symbol operator()(size_t i) const { return get(i); }
    void   operator()(size_t i, Symbol v) { set(i, v); }

    Symbol operator[](size_t i) const { return get(i); }
    Symbol& raw_ref(size_t i) { return data_.at(i); } // only for internal tests/debug

    Vector4& zeros() {
        for (auto &x : data_) x = 0;
        return *this;
    }

    Vector4& ones() {
        for (auto &x : data_) x = 1;
        return *this;
    }

    bool isEqualTo(const Vector4& other) const {
        if (size() != other.size()) return false;
        for (size_t i = 0; i < size(); ++i)
            if ((data_[i] & 0x3) != (other.data_[i] & 0x3)) return false;
        return true;
    }

    // Elementwise addition (XOR)
    Vector4& operator+=(const Vector4& rhs) {
        if (size() != rhs.size()) throw std::invalid_argument("Vector4 sizes must match");
        for (size_t i = 0; i < size(); ++i)
            data_[i] = add(data_[i], rhs.data_[i]);
        return *this;
    }

    friend Vector4 operator+(Vector4 a, const Vector4& b) {
        a += b;
        return a;
    }

    // GF4 dot product: sum_i a_i * b_i
    Symbol dot(const Vector4& rhs) const {
        if (size() != rhs.size()) throw std::invalid_argument("Vector4 sizes must match");
        Symbol acc = 0;
        for (size_t i = 0; i < size(); ++i) {
            acc = add(acc, mul(get(i), rhs.get(i)));
        }
        return acc;
    }

    friend std::ostream& operator<<(std::ostream& os, const Vector4& v) {
        os << "[";
        for (size_t i = 0; i < v.size(); ++i) {
            os << int(v.get(i));
            if (i + 1 < v.size()) os << " ";
        }
        os << "]";
        return os;
    }
};

} // namespace GF4