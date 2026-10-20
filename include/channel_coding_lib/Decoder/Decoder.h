/**
 * @file Decoder.h
 * @brief Implements decoder base class
 */

#ifndef DECODER_BASE_H
#define DECODER_BASE_H

#include <GF2/GF2.h>
#include <channel_coding_lib/Utility.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>
#include <iostream>

/**
 * @struct Decoder_config
 * @brief Base config struct for all decoders
 */
struct Decoder_config
{
    std::string decoder_base_type; // Type of decoder, e.g., bp, osd or sc
    virtual ~Decoder_config() = default;

    virtual void copy(const Decoder_config &config, bool copy_all = true);
    virtual void save(std::fstream &file) const;
};

struct ConvergenceInfo
{
    bool converged;

    // decoding effort to compare different decoders
    // for instance, assumes that decoding effort is linear in the sense of
    // decoding_effort_ensemble = decoding_effort_1+ decoding_effort_2
    // decoding effort should correlate with how fast the decoder found the codeword
    // note that is only really valid for ensembles consiting of the same inner decoder
    // for instance, for bp, we use actually exectued number of iterations per constituent decoder
    // for sc decoding, simply set to 1 as decoding effort will always be the same for the different sc decoders
    size_t decoding_effort;
};

/**
 * @class Decoder
 * @brief Base class of all decoders
 *
 * This class provides common fundamentals of all decoders (code parameters,
 * syndrome check etc.). Attributes and methods that have to be directly accessed
 * by Simulation_Env should be declared in this class.
 */
class Decoder
{
protected:
    // Only access via getter to avoid returning wrong size (e.g when using avn)

public:
    virtual ~Decoder() = default;
    ConvergenceInfo decoder_conv_info;
    // Decoding In-/Output
    std::vector<double> LLR_received; // Received channel LLRs

    // Additional data about decoding process
    std::vector<size_t> decoding_count;    // Counted decoding processes (initial hard-decision invalid)
    std::vector<size_t> unconverged_count; // Counted unconverged decoding processes

    virtual void init(const Decoder_config &config) = 0;
    virtual void adapt_to_channel(double L_c) = 0;
    virtual void decode(const std::vector<double> &LLR_received) = 0;
    GF2::Vector codeword; // Decoded codeword

    /**
     * @brief Writes the decoded codeword into `out`.
     *
     * Contract:
     *  - `out` will be resized to the correct transmitted length
     *  - No assumptions about internal decoder padding (e.g. AVNs)
     */
    virtual void get_codeword(GF2::Vector &out) const = 0;

    // ToDo: These methods are decoder specific and should be moved somewhere else (Decoder.cpp can be deleted then)
    // void alloc_decode_data(size_t num_units);
    // void remove_CNs_from_layer(const std::vector<size_t> &CNs_removed);
};

//-------------------------------------------------------------------------------------------------
//                                      DECODER_STATS
//-------------------------------------------------------------------------------------------------

/**
 * @struct Decoder_Stats
 * @brief . . .
 */
struct Decoder_Stats
{
    virtual ~Decoder_Stats() = default;

    //
    virtual void get(const std::vector<std::unique_ptr<Decoder>> &multithreading_decoder_set, size_t trials, double snr) {};
    virtual void save(double snr, const std::string &save_dir) {};
};

#endif // DECODER_BASE_H
