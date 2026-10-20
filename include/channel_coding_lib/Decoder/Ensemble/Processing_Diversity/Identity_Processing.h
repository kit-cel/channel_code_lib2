/**
 * @file Identity_Processing.h
 * @brief Implements
 */

#ifndef IDENTITY_PROCESSING_ENSEMBLE_DIVERSITY_H
#define IDENTITY_PROCESSING_ENSEMBLE_DIVERSITY_H

#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/ProcessingDiversity.h>

struct Identity_config : Processing_config
{
    Identity_config()
    {
        processing_type = "identity";
        processing_ensemble_size = 1;
    }
};

/**
 * @class Identity_Processing
 * @brief Implementation of Processing_Diversity that does not alter noise presentation
 *
 * This class provides a Processing_Diversity implementation that does not alter
 * the noise presentation along each decoding path in the ensemble but provides
 * multiple copies of the received LLR vector.
 */
class Identity_Processing_Diversity : public Processing_Diversity
{
public:
    // Identity processing does not require initialization, but method is implemented to satisfy interface
    void init(const Processing_config &config) override { this->processing_ensemble_size = config.processing_ensemble_size; };
    using Processing_Diversity::Processing_Diversity;

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override
    {
        ensure_output_shape(processed_llr_values, LLR_received.size());
        processed_llr_values[0] = LLR_received;
    };
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override {}
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Identity_Processing_Diversity>(*this); }
};

#endif // IDENTITY_PROCESSING_ENSEMBLE_DIVERSITY_H