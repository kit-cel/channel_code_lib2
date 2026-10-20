#include <channel_coding_lib/Decoder/Decoder.h>

//-------------------------------------------------------------------------------------------------
//                                      DECODER_CONFIG
//-------------------------------------------------------------------------------------------------

void Decoder_config::copy(const Decoder_config &config, bool copy_all)
{
    // Copy config parameters from other instance
    decoder_base_type = config.decoder_base_type;
}

void Decoder_config::save(std::fstream &file) const
{
    file << "n: " << decoder_base_type << std::endl;
}
