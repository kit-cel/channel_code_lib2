#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/Automorphism_Processing.h>

void Automorphism_Processing::init(const Processing_config &config)
{

    auto automorphism_config = dynamic_cast<const Automorphism_config *>(&config);
    if (!automorphism_config)
        throw std::invalid_argument("Invalid config type for Automorphism_Processing!");

    permutation_automorphisms = automorphism_config->permutation_automorphisms;
    inverse_permutation_automorphisms = automorphism_config->inverse_permutation_automorphisms;
    processing_ensemble_size = permutation_automorphisms.size();

    if (processing_ensemble_size != inverse_permutation_automorphisms.size())
        throw std::invalid_argument("Permutation and inverse permutation automorphism sizes mismatch.");

    for (size_t dec = 0; dec < processing_ensemble_size; ++dec)
    {
        const auto &perm = permutation_automorphisms[dec];
        const auto &inv = inverse_permutation_automorphisms[dec];

        if (perm.size() != inv.size())
            throw std::invalid_argument("Permutation and inverse permutation sizes mismatch for automorphism " + std::to_string(dec) + ".");

        std::vector<char> seen(perm.size(), 0);
        for (size_t i = 0; i < perm.size(); ++i)
        {
            if (perm[i] >= perm.size() || inv[i] >= inv.size())
                throw std::invalid_argument("Permutation indices out of range for automorphism " + std::to_string(dec) + ".");
            if (inv[perm[i]] != i)
                throw std::invalid_argument("Permutation and inverse pair mismatch for automorphism " + std::to_string(dec) + ".");
            seen[perm[i]] = 1;
        }
        for (size_t i = 0; i < seen.size(); ++i)
        {
            if (!seen[i])
                throw std::invalid_argument("Permutation is not a valid bijection for automorphism " + std::to_string(dec) + ".");
        }
    }
}

void Automorphism_Processing::get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received)
{

    ensure_output_shape(processed_llr_values, LLR_received.size());

    for (size_t dec = 0; dec < processing_ensemble_size; ++dec)
    {
        for (size_t i = 0; i < LLR_received.size(); ++i)
        {
            processed_llr_values[dec][i] = LLR_received[permutation_automorphisms[dec][i]];
        }
    }
}

void Automorphism_Processing::post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates)
{
    for (size_t dec = 0; dec < processing_ensemble_size; ++dec)
    {
        temp_inv_permuteded.resize(codeword_candidates[dec].size());

        if (codeword_candidates[dec].size() != inverse_permutation_automorphisms[dec].size())
            throw std::invalid_argument("Permutation and inverse permutation sizes mismatch for automorphism " + std::to_string(dec) + ".");

        for (size_t i = 0; i < codeword_candidates[dec].size(); ++i)
        {
            temp_inv_permuteded[i] = codeword_candidates[dec][inverse_permutation_automorphisms[dec][i]];
        }

        codeword_candidates[dec] = temp_inv_permuteded;
    }
}
