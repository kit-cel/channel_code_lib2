#include <channel_coding_lib/Encoder/G_based_Encoder.h>

// INIT
void G_based_Encoder::init(const Encoder_config &config)
{
    const auto &g_based_config = static_cast<const G_Encoder_config &>(config);

    k = g_based_config.k;
    this->n = g_based_config.n;

    this->get_systematic_form(g_based_config.G);
}

void G_based_Encoder::get_systematic_form(const GF2::Matrix &G)
{
    // init
    column_permutation.resize(n);
    std::iota(column_permutation.begin(), column_permutation.end(), 0);
    // init
    inv_column_permutation.resize(n);
    std::iota(inv_column_permutation.begin(), inv_column_permutation.end(), 0);

    // check if first k columns form idendity matrix
    if (G.slice(0, k, 0, k).is_identity())
    {
        // if true already in systematic form
        // i.e. G=(I|P) column_ptermutation stays

        P = G.slice(0, k, k, n);
        return;
    }
    // copy G
    auto temp_G = G;

    // Bring matrix in reduced row echelon form.
    // check if true reduced row echelon form.
    temp_G.setRowCanonical();

    // check if first k columns form idendity matrix
    if (temp_G.slice(0, k, 0, k).is_identity())
    {
        // if true already in systematic form
        // i.e. G=(I|P) column_ptermutation stays

        P = temp_G.slice(0, k, k, n);
        return;
    }
    else
    {
        // Iterate to identify independent columns
        // Identify first non-zero element in each column
        for (size_t i = 0; i < k; i++)
        {
            for (size_t j = i; j < n; j++)
            {
                if (temp_G(i, j)) // true if non-zero
                {
                    if (i == j)
                    {
                        break;
                    }
                    else
                    { // by construction j should be larger than i

                        std::swap(column_permutation[i], column_permutation[j]); // update permutation
                        temp_G.swapCols(i, j);
                        break;
                    }
                }
            }
        }
        // should now be in the form
        // G=(I|P)
        if (temp_G.slice(0, k, 0, k).is_identity())
        {
            P = temp_G.slice(0, k, k, n);
            inv_column_permutation = invPerm(column_permutation);
            return;
        }
        else
        {
            throw std::runtime_error("Failed to compute systematic form of G");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//                                      ENCODING
//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Encode data into a valid codeword
 *
 * Get valid codeword x = (s, p) by generating parity bits
 */
GF2::Vector G_based_Encoder::encode(const GF2::Vector &s)
{
    GF2::Vector codeword = s;

    GF2::Vector p = s * P; // check if p can be seen as row vector

    codeword.append(p);

    codeword.permute(inv_column_permutation);

    return codeword;
}