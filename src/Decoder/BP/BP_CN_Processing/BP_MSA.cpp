#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_MSA.h>

/**
 * @brief Computes LLRs of all outgoing messages from check node c(j), using Min-Sum
 *
 * For every variable node v(i_c) connected to the check node c(j), this method
 * processes the LLR of a message going from c(j) to v(i_c), which is stored in
 * BP_Decoder::LLR_c2v.
 *
 * @param j Index of check node
 */
void BP_MSA::check_node_update(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set of v(i_c)

    // Find the two ingoing messages to c(j) with the smallest absolute LLRs
    // (Second smallest is needed then v(i_c) of smallest LLR is excluded):

    size_t min_i = BP.M_sets[j][0];                 // Index of v(i_c) with smallest LLR
    size_t min_j_v = BP.N_sets_indices[min_i][j];   // Index of c(j) in connection set of v(min_i)
    size_t min2_i = BP.M_sets[j][1];                // Index of v(i_c) with second smallest LLR
    size_t min2_j_v = BP.N_sets_indices[min2_i][j]; // Index of c(j) in connection set of v(min2_i)

    // Initial sort if necessary
    if (std::abs(BP.LLR_v2c[min2_i][min2_j_v]) < std::abs(BP.LLR_v2c[min_i][min_j_v]))
    {
        std::swap(min2_i, min_i);
        std::swap(min2_j_v, min_j_v);
    }

    // Iterate over all remaining v(i_c) to search for smaller LLRs
    for (size_t i_c = 2; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];

        // Update if new smallest LLR found
        if (std::abs(BP.LLR_v2c[i][j_v]) < std::abs(BP.LLR_v2c[min_i][min_j_v]))
        {
            min2_i = min_i;
            min2_j_v = min_j_v;
            min_i = i;
            min_j_v = j_v;
        }
        // Update if new second smallest LLR found
        else if (std::abs(BP.LLR_v2c[i][j_v]) < std::abs(BP.LLR_v2c[min2_i][min2_j_v]))
        {
            min2_i = i;
            min2_j_v = j_v;
        }
    }

    // Form XOR-sum including all ingoing messages to c(j)
    bool XOR = BP.affine_offset[j];
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        XOR ^= std::signbit(BP.LLR_v2c[i][j_v]);
    }

    // Compute LLRs of all outgoing messages from c(j)
    double min_LLR_v2c = 0.0;
    double sign = 1.0;
    // double offset = offset_factor * BP.L_c;
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];

        // Exclude ingoing message along same edge from minimum search and XOR-sum
        min_LLR_v2c = (i == min_i) ? std::abs(BP.LLR_v2c[min2_i][min2_j_v]) : std::abs(BP.LLR_v2c[min_i][min_j_v]);
        sign = 1.0 - 2.0 * static_cast<double>(XOR ^ std::signbit(BP.LLR_v2c[i][j_v]));
        BP.LLR_c2v[j][i_c] = BP.norm_factor * sign * min_LLR_v2c;
        // BP.LLR_c2v[j][i_c] = sign * std::max(min_LLR_v2c - offset, 0.0);
    }
}
