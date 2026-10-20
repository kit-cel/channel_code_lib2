/**
 * @file BP_SPA.h
 * @brief Implements Sum-Product Algorithm (SPA) for BP decoding
 */

#ifndef BP_SPA_H
#define BP_SPA_H

#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>

/**
 * @class BP_SPA
 * @brief Implementation of Sum-Product Algorithm for BP decoding
 *
 * This class provides SPA as a check node processing rule that can be called in
 * BP_Decoder::check_node_update.
 *
 * Because this class is not derived from BP_Decoder but included via the
 * BP_CN_Processing interface, all methods work on a reference of the
 * decoder object BP_CN_Processing::BP.
 */
class BP_SPA : public BP_CN_Processing
{
public:
    using BP_CN_Processing::BP_CN_Processing;

    double prod_all; // Temporary product over all ingoing messages to c(j)

    void init(const BP_config& config) override {};

    void check_node_update(size_t j);
    void compute_c2v_message(size_t j, size_t i_c);
};

#endif // BP_SPA_H
