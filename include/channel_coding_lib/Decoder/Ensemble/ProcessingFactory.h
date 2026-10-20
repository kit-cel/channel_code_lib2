
/**
 * @file ProcessingFactory.h
 * @brief Implements decoder construction & intiailization
 */

#ifndef PROCESSING_FACTORY_H
#define PROCESSING_FACTORY_H

#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/Identity_Processing.h>
#include <channel_coding_lib/Decoder/Ensemble/Processing_Diversity/Automorphism_Processing.h>

class ProcessingFactory
{
public:
    std::unique_ptr<Processing_Diversity> createProcessing(const std::shared_ptr<Processing_config> &config)
    {
        std::unique_ptr<Processing_Diversity> processing_diversity;
        if (iequals(config->processing_type, "identity"))
        {

            processing_diversity = std::make_unique<Identity_Processing_Diversity>();
        }
        else if (iequals(config->processing_type, "aed"))
        {
            processing_diversity = std::make_unique<Automorphism_Processing>();
        }
        else
        {
            throw std::invalid_argument("Unknown processing_type!");
        }

        processing_diversity->init(*config);
        return processing_diversity;

        throw std::runtime_error("Processing construction failed!");
        return nullptr;
    }
};

#endif // PROCESSING_FACTORY_H
