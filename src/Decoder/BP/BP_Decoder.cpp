#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>
#include <channel_coding_lib/conversions.h>

/**
 * @brief Initializes BP decoder
 *
 * This method sets up the BP decoder for simulation by initializing vectors and
 * getting neighbouring sets of variable and check nodes from the parity check
 * matrix.
 *
 * @param H Parity check matrix
 */
void BP_Decoder::init(const Decoder_config &config)
{
    const auto &bp_config = static_cast<const BP_config &>(config);

    this->H = bp_config.H;
    n = H.cols();
    m = H.rows();

    this->early_stopping = bp_config.early_stopping;
    this->max_iterations = bp_config.max_iterations;
    this->norm_factor = bp_config.norm_factor;

    this->affine_offset = stdvec_to_gf2_vector(bp_config.affine_offset);

    this->use_avns = bp_config.use_avns;

    if (this->use_avns)
    {
        // prefill scratch buffer for AVNs in decode
        llr_augmented.resize(n, 0.0);
    }
    // alloc_decode_data(1);

    LLR_received.resize(n);
    LLR_total.resize(n);
    codeword.resize(n);

    // Get connection sets of variable and check nodes
    N_sets.resize(n);
    M_sets.resize(m);
    for (size_t j = 0; j < m; ++j)
    {
        for (size_t i = 0; i < n; ++i)
        {
            if (H(j, i) == 1)
            {
                N_sets[i].push_back(j);
                M_sets[j].push_back(i);
            }
        }
    }

    // Init vector for variable to check messages and get indices of check nodes
    // in connection sets of variable nodes
    N_sets_indices.resize(n);
    LLR_v2c.resize(n);
    for (size_t i = 0; i < n; ++i)
    {
        N_sets_indices[i].resize(m);
        size_t ind = 0;
        for (size_t j = 0; j < m; ++j)
        {
            // If check node c(j) & variable node v(i) are connected...
            if (H(j, i) == 1)
            {
                // ...save index of c(j) in connection set of v(i) at position j
                N_sets_indices[i][j] = ind;
                ind++;
            }
            else
                // ...else set index out of bounds
                N_sets_indices[i][j] = m;
        }
        LLR_v2c[i].resize(N_sets[i].size());
    }

    // Init vector for check to variable messages and get indices of variable nodes
    // in connection sets of check nodes
    M_sets_indices.resize(m);
    LLR_c2v.resize(m);
    for (size_t j = 0; j < m; ++j)
    {
        M_sets_indices[j].resize(n);
        size_t ind = 0;
        for (size_t i = 0; i < n; ++i)
        {
            // If check node c(j) & variable node v(i) are connected...
            if (H(j, i) == 1)
            {
                // ...save index of v(i) in connection set of c(j) at position i
                M_sets_indices[j][i] = ind;
                ind++;
            }
            else
                // ...else set index out of bounds
                M_sets_indices[j][i] = n;
        }
        LLR_c2v[j].resize(M_sets[j].size());
    }

    // Init implementations selected via interfaces
    scheduling_method->init(bp_config);
    cn_processing->init(bp_config);

    // init statistics
    cummulative_iteration_count = 0;
}

/**
 * @brief Adapts decoder attributes to channel
 */
void BP_Decoder::adapt_to_channel(double L_c)
{
    this->L_c = L_c;
}

/**
 * @brief Checks whether syndrome is all-zero
 */
bool BP_Decoder::syndrome_check()
{
    auto syndrome = H * codeword;

    return syndrome.isEqualTo(affine_offset);
}

/**
 * @brief Invokes respective decoding of scheduling and zeros to LLR_received if AVNs are used
 */

void BP_Decoder::decode(const std::vector<double> &LLR_received)
{
    transmitted_bits = LLR_received.size();
    if (transmitted_bits > n)
    {
        throw std::logic_error(
            "BP_Decoder::decode: transmitted_bits > n (invalid by design).");
    }
    if (!use_avns) // not using avns or setting use avns but actually transmitted_bits==n
    {
        if (transmitted_bits != n)
        {
            std::cerr << "THROW TRIGGERED: BP decode requires exactly n LLRs unless AVNs are enabled." << std::endl;
            throw std::invalid_argument(
                "BP decode requires exactly n LLRs unless AVNs are enabled.");
        }
        scheduling_method->decode(LLR_received);

        this->decoder_conv_info.converged = this->bp_converged;
        this->decoder_conv_info.decoding_effort = this->iteration_count;
        cummulative_iteration_count += this->iteration_count;

        return;
    } // AVN enabled
    if (transmitted_bits == n)
    {
        // AVNs enabled but not needed: fast path
        scheduling_method->decode(LLR_received);

        this->decoder_conv_info.converged = this->bp_converged;
        this->decoder_conv_info.decoding_effort = this->iteration_count;
        cummulative_iteration_count += this->iteration_count;
        return;
    }
    // AVN case, fill scratch buffer in front with received LLRs
    // end of llr_augmented already prefilled with zeros from init
    // AVN padding case
    std::copy(
        LLR_received.begin(),
        LLR_received.end(),
        llr_augmented.begin()); // requires: transmitted_bits <= n

    scheduling_method->decode(llr_augmented);

    this->decoder_conv_info.converged = this->bp_converged;
    this->decoder_conv_info.decoding_effort = this->iteration_count;
    cummulative_iteration_count += this->iteration_count;
}

/**
 * @brief Returns the decoded codeword of length n.
 *
 * This method guarantees that the returned vector has exactly n bits,
 * regardless of internal decoding mechanisms (e.g., AVNs).
 */

void BP_Decoder::get_codeword(GF2::Vector &out) const
{
    // Sanity check: decode must have been run
    if (codeword.size() == 0)
    {
        throw std::logic_error("BP_Decoder::get_codeword called before decode()");
    }

    // No AVNs or no padding case direct copy
    if (!use_avns || transmitted_bits == n)
    {
        out = codeword; // full-length copy
        return;
    }

    // AVN case: truncate to transmitted length
    if (transmitted_bits > codeword.size())
    {
        throw std::logic_error(
            "BP_Decoder::get_codeword: transmitted_bits > internal codeword size");
    }

    out = codeword.slice(0, transmitted_bits);
}

//-------------------------------------------------------------------------------------------------
//                                      BP_DECODER_STATS
//-------------------------------------------------------------------------------------------------

void BP_Decoder_Stats::get(const std::vector<std::unique_ptr<Decoder>> &decoder_set, size_t trials, double snr)
{
    Decoder_Stats::get(decoder_set, trials, snr);

    // Sum & reset thread-local counters
    size_t iteration_count = 0;
    for (auto &decoder : decoder_set)
    {
        auto &bp_decoder = static_cast<BP_Decoder &>(*decoder.get());
        iteration_count += bp_decoder.cummulative_iteration_count;
        bp_decoder.cummulative_iteration_count = 0;
    }

    // Compute statistics
    avg_iterations[snr] = static_cast<double>(iteration_count) / static_cast<double>(trials);
}

void BP_Decoder_Stats::save(double snr, const std::string &save_dir)
{
    Decoder_Stats::save(snr, save_dir);

    // Save stats in save_dir/decoder_stats.json
    std::string key = double_to_string(snr);
    update_json(save_dir + "/decoder_stats.json", [&](json &stats)
                { stats["avg_iterations"][key] = avg_iterations.at(snr); });
}
