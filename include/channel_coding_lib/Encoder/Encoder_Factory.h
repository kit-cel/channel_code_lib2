/**
 * @file Encoder_Factory.h
 * @brief Implements ENCODER construction & intiailization
 */

#ifndef ENCODER_FACTORY_H
#define ENCODER_FACTORY_H

#include <channel_coding_lib/Encoder/G_based_Encoder.h>
#include <channel_coding_lib/Encoder/PCM_based_Encoder.h>

class Encoder_Factory
{
protected:
public:
    std::unique_ptr<Encoder> createEncoder(const std::shared_ptr<const Encoder_config> &enc_config)
    {
        if (!enc_config)
            throw std::invalid_argument("config must not be null");

        std::unique_ptr<Encoder> encoder;

        if (iequals(enc_config->encoder_base_type, "g_based"))
            encoder = std::make_unique<G_based_Encoder>();
        else if (iequals(enc_config->encoder_base_type, "pcm_based"))
            encoder = std::make_unique<PCM_based_Encoder>();

        encoder->init(*enc_config);
        return encoder;
    }
};
#endif // ENCODER_FACTORY_H
