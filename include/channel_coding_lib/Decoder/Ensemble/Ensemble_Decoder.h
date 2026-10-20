/**
 * @file Ensemble_Decoder.h
 * @brief Implements ensemble decoder base clase
 */

#ifndef ENSEMBLE_DECODER_H
#define ENSEMBLE_DECODER_H

#include <channel_coding_lib/Decoder/Decoder.h>

#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/ProcessingDiversity.h>
#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/Identity_Processing.h>

#include <channel_coding_lib/Decoder/Ensemble/EnsembleUtility/Group.h>

// forward declarations
struct DecoderSelector_config;

class DecoderSelector;

struct Ensemble_config : Decoder_config
{
    Ensemble_config(const GF2::Matrix H, const std::vector<std::shared_ptr<Decoder_config>> config_set);

    Ensemble_config(const GF2::Matrix H, const std::vector<std::shared_ptr<Decoder_config>> config_set, std::shared_ptr<Processing_config> processing_config);

    const GF2::Matrix H; // Parity check matrix used for ML in the List
    // Implementations selected via interfaces

    // Default is full parallel; however handled in processingfactory
    std::shared_ptr<DecoderSelector_config> decoder_selector_config;
    // Optional setters
    void set_selector_config(std::shared_ptr<DecoderSelector_config> cfg);

    // default init is identity config; however handled in processingfactory
    std::shared_ptr<Processing_config> processing_config;

    std::vector<std::shared_ptr<Decoder_config>> dec_config_set;
    void save(std::fstream &file) const override;
};

/**
 * @class Ensemble_Decoder
 * @brief Intermediate class of Decoder for ensemble decoding
 *
 * This specialized subclass of Decoder provides ensemble decoding, using one or
 * more constituent decoder instances. Ensemble diversity can implemented via various
 * LLR processing methods (automorphism, endomorphism, generalized automorphism,
 * noise-aided or saturation based) selected through the Processing_Diversity
 * interface class, and/or by using a set of diverse decoder instances.
 */
class Ensemble_Decoder : public Decoder
{
private:
protected:
    std::unique_ptr<DecoderSelector> selector;

public:
    ~Ensemble_Decoder() override;
    // Implementations selected via interfaces
    std::unique_ptr<Processing_Diversity> processing_diversity;

    std::vector<std::unique_ptr<Decoder>> decoder_set; // Set of non-identical constituent decoders

    // Default constructor
    Ensemble_Decoder(std::vector<std::unique_ptr<Decoder>> decoder_set);
    GF2::Matrix H; // Parity check matrix
    // size_t n;      // Number of bits in codeword

    size_t decoder_ensemble_size; // Number of distinct decoders
    size_t preprocessing_size;    // Number of distinct pre- and postprocessing
    size_t ensemble_size;

    // container to allocate memory for return of get LLR variants by preprocessing
    std::vector<std::vector<double>> processed_llrs;

    // order groups are executed in; members within a group are executed in parallel
    std::vector<Group> group_ordering_of_decoders;
    std::vector<size_t> executed_paths; // all paths with index in group until now were eventually executed; so perhaps obsolete!
    std::vector<size_t> paths_for_ml;
    std::vector<std::vector<size_t>> converged_paths;
    std::vector<std::vector<size_t>> non_converged_paths;

    std::vector<std::vector<ConvergenceInfo>> ensemble_convergence_info;

    // Statistics of Ensemble Decoder (Cummulative per Thread over multiple runs)
    size_t number_active_path_counter;
    size_t number_converged_path_counter;

    size_t ensemble_decoding_effort_counter;
    size_t ensemble_latency_counter;
    std::vector<size_t> path_decoding_effort_counter;

    std::vector<GF2::Vector> codeword_candidates; // Set of codewords provided by distinct decoding paths

    // Setters for implementations selected via interfaces
    void set_processing_diversity(std::unique_ptr<Processing_Diversity> processing_diversity);

    void init(const Decoder_config &config) override;
    void adapt_to_channel(double L_c) override;
    void decode(const std::vector<double> &LLR_received) override;

    void execute_group(Group &group, const std::vector<std::vector<double>> &processed_llrs);

    void update_paths_for_ml(size_t last_group_idx);
    void update_statistics_and_ensemble_effort(size_t last_group_idx);

    void get_codeword(GF2::Vector &out) const override { out = codeword; };
    double compute_ML_metric(const GF2::Vector &codeword, const std::vector<double> &LLR_received);

    void ML_criterion();

    // (STATISTICS)
    std::vector<size_t> num_path_frame_errors;
    std::vector<size_t> num_path_bit_errors;

    // Analysis
    // TODO: Think about how to get data
    // void get_decode_data(size_t dec, size_t dec_instance);
};

//-------------------------------------------------------------------------------------------------
//                                      ENSEMBLE_DECODER_STATS
//-------------------------------------------------------------------------------------------------

/**
 * @struct Ensemble_Decoder_Stats
 * @brief . . .
 */
struct Ensemble_Decoder_Stats : Decoder_Stats
{
    std::map<double, double> avg_number_active_paths;
    std::map<double, double> avg_number_converged_paths;

    std::map<double, double> avg_ensemble_effort;
    std::map<double, double> avg_ensemble_latency;

    std::map<double, std::vector<double>> avg_decoding_effort_per_path; // Eg. average number of BP iterations per path (0 if not BP)

    void get(const std::vector<std::unique_ptr<Decoder>> &multithreading_decoder_set, size_t trials, double epsilon) override;
    void save(double snr, const std::string &save_dir) override;
};

#endif // ENSEMBLE_DECODER_H
