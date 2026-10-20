/**
 * @file Decoder_Factory.h
 * @brief Implements decoder construction & intiailization
 */

#ifndef DECODER_FACTORY_H
#define DECODER_FACTORY_H

// Include Belief Propagation Decoders
#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>
#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Flooding.h>
#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Row_Layered.h>
#include <channel_coding_lib/Decoder/BP/BP_Scheduling/BP_Column_Layered.h>
#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_SPA.h>
#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_SPA_Phi.h>
#include <channel_coding_lib/Decoder/BP/BP_CN_Processing/BP_MSA.h>
#include <channel_coding_lib/Decoder/Ensemble/ProcessingFactory.h>

/**
 * @class Decoder_Factory
 * @brief Constructs & initializes decoder instance
 */
class Decoder_Factory
{
protected:
    bool thread_printed = false;
    std::unique_ptr<Decoder> createBPdecoder(const std::shared_ptr<const Decoder_config> &config)
    {
        // Get pointer to BP specific config
        auto bp_config = std::dynamic_pointer_cast<const BP_config>(config);
        if (!bp_config)
            throw std::invalid_argument("Invalid config type for BP decoder!");
        // Create decoder instance

        auto bp_decoder = std::make_unique<BP_Decoder>();
        // auto *bp_decoder = static_cast<BP_Decoder *>(decoder_set[i].get());

        // Set scheduling implementation
        if (iequals(bp_config->scheduling_type, "column_layered"))
            bp_decoder->set_scheduling_method(std::make_unique<BP_Column_Layered>(*bp_decoder));
        else if (iequals(bp_config->scheduling_type, "row_layered"))
            bp_decoder->set_scheduling_method(std::make_unique<BP_Row_Layered>(*bp_decoder));
        else if (iequals(bp_config->scheduling_type, "flooding"))
            bp_decoder->set_scheduling_method(std::make_unique<BP_Flooding>(*bp_decoder));
        else
            throw std::invalid_argument("Unknown scheduling_type!");

        // Set CN update implementation
        if (iequals(bp_config->cn_update_type, "spa"))
            bp_decoder->set_cn_processing(std::make_unique<BP_SPA>(*bp_decoder));
        else if (iequals(bp_config->cn_update_type, "spa_phi"))
            bp_decoder->set_cn_processing(std::make_unique<BP_SPA_Phi>(*bp_decoder));
        else if (iequals(bp_config->cn_update_type, "msa"))
            bp_decoder->set_cn_processing(std::make_unique<BP_MSA>(*bp_decoder));
        else
        {
            std::cerr << bp_config->cn_update_type << " is invalid" << std::endl;
            throw std::invalid_argument("Unknown cn_update_type!");
        }

        return bp_decoder;
    }
    std::unique_ptr<Decoder> createOSDdecoder(const std::shared_ptr<const Decoder_config> &config)
    {
        throw std::invalid_argument("OSD not yet implemented!");
        return nullptr;
    }
    std::unique_ptr<Decoder> createSCdecoder(const std::shared_ptr<const Decoder_config> &config)
    {
        throw std::invalid_argument("SC not yet implemented!");
        return nullptr;
    }
    std::unique_ptr<Decoder> createEnsembledecoder(const std::shared_ptr<const Decoder_config> &config)
    {
        auto ensemble_config = std::dynamic_pointer_cast<const Ensemble_config>(config);
        if (!ensemble_config)
            throw std::invalid_argument("Invalid config type for Ensemble decoder!");
        if (ensemble_config->dec_config_set.empty())
            throw std::invalid_argument("Ensemble decoder requires at least one sub-decoder");


        auto number_decoders = ensemble_config->dec_config_set.size();
        std::vector<std::unique_ptr<Decoder>> decoder_set;
        decoder_set.resize(number_decoders);

        for (size_t i = 0; i < number_decoders; ++i)
        {
            decoder_set[i] = createDecoder(ensemble_config->dec_config_set[i]);
        }
        auto ensemble_decoder = std::make_unique<Ensemble_Decoder>(std::move(decoder_set));

        ProcessingFactory processing_factory;
        auto processing_diversity = processing_factory.createProcessing(ensemble_config->processing_config);
        ensemble_decoder->set_processing_diversity(std::move(processing_diversity));
        // Processing diversity might be set as default to identity in config;
        return ensemble_decoder;
    }

public:
    std::unique_ptr<Decoder> createDecoder(const std::shared_ptr<const Decoder_config> &config)
    {
        if (!config)
            throw std::invalid_argument("config must not be null");
        std::unique_ptr<Decoder> decoder;

        if (iequals(config->decoder_base_type, "bp"))
            //-------------------------------------------------------------------------------------------------------------
            //                                      BELIEF PROPAGATION
            //-------------------------------------------------------------------------------------------------------------
            decoder = createBPdecoder(config);
        else if (iequals(config->decoder_base_type, "osd"))
            //-------------------------------------------------------------------------------------------------------------
            //                                      ORDERED STATISTICS DECODING
            //-------------------------------------------------------------------------------------------------------------
            decoder = createOSDdecoder(config);
        else if (iequals(config->decoder_base_type, "sc"))
            //-------------------------------------------------------------------------------------------------------------
            //                                      SUCCESSIVE CANCELLATION
            //-------------------------------------------------------------------------------------------------------------
            decoder = createSCdecoder(config);
        else if (iequals(config->decoder_base_type, "ensemble"))
        {

            //-------------------------------------------------------------------------------------------------------------
            //                                      ENSEMBLE DECODING
            //-------------------------------------------------------------------------------------------------------------
            decoder = createEnsembledecoder(config);
        }
        else
            throw std::invalid_argument("Unknown decoder_base_type!");
        // Constructing standalone decoder
        decoder->init(*config);
        return decoder;
    }
    // std::unique_ptr<Decoder> createDecoder(const std::shared_ptr<Decoder_config> &config, std::unique_ptr<Processing_Diversity> processing_diversity)
    // {
    //     // Constructing standalone decoder
    //     return createDecoder({config}, std::move(processing_diversity));
    // }
    // std::unique_ptr<Decoder> createDecoder(const std::vector<std::shared_ptr<Decoder_config>> &config_set)
    // {
    //     // Constructing ensemble decoding with possibly different decoders in ensemble but no processing diversity
    //     return createDecoder(config_set, nullptr);
    // }
    // std::unique_ptr<Decoder> createDecoder(const std::vector<std::shared_ptr<Decoder_config>> &config_set, std::unique_ptr<Processing_Diversity> processing_diversity)
    // {
    //     std::vector<std::unique_ptr<Decoder>> decoder_set(config_set.size());

