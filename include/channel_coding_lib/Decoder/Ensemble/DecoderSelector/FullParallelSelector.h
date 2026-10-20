#pragma once
#include <GF2/GF2.h>

#include <channel_coding_lib/Decoder/Ensemble/DecoderSelector/DecoderSelector.h>

struct FullParallelSelector_config : DecoderSelector_config
{
    FullParallelSelector_config()
    {
        selector_type = "full_parallel";
    };
    void save(std::fstream &file) const override
    {
        DecoderSelector_config::save(file);
        file << "All paths are executed in parallel" << std::endl;
    };
};

class Ensemble_Decoder; // Forward declaration

/**
 * This is default
 * all decoders run in parallel in the first group
 */

class FullParallelSelector : public DecoderSelector
{
public:
    FullParallelSelector(Ensemble_Decoder &ensemble);

    void init(const DecoderSelector_config &config) override {};
    std::vector<Group> select() override;
};