/**
 * @file BP_Decoder.h
 * @brief Implements belief propagation (BP) decoders
 *
 * Besides BP_Decoder, this file also contains the interface classes
 * BP_Scheduling & BP_CN_Processing, which are used to define
 * the specific scheduling & check node processing rule of the BP decoder.
 */

#ifndef BP_DECODER_H
#define BP_DECODER_H

#include <channel_coding_lib/Decoder/Decoder.h>

class BP_Decoder; // Forward declaration (required for interface classes)

/**
 * @struct BP_config
 * @brief Config struct for BP decoding
 */
struct BP_config : Decoder_config
{
    BP_config(GF2::Matrix H) : H(H)
    {
        decoder_base_type = "bp";
        affine_offset = std::vector<int>(H.rows(), 0); // Default: No affine offset
    } // Set base identifier for factory

    // Required non-changeable attributes
    const GF2::Matrix H; // Parity check matrix

    bool early_stopping = true;               // En-/Disables termination of decoding when syndrome is zero
    size_t max_iterations = 32;               // Maximum number of decoding iterations
    std::string cn_update_type = "spa";       // Check node update rule (msa, spa, spa_phi)
    std::string scheduling_type = "flooding"; // Scheduling method (flooding, row_layered, column_layered)

    // Scheduling
    std::vector<size_t> schedule;                    // Order in which layered decoder iterates over nodes
    std::vector<std::vector<size_t>> nodes_in_layer; // Set of nodes for each scheduling layer
    size_t Z;

    std::vector<int> affine_offset; // Optional affine offset for check node processing (e.g., for offset min-sum algorithm)

    // PCMs that use avns (e.g. ssPCMs)
    bool use_avns = false;

    // MSA and normalized SPA
    double norm_factor = 1; // Normalization factor
};

/**
 * @class BP_Scheduling
 * @brief Interface class for scheduling methods of BP decoding
 */
class BP_Scheduling
{
protected:
    BP_Decoder &BP;

public:
    explicit BP_Scheduling(BP_Decoder &BP) : BP(BP) {}

    virtual void init(const BP_config &config) = 0;
    virtual void decode(const std::vector<double> &LLR_received) = 0;
};

/**
 * @class BP_CN_Processing
 * @brief Interface class for check node processing rules of BP decoding
 */
class BP_CN_Processing
{
protected:
    BP_Decoder &BP;

public:
    explicit BP_CN_Processing(BP_Decoder &BP) : BP(BP) {}

    virtual void init(const BP_config &config) = 0;
    virtual void check_node_update(size_t j) = 0;
    // virtu
    // determine
};

/**
 * @class BP_Decoder
 * @brief Decoder subclass for belief propagation (BP) decoding
 *
 * This specialized sublass of Decoder provides belief propagation decoding,
 * supporting various scheduling methods (flooding, row-layering & column-layering)
 * and check node processing rules (Sum-Product Algorithm (SPA), Phi-based SPA &
 * Normalized Min-Sum Algorithm (MSA)).
 */
class BP_Decoder : public Decoder
{
protected:
    // Implementations selected via interfaces
    std::unique_ptr<BP_Scheduling> scheduling_method;
    std::unique_ptr<BP_CN_Processing> cn_processing;

    bool use_avns;

public:
    GF2::Vector affine_offset;
    std::vector<double> llr_augmented; // scratch buffer for AVNs
    size_t transmitted_bits;

    bool early_stopping;   // En-/Disables termination of decoding when syndrome is zero
    size_t max_iterations; // Maximum number of decoding iterations
    double L_c;            // Channel reliability parameter

    // Graph structure
    GF2::Matrix H;                                   // Parity check matrix
    size_t n;                                        // Number of bits in codeword
    size_t m;                                        // Number of parity equations in codeword
    std::vector<std::vector<size_t>> N_sets;         // Connection sets of variable nodes
    std::vector<std::vector<size_t>> M_sets;         // Connection sets of check nodes
    std::vector<std::vector<size_t>> N_sets_indices; // Indices of check nodes in connection sets of variable nodes
    std::vector<std::vector<size_t>> M_sets_indices; // Indices of variable nodes in connection sets of check nodes

    // Log-Likelihood Ratios
    std::vector<std::vector<double>> LLR_v2c; // LLRs of variable to check messages
    std::vector<std::vector<double>> LLR_c2v; // LLRs of check to variable messages
    std::vector<double> LLR_total;            // A-posteriori LLRs

    // Additional data about decoding process
    size_t iteration_count;
    bool bp_converged;

    // statistics
    size_t cummulative_iteration_count;

    // Normalization factor used for normalized MSA or normalized Sum Product
    double norm_factor = 1;

    // Setters for implementations selected via interfaces
    void set_scheduling_method(std::unique_ptr<BP_Scheduling> scheduling_method) { this->scheduling_method = std::move(scheduling_method); }
    void set_cn_processing(std::unique_ptr<BP_CN_Processing> cn_processing) { this->cn_processing = std::move(cn_processing); }

    void init(const Decoder_config &config) override;
    void adapt_to_channel(double L_c) override;
    void decode(const std::vector<double> &LLR_received) override;

    void get_codeword(GF2::Vector &out) const override;

    // Decoding
    virtual void check_node_update(size_t j) { cn_processing->check_node_update(j); }
    bool syndrome_check();
};

//-------------------------------------------------------------------------------------------------
//                                      BP_DECODER_STATS
//-------------------------------------------------------------------------------------------------

/**
 * @struct BP_Decoder_Stats
 * @brief . . .
 */
struct BP_Decoder_Stats : Decoder_Stats
{
    std::map<double, double> avg_iterations; // Average number of BP iterations

    void get(const std::vector<std::unique_ptr<Decoder>> &decoder_set, size_t trials, double snr) override;
    void save(double snr, const std::string &save_dir) override;
};

#endif // BP_DECODER_H