    //     for (size_t i = 0; i < config_set.size(); ++i)
    //     {
    //         //-------------------------------------------------------------------------------------------------------------
    //         //                                      BELIEF PROPAGATION
    //         //-------------------------------------------------------------------------------------------------------------

    //         if (iequals(config_set[i]->decoder_base_type, "bp"))
    //         {
    //             decoder_set[i] = createBPdecoder(config_set[i]);
    //         }

    //         //-------------------------------------------------------------------------------------------------------------
    //         //                                      ORDERED STATISTICS DECODING
    //         //-------------------------------------------------------------------------------------------------------------

    //         else if (iequals(config_set[i]->decoder_base_type, "osd"))
    //         {
    //         }

    //         else if (iequals(config_set[i]->decoder_base_type, "sc"))
    //         {
    //         }

    //         else
    //             throw std::invalid_argument("Unknown decoder_base_type!");

    //         // Run decoder specific init using its config
    //         decoder_set[i]->init(*config_set[i]);
    //     }

    //     // Return single decoder instance as standalone decoder
    //     if (decoder_set.size() == 1)
    //         return std::move(decoder_set[0]);
    //     // Return ensemble decoder built from decoder instances
    //     else
    //     {
    //         auto ensemble_decoder = std::make_unique<Ensemble_Decoder>(std::move(decoder_set));
    //         // if not nullptr; processing diversity provided -> set it for ensemble decoder
    //         if (processing_diversity)
    //         {
    //             ensemble_decoder->set_processing_diversity(std::move(processing_diversity));
    //         }
    //         else
    //         // Otherwise: multiple decoder configs provided -> Ensemble decoder
    //         // The individual decoder are already initialized-> only missing processing diversity
    //         // Not provided; therefore assumed to be identity!
    //         {
    //             ensemble_decoder->set_processing_diversity(std::make_unique<Identity_Processing_Diversity>());
    //         }

    //         return ensemble_decoder;
    //     }

    //     throw std::runtime_error("Decoder construction failed!");
    //     return nullptr;
    // }
};

#endif // DECODER_FACTORY_H
