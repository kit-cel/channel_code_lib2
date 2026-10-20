/**
 * @file Endomorphism_Processing.h
 * @brief Implements endomorphism based processing for ensemble decoding
 */

#ifndef ENDEMORPHISM_PROCESSING_H
#define ENDEMORPHISM_PROCESSING_H

#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/ProcessingDiversity.h>
/**
 * @class Endomorphism_Processing
 * @brief Implementation of endomorphism based processing for ensemble decoding
 *
 * This class provides a Processing_Diversity implementation for endomorphism
 * ensemble decoding, which uses code endomorphisms to alter the noise represenation
 * along each decoding path of the ensemble.
 */
class Endomorphism_Processing : public Processing_Diversity
{
public:
    using Processing_Diversity::Processing_Diversity;

    // Endomorphisms (and their inverses) of a code (for diversity via code endomorphisms)
    std::vector<GF2::Matrix> endomorphisms;           // Set of endomorphisms
    std::vector<GF2::Matrix> reconstruction_matrices; // Set of reconstruction matrices
    std::vector<GF2::Matrix> kernel_matrices;         // set of kernel of endomorphisms

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override;
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Endomorphism_Processing>(*this); }
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override;

    void configure_endomorphisms(std::vector<GF2::Matrix> transformation_matrices,
                                 std::vector<GF2::Matrix> reconstruction_matrices,
                                 std::vector<GF2::Matrix> kernel_matrices);
};

#endif // ENDEMORPHISM_PROCESSING_H