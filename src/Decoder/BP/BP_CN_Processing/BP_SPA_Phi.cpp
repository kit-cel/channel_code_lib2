#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_SPA_Phi.h>

/**
 * @brief Computes LLRs of all outgoing messages from check node c(j), using Phi-based SPA
 *
 * For every variable node v(i_c) connected to the check node c(j), this method
 * processes the LLR of a message going from c(j) to v(i_c), which is stored in
 * BP_Decoder::LLR_c2v.
 *
 * @param j Index of check node
 */
void BP_SPA_Phi::check_node_update(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set of v(i_c)

    // Form sum and XOR-sum including all ingoing messages to c(j)

    // TODO
    //  IF affine offset empty; set XOR to false
    bool XOR = BP.affine_offset[j];
    double sum = 0.0;

    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        XOR ^= std::signbit(BP.LLR_v2c[i][j_v]);
        sum += safe::phi_function(std::abs(BP.LLR_v2c[i][j_v]));
    }

    // Compute LLRs of all outgoing messages from c(j)
    double sign = 1.0;
    double argument;
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];

        // Exclude ingoing message along same edge from sum and XOR-sum
        sign = 1.0 - 2.0 * static_cast<double>(XOR ^ std::signbit(BP.LLR_v2c[i][j_v]));
        argument = sum - safe::phi_function(std::abs(BP.LLR_v2c[i][j_v]));
        BP.LLR_c2v[j][i_c] = BP.norm_factor * sign * safe::phi_function(argument);
    }
}
