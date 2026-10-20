#pragma once

#include <channel_coding_lib/Utility.h>
#include <channel_coding_lib/Decoder/Ensemble/EnsembleUtility/Group.h>

class Ensemble_Decoder; // Forward declaration (required for interface classes)

struct DecoderSelector_config
{
    std::string selector_type; // Type of decoder selection/grouping strategy, e.g. "full parallel"
    virtual ~DecoderSelector_config() = default;

    virtual void save(std::fstream &file) const
    {
        file << "Selector type: " << selector_type << std::endl;
    };
};

/**
 * This base class serves as template for selecting which decoders will be run in which order
 * it returns std::vector<std::vector<size_t>> {vec1, ..., vec_last_group}
 * and all decoders indexed by vec1 are emulated for
 * i.e., different groups are run sequentially
 */

class DecoderSelector
{
protected:
    const Ensemble_Decoder &ensemble;

public:
    explicit DecoderSelector(Ensemble_Decoder &ensemble) : ensemble(ensemble) {}
    virtual void init(const DecoderSelector_config &config) = 0;
    std::string selector_type;

    size_t number_groups;
    size_t decoder_per_groups;
    virtual std::vector<Group> select() = 0; // const EnsembleStateView &state view allows to look at what is relevant for selection!

    std::array<size_t, 2> get_group_shapes() const { return {number_groups, decoder_per_groups}; };

    virtual ~DecoderSelector() = default;
};