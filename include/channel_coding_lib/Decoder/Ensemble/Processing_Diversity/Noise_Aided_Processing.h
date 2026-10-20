/**
 * @file Noise_Aided_Processing.h
 * @brief Implements noise-aided processing for ensemble decoding
 */

#ifndef NOISE_AIDED_PROCESSING_H
#define NOISE_AIDED_PROCESSING_H

#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>

/**
 * @class Noise_Aided_Processing
 * @brief Implementation of noise-aided processing for ensemble decoding
 *
 * This class provides a Processing_Diversity implementation for noise-aided
 * ensemble decoding, which alter the noise representation along each decoding
 * path of the ensemble by applying a second layer of artificial noise.
 */
class Noise_Aided_Processing : public Processing_Diversity
{
public:
    using Processing_Diversity::Processing_Diversity;

    double sigma_NED; // Standard deviation of added noise

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override;
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override;

    void configure_noise(double sigma_NED);
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Noise_Aided_Processing>(*this); }
};

#endif // NOISE_AIDED_PROCESSING_H
