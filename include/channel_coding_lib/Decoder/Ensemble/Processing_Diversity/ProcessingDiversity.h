// Processing_config.h
#pragma once
#include <GF2/GF2.h>

/**
 * @class Processing_Diversity
 * @brief Interface class of LLR processing methods used for ensemble diversity
 */

struct Processing_config
{
    std::string processing_type = "identity";
    size_t processing_ensemble_size;
    virtual ~Processing_config() = default;
};
class Processing_Diversity
{
protected:
    void ensure_output_shape(
        std::vector<std::vector<double>> &processed_llrs,
        size_t llr_length) const
    {
        if (processed_llrs.size() != processing_ensemble_size)
            processed_llrs.resize(processing_ensemble_size);

        for (auto &llr : processed_llrs)
        {
            if (llr.size() != llr_length)
                llr.resize(llr_length);
        }
    }

public:
    explicit Processing_Diversity() {}
    size_t processing_ensemble_size;

    virtual void init(const Processing_config &config) = 0;

    virtual void get_LLR_variants(std::vector<std::vector<double>> &processed_llr_values, const std::vector<double> &LLR_received) = 0;
    virtual void post_process_codeword_candidates(std::vector<GF2::Vector> &codeword_candidates) = 0;
    virtual ~Processing_Diversity() = default;
    virtual std::unique_ptr<Processing_Diversity> clone() const = 0;
};
