#include <GFq/GFqField.h>

GFqField::GFqField(int m, uint32_t prim_poly) : m_(m), prim_poly_(prim_poly) {
    if (m_ <= 0 || m_ > 16) throw std::invalid_argument("m must be in [1..16] for this implementation");
    q_ = 1 << m_;
    build_tables();
}

void GFqField::build_tables() {
    exp_.assign(2 * (q_ - 1), 0);
    log_.assign(q_, -1);

    // generator alpha typically represented as x -> 0b10 (i.e., 2)
    elem_t x = 1; // alpha^0 = 1
    for (int i = 0; i < (q_ - 1); i++) {
        exp_[i] = x;
        log_[x] = i;

        // multiply by alpha (i.e., shift left by 1 in polynomial basis)
        x <<= 1;

        // if overflow beyond m bits, reduce modulo primitive polynomial
        if (x & q_) { // (bit at position m)
            x ^= static_cast<elem_t>(prim_poly_);
        }

        // keep within m bits
        x &= static_cast<elem_t>(q_ - 1);
    }

    // duplicate exp table to avoid mod in index
    for (int i = (q_ - 1); i < 2 * (q_ - 1); i++) {
        exp_[i] = exp_[i - (q_ - 1)];
    }

    log_[0] = -1;
}

GFqField::elem_t GFqField::mul(elem_t a, elem_t b) const {
    if (a == 0 || b == 0) return 0;
    int la = log_[a];
    int lb = log_[b];
    if (la < 0 || lb < 0) throw std::runtime_error("Invalid log table entry (field build issue?)");
    return exp_[la + lb]; // because exp_ is duplicated
}

GFqField::elem_t GFqField::inv(elem_t a) const {
    if (a == 0) throw std::invalid_argument("0 has no multiplicative inverse");
    int la = log_[a];
    // a^{-1} = alpha^{(q-1) - log(a)}
    return exp_[(q_ - 1) - la];
}

GFqField::elem_t GFqField::div(elem_t a, elem_t b) const {
    if (b == 0) throw std::invalid_argument("division by 0");
    if (a == 0) return 0;
    int la = log_[a];
    int lb = log_[b];
    int diff = la - lb;
    if (diff < 0) diff += (q_ - 1);
    return exp_[diff];
}

GFqField::elem_t GFqField::pow(elem_t a, int e) const {
    if (e < 0) return pow(inv(a), -e);
    if (a == 0) return (e == 0) ? 1 : 0;
    int la = log_[a];
    int idx = (la * e) % (q_ - 1);
    return exp_[idx];
}
