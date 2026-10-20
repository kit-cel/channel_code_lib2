/**
 * @file Encoder.h
 * @brief Implements efficient encoding
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <GF2/GF2.h>
#include <channel_coding_lib/Utility.h>

#include <vector>
#include <utility>

#include <random>

#include <iostream>

/**
 * @struct Encoder_config
 * @brief Base config struct for all encoders
 */
struct Encoder_config
{
    Encoder_config(size_t k_, size_t n_) : k(k_), n(n_) {};
    std::string encoder_base_type; // Type of decoder, e.g., bp, osd or sc
    size_t k;
    size_t n;
    size_t min_col;

    virtual ~Encoder_config() = default;
};

class Encoder
{
public:
    virtual void init(const Encoder_config &config) = 0;

    // Code parameters
    size_t n; // Number of bits in codeword
    size_t k; // Number of data bits in codeword
    size_t min_col;

    // permutation between the code and its equivalent code, where the latter can be transformed into systematic form
    // should be identiy permutation in most cases
    std::vector<size_t> column_permutation;
    std::vector<size_t> inv_column_permutation;
    // Encoding
    virtual GF2::Vector encode(const GF2::Vector &s) = 0;
};

#endif // ENCODER_H
