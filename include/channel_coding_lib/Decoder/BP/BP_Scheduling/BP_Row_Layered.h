/**
 * @file BP_Row_Layered.h
 * @brief Implements row-layered scheduling for BP decoding
 */

#ifndef BP_ROW_LAYERED_H
#define BP_ROW_LAYERED_H

#include <channel_coding_lib/Decoder/BP/BP_Decoder.h>

/**
 * @class BP_Row_Layered
 * @brief Implementation of row-layered scheduling for BP decoding
 *
 * This class provides row-layering as a scheduling implementation that can be
 * called in BP_Decoder::decode.
 *
 * Because this class is not derived from BP_Decoder but included via the
 * BP_Scheduling interface, all methods work on a reference of the decoder
 * object BP_Scheduling::BP.
 */
class BP_Row_Layered : public BP_Scheduling
{
public:
    using BP_Scheduling::BP_Scheduling;

    std::vector<size_t> schedule;                    // Order in which layered decoder iterates over nodes
    std::vector<std::vector<size_t>> nodes_in_layer; // Set of nodes for each scheduling layer

    void init(const BP_config& config) override;
    void decode(const std::vector<double> &LLR_received) override;

    void variable_node_update(size_t j);
    void compute_total_LLR(size_t j);
};

#endif // BP_ROW_LAYERED_H
