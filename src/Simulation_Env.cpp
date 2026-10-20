#include <channel_coding_lib/Simulation_Env.h>
#include <omp.h>

//---------------------------------------------------------------------------------------------------------------------
//                                      SIMULATION_STATS
//---------------------------------------------------------------------------------------------------------------------

void Simulation_Stats::save(double snr, const std::string &save_dir)
{
    // Save simulation stats in stats.json
    std::string key = double_to_string(snr);
    std::string path = save_dir + "/stats.json";
    json stats = load_json(path);

    stats["frame_errors"][key] = frame_errors.at(snr);
    stats["trials"][key] = trials.at(snr);
    stats["bit_errors"][key] = bit_errors.at(snr);

    save_json(path, stats);

    // Save decoder stats in decoder_stats.json
    std::visit([&](auto &decoder_stats)
               { decoder_stats.save(snr, save_dir); }, decoder); // (this struct's decoder stats)
}
//-------------------------------------------------------------------------------------------------
//                                      SIMULATION
//-------------------------------------------------------------------------------------------------

/**
 * @brief Simulates transmission of a single codeword to get number of errors
 *
 * Uses BPSK modulation to transmit through an AWGN channel. Bit errors are
 * counted before and after FEC.
 *
 * @param threadID Index of thread to access thread-local objects
 * @param local Pre-allocated containers of thread
 */
void Simulation_Env::simulate_transmission(size_t threadID, TrialContext &local)
{
    local.bit_errors_in_trial[0] = 0; // Pre-FEC bit errors
    local.bit_errors_in_trial[1] = 0; // Post-FEC bit errors

    if (use_all_zero_codeword)
        // Use all-zero codeword
        local.codeword_sent.zeros();
    else
    {
        // Encode random data to get valid codeword
        const GF2::Vector &random_data = GF2::Vector::Random(k, RNG_set[threadID]);
        // ra const GF2::Vector &random_data = GF2::Vector::Random(k, RNG_set[threadID]);
        //  random_data[shortened bits] = 0

        // if possible, uses systematic encoding
        // however, if code can't be put in systematic form--> k info bits at other positions
        local.codeword_sent = encoder_set[threadID]->encode(random_data);
    }

    // Use BPSK to modulate codeword and simulate channel by applying AWGN
    for (size_t i : bits_to_transmit)
    {
        double noise = normal_distribution_set[threadID](RNG_set[threadID]) * sigma_AWGN;
        local.LLR_received[i] = ((1.0 - 2.0 * local.codeword_sent[i]) + noise) * L_c;
    }
    // Account for erasures of message bit puncturing
    std::uniform_int_distribution<> random_bool(0, 1);
    if (!bits_to_erase.empty())
        for (size_t i : bits_to_erase)
            local.LLR_received[i] = random_bool(RNG_set[threadID]) ? -NEAR_ZERO : +NEAR_ZERO;

    // if(!enforcetozero)
    // set respctive llrs to infinity

    // Compare received LLRs and sent bits to get Pre-FEC bit errors
    // Bit = 0   -->   LLR > 0   -->   signbit = 0
    // Bit = 1   -->   LLR < 0   -->   signbit = 1
    for (size_t i = 0; i < num_bits_to_compare; ++i)
        if (std::signbit(local.LLR_received[i]) != local.codeword_sent[i])
            local.bit_errors_in_trial[0]++;
    // TODO: if not allzero and num_bits_to_compare=k, then check at which position k inf bits are located using permutation stored in enc. object

    // Use decoder to perform FEC based on received channel LLRs
    decoder_set[threadID]->decode(local.LLR_received);

    // now using getter
    decoder_set[threadID]->get_codeword(local.codeword_decoded);
    // local.codeword_decoded = decoder_set[threadID]->codeword;

    //  Compare decoded and sent bits to get Post-FEC bit errors
    for (size_t i = 0; i < num_bits_to_compare; ++i)
        if (local.codeword_decoded[i] != local.codeword_sent[i])
            local.bit_errors_in_trial[1]++;
    // TODO: if not allzero and num_bits_to_compare=k, then check at which position k inf bits are located using permutation stored in enc. object
}

/**
 * @brief Runs transmissions in parallel threads to simulate sufficient number of errors
 *
 * Synchronization of a thread is based on a fixed amount of local frame errors.
 * The method gets terminated if the targeted number of global frame errors is met
 * or the max. number of transmissions exceeded.
 */
