/**
 * @file Saturation_Processing.h
 * @brief Implements saturation based processing for ensemble decoding
 */

#ifndef SATURATION_PROCESSING_H
#define SATURATION_PROCESSING_H

#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>

/**
 * @class Saturation_processing
 * @brief Implementation of saturation based processing for ensemble decoding
 *
 * This class provides a Processing_Diversity implementation for saturated ensemble
 * decoding, which alters the noise representation along each decoding path of the
 * ensemble by oversaturating the least reliable LLRs and testing all combinations.
 */
class Saturation_Processing : public Processing_Diversity
{
public:
    using Processing_Diversity::Processing_Diversity;

    // Diversity parameters for saturation ensemble decoding):
    size_t S;                // Number of LLRs marked for saturation
    double saturation_value; // Value assigned to LLR of saturated bit

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override;
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Saturation_Processing>(*this); }
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override;

    void configure_saturation(size_t S, double saturation_value);
};

#endif // SATURATION_PREPROCESSING_H
