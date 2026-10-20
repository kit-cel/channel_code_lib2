#pragma once

/**
 * @file DecoderSelectorFactory.h
 * @brief Implements DecoderSelectorFactory construction & intiailization
 */


#include <channel_coding_lib/Decoder/Ensemble/DecoderSelector/FullParallelSelector.h>

class DecoderSelectorFactory
{
public:
    static std::unique_ptr<DecoderSelector> createDecoderSelector(Ensemble_Decoder &ensemble, const std::shared_ptr<DecoderSelector_config> &config)
    {
        std::unique_ptr<DecoderSelector> decoder_selector;
        if (iequals(config->selector_type, "full_parallel"))
        {

            decoder_selector = std::make_unique<FullParallelSelector>(ensemble);
        }
        else
        {
            throw std::invalid_argument("Unknown selection policy!");
        }

        decoder_selector->init(*config);
        return decoder_selector;

        throw std::runtime_error("Selector construction failed!");
        return nullptr;
    }
};