void Simulation_Env::simulate_errors()
{
#pragma omp parallel num_threads(num_threads)
    {
        size_t threadID = omp_get_thread_num();

        // Preallocate containers used in transmission
        thread_local TrialContext local(n);

        // Init thread local counters
        size_t local_preFEC_bit_errors = 0;
        size_t local_postFEC_bit_errors = 0;
        size_t local_preFEC_frame_errors = 0;
        size_t local_postFEC_frame_errors = 0;
        size_t local_transmissions = 0;

        while (1)
        {
            // Get errors from single transmission
            simulate_transmission(threadID, local);

            // Update local counters
            if (local.bit_errors_in_trial[0] > 0)
            {
                local_preFEC_bit_errors += local.bit_errors_in_trial[0];
                local_preFEC_frame_errors++;
            }
            if (local.bit_errors_in_trial[1] > 0)
            {
                local_postFEC_bit_errors += local.bit_errors_in_trial[1];
                local_postFEC_frame_errors++;
            }
            local_transmissions++;

            // Synchronize every few Post-FEC frame errors
            if (local_postFEC_frame_errors >= 10 || local_transmissions >= 100000)
            {
#pragma omp critical
                {
                    // Update critical global counters
                    postFEC_frame_errors += local_postFEC_frame_errors;
                    transmissions += local_transmissions;

                    // Check global break conditions
                    // (max_transmissions may be exceeded significantly due to infrequent sync at low FERs)
                    if (postFEC_frame_errors >= target_errors || transmissions >= max_transmissions)
                        // Set break flag
                        target_reached = true;
                }
                // Reset read out counters to avoid multiple counting
                local_postFEC_frame_errors = 0;
                local_transmissions = 0;
            }

            if (target_reached)
                break;
        }

// Update global counters
#pragma omp critical
        {
            preFEC_bit_errors += local_preFEC_bit_errors;
            postFEC_bit_errors += local_postFEC_bit_errors;
            preFEC_frame_errors += local_preFEC_frame_errors;
            postFEC_frame_errors += local_postFEC_frame_errors;
            transmissions += local_transmissions;
        }
    }

// Wait for all threads to finish
#pragma omp barrier
}

/**
 * @brief Simulates pre- & post-FEC error rates at different SNR values
 *
 * @param max_iterations Max. number of decoding iterations
 * @param SNR_set Set of SNR values (dB) to simualate at
 */
void Simulation_Env::get_error_rates(std::vector<double> &SNR_set)
{

    // Simulate error rates for every SNR:
    for (size_t ind = 0; ind < SNR_set.size(); ind++)
    {
        reset_counters_and_flags();

        // Update noise parameters
        double SNR = SNR_set[ind];
        get_noise_parameters(SNR);
        for (auto &decoder : decoder_set)
            decoder->adapt_to_channel(L_c);

        simulate_errors();

        get_stats(SNR);

        // Compute error rates for this SNR
        double preFEC_BER = static_cast<double>(preFEC_bit_errors) / static_cast<double>(transmissions * num_bits_to_compare);
        double postFEC_BER = static_cast<double>(postFEC_bit_errors) / static_cast<double>(transmissions * num_bits_to_compare);
        double preFEC_FER = static_cast<double>(preFEC_frame_errors) / static_cast<double>(transmissions);
        double postFEC_FER = static_cast<double>(postFEC_frame_errors) / static_cast<double>(transmissions);

        // Save error rates
        error_rates["preFEC_BER-SNR"][SNR] = preFEC_BER;
        error_rates["preFEC_FER-SNR"][SNR] = preFEC_FER;
        error_rates["BER-SNR"][SNR] = postFEC_BER;
        error_rates["FER-SNR"][SNR] = postFEC_FER;

        // Save additional data
        if (auto_save)
        {
            save(SNR);
        }
    }
}

/**
 * @brief Computes standard deviation of noise & channel reliability from SNR
 *
 * @param SNR SNR of channel in dB (Eb/N0 or Es/N0, depending on SNR_type set)
 */
void Simulation_Env::get_noise_parameters(double SNR)
{
    // Get standard deviation of AWGN
    double SNR_linear = std::pow(10.0, SNR / 10.0);
    if (SNR_type == "Eb/N0")
        sigma_AWGN = std::sqrt(1.0 / (2.0 * r * SNR_linear));
    else if (SNR_type == "Es/N0")
        sigma_AWGN = std::sqrt(1.0 / (2.0 * SNR_linear));
    else
        throw std::invalid_argument("Invalid snr_type. Use 'Eb/N0' or 'Es/N0'.");

    // Set channel reliability parameter
    L_c = 2.0 / (sigma_AWGN * sigma_AWGN);
}

//-------------------------------------------------------------------------------------------------
//                                      INITIALIZATION
//-------------------------------------------------------------------------------------------------

