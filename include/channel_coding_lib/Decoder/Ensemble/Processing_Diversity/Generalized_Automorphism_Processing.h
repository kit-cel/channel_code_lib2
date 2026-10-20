/**
 * @file Generalized_Automorphism_Processing.h
 * @brief Implements generalized automorphism based processing for ensemble decoding
 */

#ifndef GENERALIZED_AUTOMORPHISM_PROCESSING_H
#define GENERALIZED_AUTOMORPHISM_PROCESSING_H

#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>

/**
 * @class Generalized_Automorphism_Processing
 * @brief Implementation of generalized automorphism based processing for ensemble decoding
 *
 * This class provides a Processing_Diversity implementation for generalized automorphism
 * ensemble decoding, which uses code generalized automorphisms to alter noise represenation
 * along each decoding path of the ensemble.
 */
class Generalized_Automorphism_Processing : public Processing_Diversity
{
public:
    using Processing_Diversity::Processing_Diversity;

    // Generalized Automorphisms (and their inverses) of a code (for diversity via code Generalized Automorphisms)
    // std::vector<GF2::Matrix> generalized_automorphisms;         // Set of generalized automorphisms
    // std::vector<GF2::Matrix> inverse_generalized_automorphisms; // Set of inverse generalized automorphisms

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override;
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Generalized_Automorphism_Processing>(*this); }
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override;

    void configure_GeneralizedAutomorphisms(std::vector<GF2::Matrix> generalized_automorphisms,
                                            std::vector<GF2::Matrix> inverse_generalized_automorphisms);
};

#endif // GENERALIZED_AUTOMORPHISM_PROCESSING_H