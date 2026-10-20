#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_SPA.h>

/**
 * @brief Computes LLRs of all outgoing messages from check node c(j), using SPA
 *
 * For every variable node v(i_c) connected to the check node c(j), this method
 * processes the LLR of a message going from c(j) to v(i_c), which is stored in
 * BP_Decoder::LLR_c2v.
 *
 * @param j Index of check node
 */
void BP_SPA::check_node_update(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set of v(i_c)

    // Form product including all ingoing messages to c(j)
    prod_all = 1.0;
    double argument;
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        argument = BP.LLR_v2c[i][j_v] * 0.5;
        prod_all *= safe::tanh(argument);
    }

    // Compute LLRs of all outgoing messages from c(j)
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
        compute_c2v_message(j, i_c);
}

/**
 * @brief Computes LLR of check-to-variable message, using SPA
 *
 * @param j Index of check node
 * @param i_c Index of variable node in connection set of check node
 */
void BP_SPA::compute_c2v_message(size_t j, size_t i_c)
{
    size_t i = BP.M_sets[j][i_c];         // Index of v(i_c) connected to c(j)
    size_t j_v = BP.N_sets_indices[i][j]; // Index of c(j) in connection set of v(i_c)

    // Exclude ingoing message along same edge from product
    BP.LLR_c2v[j][i_c] = BP.norm_factor*(1 - 2 * BP.affine_offset[j]) * 2.0 * safe::atanh(prod_all / safe::tanh(BP.LLR_v2c[i][j_v] * 0.5));
}
