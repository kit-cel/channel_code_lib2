#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Flooding.h>

/**
 * @brief Computes LLRs of all outgoing messages from variable node v(i)
 *
 * For every check node c(j_v) connected to the variable node v(i), this method
 * processes the LLR of a message going from v(i) to c(j_v), which is stored in
 * BP_Decoder::LLR_v2c.
 *
 * @param i Index of variable node
 */
void BP_Flooding::variable_node_update(size_t i)
{
    size_t j = 0;   // Index of c(j_v) connected to v(i)
    size_t i_c = 0; // Index of v(i) in connection set of c(j_v)

    // Sum LLRs of all ingoing messages to v(i)
    sum_all = 0.0;
    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
    {
        j = BP.N_sets[i][j_v];
        i_c = BP.M_sets_indices[j][i];
        sum_all += BP.LLR_c2v[j][i_c];
    }

    // Compute LLRs of all outgoing messages from v(i)
    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
        compute_v2c_message(i, j_v);
}

/**
 * @brief Computes LLR of variable-to-check message
 *
 * @param i Index of variable node
 * @param j_v Index of check node in connection set of variable node
 */
void BP_Flooding::compute_v2c_message(size_t i, size_t j_v)
{
    size_t j = BP.N_sets[i][j_v];         // Index of c(j_v) connected to v(i)
    size_t i_c = BP.M_sets_indices[j][i]; // Index of v(i) in connection set of c(j_v)

    // Exclude ingoing message along same edge from sum
    BP.LLR_v2c[i][j_v] = BP.LLR_received[i] + sum_all - BP.LLR_c2v[j][i_c];
}

/**
 * @brief Computes a-posteriori LLR of variable node v(i)
 *
 * @param i Index of variable node
 */
void BP_Flooding::compute_total_LLR(size_t i)
{
    size_t j = 0;   // Index of c(j_v) connected to v(i)
    size_t i_c = 0; // index of v(i) in connection set of c(j_v)

    // Sum LLRs of all ingoing messages to v(i)
    sum_all = 0.0;
    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
    {
        j = BP.N_sets[i][j_v];
        i_c = BP.M_sets_indices[j][i];
        sum_all += BP.LLR_c2v[j][i_c];
    }

    // Compute a-posteriori LLR of v(i)
    BP.LLR_total[i] = BP.LLR_received[i] + sum_all;
}

/**
 * @brief Decodes received LLR vector, using flooding scheduling
 *
 * @param LLR_received Received channel LLRs
 */
void BP_Flooding::decode(const std::vector<double> &LLR_received)
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
            BP.LLR_v2c[i][j_v] = BP.LLR_received[i];

    // Init LLRs of check to variable messages to zero
    for (size_t j = 0; j < BP.m; ++j)
        for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
            BP.LLR_c2v[j][i_c] = 0.0;

    // Iterative decoding:
    BP.iteration_count = 1;
    BP.bp_converged = false;
    while (BP.iteration_count <= BP.max_iterations)
    {
        // Compute outgoing messages from all check nodes
        for (size_t j = 0; j < BP.m; ++j)
            BP.check_node_update(j);

        // Compute outgoing messages from all variable nodes
        for (size_t i = 0; i < BP.n; ++i)
            variable_node_update(i);

        // Compute a-posteriori LLRs & update codeword as hard-decision
        for (size_t i = 0; i < BP.n; ++i)
        {
            compute_total_LLR(i);
            BP.codeword[i] = (BP.LLR_total[i] < 0) ? 1 : 0;
        }

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
