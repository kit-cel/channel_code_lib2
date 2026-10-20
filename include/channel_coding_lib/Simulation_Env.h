/**
 * @file Simulation_Env.h
 * @brief Contains main structure of simulation
 */

#ifndef SIMULATION_ENV_H
#define SIMULATION_ENV_H

#include <channel_coding_lib/Decoder/Decoder_Factory.h>
#include <channel_coding_lib/Decoder/Decoder_Stats.h>
#include <channel_coding_lib/Encoder/Encoder_Factory.h>
#include <GF2/GF2.h>
#include <GF2/Matrix.h>
#include <channel_coding_lib/Utility.h>

#include <vector>
#include <string>
#include <map>

#include <random>

#include <memory>

/**
 * @class TrialContext
 * @brief Thread local containers used in trial
 *
 * By using these containers in a struct, redundant memory allocation in
 * Simulation_Env::simulate_transmission can be avoided.
 */
struct TrialContext
{
    std::vector<size_t> bit_errors_in_trial;
    GF2::Vector codeword_sent;
    std::vector<double> LLR_received;
    GF2::Vector codeword_decoded;

    TrialContext(size_t n)
    {
        bit_errors_in_trial.resize(2, 0);
        codeword_sent.resize(n);
        LLR_received.resize(n);
        codeword_decoded.resize(n);
    }
};

//-------------------------------------------------------------------------------------------------
//                                      SIMULATION_STATS
//-------------------------------------------------------------------------------------------------

/**
 * @struct Simulation_Stats
 * @brief . . .
 */
struct Simulation_Stats
{
    Simulation_Stats() = default;
    Simulation_Stats(const Decoder_config &config) : decoder(init_decoder_stats(config)) {}

    std::map<double, size_t> frame_errors; // Number of frame errors
    std::map<double, size_t> bit_errors;   // Number of bit errors
    std::map<double, size_t> trials;       // Number of trials
    bool all_zero;

    // Any_Channel_Stats channel; // Channel statistics //currently only AWGN
    Any_Decoder_Stats decoder; // Decoder statistics

    void save(double snr, const std::string &save_dir);
};

/**
 * @class Simulation_Env
 * @brief Main class of simulation
 */
class Simulation_Env
{
private:
    Decoder_Factory decoder_factory;
    Encoder_Factory encoder_factory;

    // Vectors of per-thread objects & pointers
    std::vector<std::unique_ptr<Decoder>> decoder_set;
    std::vector<std::unique_ptr<Encoder>> encoder_set;
    std::vector<std::mt19937> RNG_set;
    std::vector<std::normal_distribution<double>> normal_distribution_set;

public:
    // TODO remove unnecessary params from constructor (H)
    // keep k and n to check if consistent with encoder!
    Simulation_Env(size_t k, size_t n, std::string bit_comparison_scope)
        : k(k), n(n), bit_comparison_scope(bit_comparison_scope)
    {
        // Init decoder factory
        decoder_factory = Decoder_Factory();
        encoder_factory = Encoder_Factory();

        // Init full transmission & code parameters
        init_full_transmission();
        m = this->H.rows();
    }

    // Code
    GF2::Matrix H;        // Parity check matrix
    size_t n;             // Number of bits in codeword //num columns of G or PCM
    size_t k;             // Number of data bits in codeword
    size_t m;             // Number of parity bits in codeword
    size_t n_transmitted; // Number of actually transmitted bits n_transmitted=n-message_bit_pucturing.size()
    size_t k_transmitted; // Number of actually transmitted data bits
    double r;             // Code rate

    // Simulation scenario
    std::string SNR_type = "Eb/N0";         // Used to switch between SNR per bit & SNR per symbol
    const std::string bit_comparison_scope; // Sets scope of bit comparison for evaluating error rates
    size_t num_bits_to_compare;             // Length of bit sequence to be compared for error rate evaluation

    // Transmission
    std::vector<size_t> bits_to_transmit; // Bits actually transmitted through noisy channel
    std::vector<size_t> bits_to_erase;    // Bits set to be erasures

    // Performance & Precision
    size_t num_threads = 0;            // Number of hardware threads used (Default 0 -> use all available)
    size_t target_errors = 400;        // Min. number of frame errors simulated per SNR
    size_t max_transmissions = 1e6;    // Max. number of transmissions simulated per SNR
    bool use_all_zero_codeword = true; // En-/Disables use of "all-zero" codeword for transmission

    // Channel
    double sigma_AWGN; // Standard deviation of AWGN
    double L_c;        // Channel reliability parameter

    // Flags & counter
    bool target_reached; // True, when targeted number of errors reached
    size_t preFEC_bit_errors;
    size_t postFEC_bit_errors;
    size_t preFEC_frame_errors;
    size_t postFEC_frame_errors;
    size_t transmissions;

    // Results
    std::string save_dir;                                        // Path to auto save directory
    bool auto_save = false;                                      // Enables automatic saving of simulation results
    std::map<std::string, std::map<double, double>> error_rates; // Error rate mappings (over SNR or iterations)
    Simulation_Stats stats;                                      // Simulation & decoder statistics

    // Simulation
    void get_error_rates(std::vector<double> &SNR_set);
    // void get_error_rates(std::vector<size_t> &max_iterations_set, double SNR);

    // Initialization
    // void init(const std::shared_ptr<Decoder_config> config_set);                                                                          // For standalone decoding
    void all_zero_init(const std::shared_ptr<Decoder_config> config_set); // For ensemble decoding using possibly different decoders in ensemble

    void init(const std::shared_ptr<Encoder_config> enc_config, const std::shared_ptr<Decoder_config> config_set, bool all_zero); // For ensemble decoding using possibly different decoders in ensemble

    // Configuration
    void puncturing(std::vector<size_t> bits_to_puncture);

private:
    // Simulation
    void simulate_transmission(size_t threadID, TrialContext &local);
    void simulate_errors();
    void get_noise_parameters(double SNR);

    void init_full_transmission();
    void init_bit_comparison_scope();

    void get_stats(double SNR);

    // Helpers
    void reset_counters_and_flags();

    // Save
    void save(double SNR);
    void init_save(const Decoder_config &decoder_config);
};

#endif // SIMULATION_ENV_H
