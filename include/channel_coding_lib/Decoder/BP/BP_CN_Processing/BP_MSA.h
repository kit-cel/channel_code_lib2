/**
 * @file BP_MSA.h
 * @brief Implements Min-Sum Algorithm for BP decoding
 */

#ifndef BP_MSA_H
#define BP_MSA_H

#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>

/**
 * @class BP_MSA
 * @brief Implementation of normalized Min-Sum Algorithm (MSA) for BP decoding
 *
 * This class provides NSMA as a check node processing rule that can be called
 * in BP_Decoder::check_node_update.
 *
 * Because this class is not derived from BP_Decoder but included via the
 * BP_CN_Processing interface, all methods work on a reference of the
 * decoder object BP_CN_Processing::BP.
 */
class BP_MSA : public BP_CN_Processing
{
public:
    using BP_CN_Processing::BP_CN_Processing;

    void init(const BP_config &config) override {};

    void check_node_update(size_t j) override;
};

#endif // BP_MSA_H