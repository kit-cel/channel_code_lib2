/**
 * @file BP_Flooding.h
 * @brief Implements flooding scheduling for BP decoding
 */

#ifndef BP_FLOODING_H
#define BP_FLOODING_H

#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>

/**
 * @class BP_Flooding
 * @brief Implementation of flooding scheduling for BP decoding
 *
 * This class provides flooding as a scheduling implementation that can be called
 * in BP_Decoder::decode.
 *
 * Because this class is not derived from BP_Decoder but included via the
 * BP_Scheduling interface, all methods work on a reference of the decoder
 * object BP_Scheduling::BP.
 */
class BP_Flooding : public BP_Scheduling
{
public:
    using BP_Scheduling::BP_Scheduling;

    double sum_all; // Temporary sum over all ingoing messages to v(i)

    void init(const BP_config& config) override {};
    void decode(const std::vector<double> &LLR_received) override;

    void variable_node_update(size_t i);
    void compute_v2c_message(size_t i, size_t j_v);
    void compute_total_LLR(size_t i);
};

#endif // BP_FLOODING_H