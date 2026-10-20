#include <channel_coding_lib/Decoder/Ensemble/DecoderSelector/FullParallelSelector.h>
#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>

FullParallelSelector::FullParallelSelector(Ensemble_Decoder &ensemble) : DecoderSelector(ensemble)
{
    number_groups = 1;
    decoder_per_groups = this->ensemble.ensemble_size;
};

std::vector<Group> FullParallelSelector::select()
{
    Group full_parallel;

    full_parallel.members.resize(this->ensemble.ensemble_size);
    full_parallel.group_index = 0;
    std::iota(full_parallel.members.begin(), full_parallel.members.end(), 0);
    return {full_parallel};
};