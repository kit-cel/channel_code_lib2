/**
 * @file G_based_encoder.h
 * @brief Implements efficient and systematic encoding based on the G

 */

#ifndef G_based_Encoder_H
#define G_based_Encoder_H

#include <channel_coding_lib/Encoder/Encoder.h>

struct G_Encoder_config : Encoder_config
{
    G_Encoder_config(GF2::Matrix G, size_t k, size_t n) : G(G), Encoder_config(k, n)
    {
        encoder_base_type = "g_based";
    };
    GF2::Matrix G;
};

class G_based_Encoder : public Encoder
{
private:
public:
    // Permutation might be required if first k columns of G are not of rank k
    // in this case

    // Systematic G is of the form G_sys=(I|P)
    GF2::Matrix P;
    // GF2::Matrix G;

    void init(const Encoder_config &config) override;
    void get_systematic_form(const GF2::Matrix &G);
    GF2::Vector encode(const GF2::Vector &s) override;
};

#endif // G_based_Encoder_H
