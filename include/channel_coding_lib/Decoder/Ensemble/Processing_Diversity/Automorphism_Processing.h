/**
 * @file Automorphism_Processing.h
 * @brief Implements automorphism based processing for ensemble decoding
 */

#ifndef AUTOMORPHISM_PROCESSING_H
#define AUTOMORPHISM_PROCESSING_H

#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/ProcessingDiversity.h>

struct Automorphism_config : Processing_config
{
    // Automorphism_config(size_t Z) { processing_type = "aed"; }//on the fly for QC not yet

    Automorphism_config(const std::vector<std::vector<size_t>> &permutation_automorphisms,
                        const std::vector<std::vector<size_t>> &inverse_permutation_automorphisms)
        : permutation_automorphisms(permutation_automorphisms), inverse_permutation_automorphisms(inverse_permutation_automorphisms)
    {
        processing_type = "aed";
    }
    Automorphism_config(const std::vector<std::vector<size_t>> &permutation_automorphisms) : permutation_automorphisms(permutation_automorphisms)
    {
        processing_ensemble_size = permutation_automorphisms.size();
        inverse_permutation_automorphisms.resize(processing_ensemble_size);

        // iterate over all permutations
        for (size_t perm = 0; perm < processing_ensemble_size; ++perm)
        {
            // Compute inverse permutations
            inverse_permutation_automorphisms[perm].resize(permutation_automorphisms[perm].size());
            for (size_t i = 0; i < permutation_automorphisms[perm].size(); ++i)
                inverse_permutation_automorphisms[perm][permutation_automorphisms[perm][i]] = i;
        }
        processing_type = "aed";
    }

    // Permutation automorphisms (and their inverses) of a code (for diversity via code automorphisms)
    std::vector<std::vector<size_t>> permutation_automorphisms;         // Set of permutation automorphisms
    std::vector<std::vector<size_t>> inverse_permutation_automorphisms; // Set of inverse permutation automorphisms
};

/**
 * @class Automorphism_Processing
 * @brief Implementation of automorphism based processing for ensemble decoding
 *
 * This class provides a Processing_Diversity implementation for automorphism
 * ensemble decoding, which uses code automorphisms to alter the noise represenation
 * along each decoding path of the ensemble.
 */
class Automorphism_Processing : public Processing_Diversity
{
protected:
    GF2::Vector temp_inv_permuteded;

public:
    using Processing_Diversity::Processing_Diversity;

    void init(const Processing_config &config) override;

    // Permutation automorphisms (and their inverses) of a code (for diversity via code automorphisms)
    std::vector<std::vector<size_t>> permutation_automorphisms;         // Set of permutation automorphisms
    std::vector<std::vector<size_t>> inverse_permutation_automorphisms; // Set of inverse permutation automorphisms

    void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) override;
    void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) override;

    // Clone method for deep copying of processing diversity implementations (required for correct copying of ensemble decoders with processing diversity)
    std::unique_ptr<Processing_Diversity> clone() const override { return std::make_unique<Automorphism_Processing>(*this); }

    // Configure on the fly for QC codes
    // void configure_permutation_automorphisms(size_t Z);

    // Configure by providing the permutation automorphisms and their inverses directly (for non-QC codes or pre-computed automorphisms)
    // void configure_permutation_automorphisms(std::vector<std::vector<size_t>> permutation_automorphisms);
    // void configure_permutation_automorphisms(std::vector<std::vector<size_t>> permutation_automorphisms,
    //                                          std::vector<std::vector<size_t>> inverse_permutation_automorphisms);
};

#endif // AUTOMORPHISM_PREPROCESSING_H