void Simulation_Env::all_zero_init(const std::shared_ptr<Decoder_config> config)
{
    this->init(std::shared_ptr<Encoder_config>{}, config, true);
}
//-------------------------------------------------------------------------------------------------
//                                      INITIALIZATION
//-------------------------------------------------------------------------------------------------

void Simulation_Env::init(std::shared_ptr<Encoder_config> enc_config, const std::shared_ptr<Decoder_config> dec_config, bool all_zero)
{

    use_all_zero_codeword = all_zero;

    if (!all_zero)
    {
        assert(enc_config != nullptr);
        std::cerr << "Using random codeword\n";
    }
    else
    {
        std::cerr << "Using all-zero codeword assumption\n";
    }

    init_bit_comparison_scope();

    // TODO, check if n in encoder and sim env equal!

    // Init statistics
    stats = Simulation_Stats(*dec_config);
    stats.all_zero = all_zero;
    // (Optional) Setup save
    if (auto_save)
    {
        init_save(*dec_config);
    }

    // For each available thread, init all required objects:
    if (num_threads == 0)
        num_threads = omp_get_max_threads();

    decoder_set.resize(num_threads);
    RNG_set.resize(num_threads);
    normal_distribution_set.resize(num_threads);

    if (!use_all_zero_codeword && enc_config)
    {

        encoder_set.resize(num_threads);

        // TODO: This should not be done here?! //check why requiredby pcm encoder
        size_t min_col = (bits_to_erase.empty()) ? 0 : bits_to_erase.back();
        enc_config->min_col = min_col;
    }

    std::random_device random_seed;
    for (size_t thread = 0; thread < num_threads; ++thread)
    {
        // Init RNG
        RNG_set[thread] = std::mt19937(random_seed());

        // Get standardized normal distribution
        normal_distribution_set[thread] = std::normal_distribution<double>(0, 1);

        // Construct decoder
        decoder_set[thread] = decoder_factory.createDecoder(dec_config);

        // Get a copy of encoder prototype for this thread
        if (!use_all_zero_codeword && enc_config)
        {
            encoder_set[thread] = encoder_factory.createEncoder(enc_config);
        }
    }
}

/**
 * @brief Init full transmission
 */
void Simulation_Env::init_full_transmission()
{
    // Include all codeword bits in transmission set
    bits_to_transmit.resize(n);
    for (size_t i = 0; i < n; ++i)
        bits_to_transmit[i] = i;
    // Set code parameters
    n_transmitted = n;
    k_transmitted = k;
    // Get code rate
    r = static_cast<double>(k_transmitted) / static_cast<double>(n_transmitted);
}

/**
 * @brief Set length of bit sequence to be compared for error rate evaluation
 */
void Simulation_Env::init_bit_comparison_scope()
{
    if (bit_comparison_scope == "data")
        num_bits_to_compare = k;
    else if (bit_comparison_scope == "all")
        num_bits_to_compare = n;
    else
        throw std::invalid_argument("Invalid bit_comparison_scope. Use 'data' or 'all'.");
}

//-------------------------------------------------------------------------------------------------
//                                      HELPERS
//-------------------------------------------------------------------------------------------------

void Simulation_Env::reset_counters_and_flags()
{
    target_reached = false;
    preFEC_bit_errors = 0;
    postFEC_bit_errors = 0;
    preFEC_frame_errors = 0;
    postFEC_frame_errors = 0;
    transmissions = 0;
}

void Simulation_Env::get_stats(double SNR)
{
    // Get stats
    stats.frame_errors[SNR] = postFEC_frame_errors;
    stats.bit_errors[SNR] = postFEC_bit_errors;
    stats.trials[SNR] = transmissions;

    // Get channel stats
    // std::visit([&](auto& channel_stats) {
    //     channel_stats.get(channel_set, trials, epsilon);
    // }, stats.channel);

    // Get decoder stats
    std::visit([&](auto &decoder_stats)
               { decoder_stats.get(decoder_set, transmissions, SNR); }, stats.decoder);
}

//---------------------------------------------------------------------------------------------------------------------
//                                      CONFIGURATION
//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets up simulation for puncturing
 *
 * Marks punctured message bits as erasures

 * To remove punctured parity bits from transmission and PCM entirely, provide the adjusted PCM directly!
 *
 * @param bits_to_puncture Indices of bits to puncture
 */
