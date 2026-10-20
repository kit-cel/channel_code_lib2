#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Column_Layered.h>

void BP_Column_Layered::init(const BP_config &config)
{
    schedule = config.schedule;
    nodes_in_layer = config.nodes_in_layer;
    cn_memory.resize(BP.m);
}

/**
 * @brief Sets initial value for memory of check node c(j)
 *
 * @param j Index of check node
 */
void BP_Column_Layered::cn_memory_init(size_t j)
{
    size_t i = 0;   // Index of v(i_c) connected to c(j)
    size_t j_v = 0; // Index of c(j) in connection set of v(i_c)

    cn_memory[j] = 1.0;
    for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
    {
        i = BP.M_sets[j][i_c];
        j_v = BP.N_sets_indices[i][j];
        cn_memory[j] *= BP.LLR_v2c[i][j_v];
    }
}

/**
 * @brief Computes LLRs of all ingoing messages to variable node v(i) and it's a-posteriori LLR
 *
 * Combines check node & a-posteriori update in one method (to optimize runtime):
 * For every check node c(j_v) connected to the variable node v(i), this method
 * processes the LLR of a message going from c(j_v) to v(i), which is stored in
 * BP_Decoder::LLR_c2v, and computes the a-posteriori LLR of v(i), stored in
 * BP_Decoder::LLR_total.
 *
 * @param i Index of variable node
 */
void BP_Column_Layered::process_ingoing_messages(size_t i)
{
    size_t j = 0;   // Index of c(j_v) connected to v(i)
    size_t i_c = 0; // Index of v(i) in connection set of c(j_v)

    double sum = 0.0;
    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
    {
        j = BP.N_sets[i][j_v];
        i_c = BP.M_sets_indices[j][i];

        // Compute ingoing message from c(j_v) to v(i)
        BP.LLR_c2v[j][i_c] = cn_memory[j] / BP.LLR_v2c[i][j_v];

        // Sum ingoing messages for a-posteriori LLR
        sum += safe::atanh(BP.LLR_c2v[j][i_c]);
    }

    // Compute a-posteriori LLR of v(i)
    BP.LLR_total[i] = BP.LLR_received[i] + 2.0 * sum;
}

/**
 * @brief Computes LLRs of all outgoing messages from variable node v(i)
 *
 * For every check node c(j_v) connected to the variable node v(i), this method
 * processes the LLR of a message going from v(i) to c(j_v), which is stored in
 * BP_Decoder::LLR_v2c.
 *
 * @param i Index of variable node
 */
void BP_Column_Layered::variable_node_update(size_t i)
{
    size_t j = 0;   // Index of c(j_v) connected to v(i)
    size_t i_c = 0; // Index of v(i) in connection set of c(j_v)

    double argument;
    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
    {
        j = BP.N_sets[i][j_v];
        i_c = BP.M_sets_indices[j][i];
        argument = (BP.LLR_total[i] - 2.0 * safe::atanh(BP.LLR_c2v[j][i_c])) * 0.5;
        BP.LLR_v2c[i][j_v] = safe::tanh(argument);
    }
}

/**
 * @brief Computes memories of all check nodes c(j_v) connected to variable node v(i)
 *
 * @param i Index of variable node
 */
void BP_Column_Layered::cn_memory_update(size_t i)
{
    size_t j = 0;   // Index of c(j_v) connected to v(i)
    size_t i_c = 0; // Index of v(i) in connection set of c(j_v)

    for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
    {
        j = BP.N_sets[i][j_v];
        i_c = BP.M_sets_indices[j][i];
        cn_memory[j] = BP.LLR_v2c[i][j_v] * BP.LLR_c2v[j][i_c];
    }
}

/**
 * @brief Decodes received LLR vector, using column-layered scheduling
 *
 * @param LLR_received Received channel LLRs
 */
void BP_Column_Layered::decode(const std::vector<double> &LLR_received)
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

    // Init LLRs of variable to check messages based on received LLRs
    double argument;
    for (size_t i = 0; i < BP.n; ++i)
        for (size_t j_v = 0; j_v < BP.N_sets[i].size(); ++j_v)
        {
            argument = BP.LLR_received[i] * 0.5;
            BP.LLR_v2c[i][j_v] = safe::tanh(argument);
        }

    // Init LLRs of check to variable messages to zero
    for (size_t j = 0; j < BP.m; ++j)
        for (size_t i_c = 0; i_c < BP.M_sets[j].size(); ++i_c)
            BP.LLR_c2v[j][i_c] = 0.0;

    // Init check node memories
    for (size_t j = 0; j < BP.m; ++j)
        cn_memory_init(j);

    // Iterative decoding:
    BP.iteration_count = 1;
    BP.bp_converged = false;
    while (BP.iteration_count <= BP.max_iterations)
    {
        // Iterate over layers in schedule...
        for (size_t layer : schedule)
        {
            // Iterate over variable nodes v(i) in layer...
            for (size_t i : nodes_in_layer[layer])
            {
                process_ingoing_messages(i);

                variable_node_update(i);

                cn_memory_update(i);
            }
        }

        // Update codeword as hard-decision of a-posteriori LLRs
        for (size_t i = 0; i < BP.n; ++i)
            BP.codeword[i] = (BP.LLR_total[i] < 0 ? 1 : 0);

        // Stop decoding if codeword is valid
        if (BP.early_stopping == true)
            if (BP.syndrome_check())
                break;

        BP.iteration_count++;
    }
    if (BP.syndrome_check())
        BP.bp_converged = true;
}
