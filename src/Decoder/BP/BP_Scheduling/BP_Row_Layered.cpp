#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Row_Layered.h>

void BP_Row_Layered::init(const BP_config &config)
{
    this->schedule = config.schedule;
    this->nodes_in_layer = config.nodes_in_layer;
}

/**
 * @brief Computes LLRs of all ingoing messages to check node c(j)
 *
 * For every variable node v(i_c) connected to the check node c(j), this method
 * processesthe LLR of a message going from v(i_c) to c(j), which is stored in
 * BP_Decoder::LLR_v2c.
 *
 * @param j Index of check node
 */
void BP_Row_Layered::variable_node_update(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set of v(i_c)

    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        BP.LLR_v2c[i][j_v] = BP.LLR_total[i] - BP.LLR_c2v[j][i_c];
    }
}

/**
 * @brief Computes a-posteriori LLR of all variables nodes connected to check node c(j)
 *
 * For every variable node v(i_c) connected to the check node c(j), this method processes
 * the a-posteriori LLR of v(i_c), which is stored in BP_Decoder::LLR_total and
 * corresponds to the LLR of the i-th bit.
 *
 * @param j Index of check node
 */
void BP_Row_Layered::compute_total_LLR(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set v(i_c)

    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        BP.LLR_total[i] = BP.LLR_v2c[i][j_v] + BP.LLR_c2v[j][i_c];
    }
}

/**
 * @brief Decodes received LLR vector, using row-layered scheduling
 *
 * @param LLR_received Received channel LLRs
 */
void BP_Row_Layered::decode(const std::vector<double> &LLR_received)
{
    BP.LLR_received = LLR_received;

    // Init codeword as hard-decision of received LLRs
    for (size_t i = 0; i < BP.n; ++i)
        BP.codeword[i] = (BP.LLR_received[i] < 0) ? 1 : 0;

    // Exit method if initial hard-decision provides valid codeword
    if (BP.syndrome_check())
        return;

    // Init a-posteriori LLRs to received LLRs
    for (size_t i = 0; i < BP.n; ++i)
        BP.LLR_total[i] = BP.LLR_received[i];

    // Init LLRs of variable to check messages to received LLRs
    for (size_t i = 0; i < BP.n; ++i)
        for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
            BP.LLR_v2c[i][j_v] = 0.0;

    // Init LLRs of check to variable messages to zero
    for (size_t j = 0; j < BP.m; ++j)
        for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
            BP.LLR_c2v[j][i_c] = 0.0;

    // Iterative decoding:
    BP.iteration_count = 1;
    BP.bp_converged = false;
    while (BP.iteration_count <= BP.max_iterations)
    {
        // Iterate over layers in schedule...
        for (size_t layer : schedule)
        {
            // Iterate over check nodes c(j) in layer...
            for (size_t j : nodes_in_layer[layer])
            {
                variable_node_update(j);

                BP.check_node_update(j);

                compute_total_LLR(j);
            }
        }

        // Update codeword as hard-decision of a-posteriori LLRs
        for (size_t i = 0; i < BP.n; ++i)
            BP.codeword[i] = (BP.LLR_total[i] < 0 ? 1 : 0);

        // Stop decoding if codeword is valid
        if (BP.early_stopping == true)
            if (BP.syndrome_check())
            {
                BP.bp_converged = true;
                break;
            }

        BP.iteration_count++;
    }
    if (BP.syndrome_check())
        BP.bp_converged = true;
}