void Simulation_Env::puncturing(std::vector<size_t> bits_to_puncture)
{
    if (bits_to_puncture.empty())
        return;

    // Message bit puncturing:
    if (!bits_to_puncture.empty())
    {
        bits_to_erase.resize(bits_to_puncture.size());
        // Add punctured message bits to erasure set
        bits_to_erase = bits_to_puncture;

        // Remove erasures from transmission set
        std::vector<size_t> temp;
        std::set_difference(
            bits_to_transmit.begin(), bits_to_transmit.end(),
            bits_to_erase.begin(), bits_to_erase.end(),
            std::back_inserter(temp));
        bits_to_transmit.swap(temp);
    }

    // Update code parameters
    n_transmitted -= bits_to_puncture.size();
    r = static_cast<double>(k_transmitted) / static_cast<double>(n_transmitted);
}

//-------------------------------------------------------------------------------------------------
//                                      SIMULATION_ENV - SAVE
//-------------------------------------------------------------------------------------------------

void Simulation_Env::save(double SNR)
{
    std::string key = double_to_string(SNR);

    // Lambda method for saving LER map into JSON file
    auto save_error_rate = [&](const std::string &filename, const std::map<double, double> &error_rate_map)
    {
        std::string path = save_dir + "/" + filename;
        json error_rate_json = load_json(path);
        if (error_rate_map.count(SNR))
            error_rate_json[key] = error_rate_map.at(SNR);
        save_json(path, error_rate_json);
    };

    // Save LERs
    save_error_rate("FER.json", error_rates["FER-SNR"]);
    save_error_rate("BER.json", error_rates["BER-SNR"]);

    // Save statistics
    stats.save(SNR, save_dir);
}

void Simulation_Env::init_save(const Decoder_config &decoder_config)
{
    // Get timestamp of init (different formats for directory name & meta.txt)
    std::time_t time = std::time(nullptr);
    std::ostringstream oss_dir, oss_txt;
    oss_dir << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
    oss_txt << std::put_time(std::localtime(&time), "%d.%m.%Y %H:%M:%S");
    std::string timestamp_dir = oss_dir.str();
    std::string timestamp_txt = oss_txt.str();

    // Setup auto save directory
    if (save_dir.empty())
        save_dir = "results_" + timestamp_dir;
    std::filesystem::create_directories(save_dir);

    // Generate documentation from configs
    std::fstream file(save_dir + "/meta.txt", std::ios::in | std::ios::out | std::ios::trunc);
    file << "CCL2 Simulation " << timestamp_txt << std::endl;
    file << "==================================\n\n";
    file << "Code" << std::endl;
    file << "----------------------------------\n";
    file << "n: " << n << std::endl;
    file << "k: " << k << std::endl;
    file << std::endl;
    file << "Decoder" << std::endl;
    file << "----------------------------------\n";
    decoder_config.save(file);
    file << std::endl;
    file << "Simulation" << std::endl;
    file << "----------------------------------\n";
    file << "target_errors: " << target_errors << std::endl;
    file << "max_trials: " << max_transmissions << std::endl;
}

//-------------------------------------------------------------------------------------------------
//                                      SIMULATION_ENV - INIT
//-------------------------------------------------------------------------------------------------

// void Simulation_Env::init(const std::shared_ptr<Channel_Config> channel_config, const std::shared_ptr<Decoder_Config> decoder_config)
// {
//     // Check enabled features
//     if (evaluate_list && !iequals(decoder_config->decoder_type, "Ensemble"))
//         throw std::invalid_argument("Invalid decoder config specified: evaluate_list requires ensemble decoder!");

//     // Check configs
//     if (iequals(channel_config->channel_type, "BSC") && !iequals(decoder_config->field_type, "GF2"))
//         throw std::invalid_argument("Invalid field_type specified in decoder config: BSC requires GF2 decoding!");
//     if (iequals(channel_config->channel_type, "Pauli-Channel") && !iequals(decoder_config->field_type, "GF4"))
//         throw std::invalid_argument("Invalid field_type specified in decoder config: Pauli channel requires GF4 decoding!");

//     // Init statistics
//     stats = Simulation_Stats(*decoder_config);

//     // (Optional) Setup save
//     if (auto_save)
//         init_save(*channel_config, *decoder_config);

//     // Init per-thread objects and pointers:
//     if (num_threads == 0)
//         num_threads = omp_get_max_threads();
//     decoder_set.resize(num_threads);
//     channel_set.resize(num_threads);

//     for (size_t threadID = 0; threadID < num_threads; ++threadID)
//     {
//         // Create channel instance
//         channel_set[threadID] = create_channel(G, channel_config);

//         // Create decoder instance
//         decoder_set[threadID] = create_decoder(decoder_config);
//     }
// }