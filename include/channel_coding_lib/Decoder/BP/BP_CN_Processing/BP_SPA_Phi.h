/**
 * @file BP_SPA_Phi.h
 * @brief Implements Phi-based SPA for BP decoding
 */

#ifndef BP_SPA_PHI_H
#define BP_SPA_PHI_H

#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>

/**
 * @class BP_SPA_Phi
 * @brief Implementation of Phi-based SPA for BP decoding
 *
 * This class provides Phi-based SPA as a check node processing rule that can be
 * called in BP_Decoder::check_node_update.
 *
 * Because this class is not derived from BP_Decoder but included via the
 * BP_CN_Processing interface, all methods work on a reference of the
 * decoder object BP_CN_Processing::BP.
 */
class BP_SPA_Phi : public BP_CN_Processing
{
public:
    using BP_CN_Processing::BP_CN_Processing;
    
    void init(const BP_config& config) override {};

    void check_node_update(size_t j);
};

#endif // BP_SPA_PHI_H