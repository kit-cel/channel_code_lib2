/**
 * @file PCM_based_encoder.h
 * @brief Implements efficient and systematic encoding based on the PCM

 */

#ifndef PCM_based_Encoder_H
#define PCM_based_Encoder_H

#include <channel_coding_lib/Encoder/Encoder.h>

struct PCM_Encoder_config : Encoder_config
{
    PCM_Encoder_config(GF2::Matrix H, size_t k, size_t n) : H(H), Encoder_config(k, n)
    {
        encoder_base_type = "pcm_based";
    };
    GF2::Matrix H;
};

class PCM_based_Encoder : public Encoder
{
private:
public:
    void init(const Encoder_config &config) override;

    size_t m;       // Number of rows in parity check matrix
    size_t min_col; // Marks the last column corresponding to an punctured message bit

    // Equivalent parity check matrix in approximate lower triangular (ALT) form   H_ALT = | A | B | T |
    //                                                                                     | C | D | E |
    // A is (m-g)x(n-m),  B is (m-g)xg,   T is (m-g)x(m-g),
    // C is gx(n-m),      D is gxg,       E is gx(m-g)

    GF2::Matrix H_ALT;  // Equivalent parity check matrix in ALT form
    GF2::Matrix H_SALT; // Equivalent parity check matrix in systematic ALT form
    size_t g;           // Gap of ALT form

    // std::vector<size_t> ALT_perm;     // Column permutation from original H to H_ALT
    // std::vector<size_t> inv_ALT_perm; // Column permutation from H_ALT to original H

    GF2::Matrix T_inv; // Inverse of lower triangular submatrix T
    GF2::Matrix E;

    // Pre-computed matrices for encoding
    GF2::Matrix PreComp;
    GF2::Matrix TA;
    GF2::Matrix TB;

    // ALT algorithms
    void ALT_deterministic();

    // Preprocessing
    void get_ALT();
    void get_SALT();
    void rank_reduction();
    void remove_singularity();
    void preprocess();

    GF2::Vector encode(const GF2::Vector &s) override;
};

#endif // PCM_based_Encoder_H
