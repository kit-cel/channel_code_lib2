#ifndef DECODER_STATS_H
#define DECODER_STATS_H

#include "Decoder_Factory.h"
#include <variant>
// Wrapper for decoder statistics
using Any_Decoder_Stats = std::variant<Decoder_Stats, BP_Decoder_Stats, Ensemble_Decoder_Stats>;

inline Any_Decoder_Stats init_decoder_stats(const Decoder_config &config)
{
    if (iequals(config.decoder_base_type, "bp"))
        return BP_Decoder_Stats();
    else if (iequals(config.decoder_base_type, "ensemble"))
        return Ensemble_Decoder_Stats();
    else
        return Decoder_Stats();
}

#endif // DECODER_STATS_H