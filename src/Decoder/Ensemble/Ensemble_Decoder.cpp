#include <channel_coding_lib/Decoder/Ensemble/Ensemble_Decoder.h>
#include <channel_coding_lib/Decoder/Ensemble/DecoderSelectorFactory.h>
#include <cstdlib>

//-------------------------------------------------------------------------------------------------
//                                      ENSEMBLE_DECODER_CONFIG
//-------------------------------------------------------------------------------------------------

Ensemble_config::Ensemble_config(const GF2::Matrix H, const std::vector<std::shared_ptr<Decoder_config>> config_set)
    : H(H), dec_config_set(config_set), processing_config(std::make_shared<Identity_config>()), decoder_selector_config(std::make_shared<FullParallelSelector_config>())
{
    decoder_base_type = "ensemble";
    // Default is zero an then set to simulate all paths
}

Ensemble_config::Ensemble_config(const GF2::Matrix H, const std::vector<std::shared_ptr<Decoder_config>> config_set, std::shared_ptr<Processing_config> processing_config)
    : H(H), dec_config_set(config_set), processing_config(processing_config), decoder_selector_config(std::make_shared<FullParallelSelector_config>())
{
    decoder_base_type = "ensemble";
    // Default is zero an then set to simulate all paths
}

void Ensemble_config::set_selector_config(std::shared_ptr<DecoderSelector_config> cfg)
{
    decoder_selector_config = std::move(cfg);
}

void Ensemble_config::save(std::fstream &file) const
{
    Decoder_config::save(file);

    auto decoder_ensemble_size = dec_config_set.size();
    auto preprocessing_size = processing_config->processing_ensemble_size;
    size_t ensemble_size;
    if (decoder_ensemble_size > 1 && preprocessing_size == 1)
    {
        ensemble_size = decoder_ensemble_size;
    }
    // Case 2: processing diversity only
    else if (decoder_ensemble_size == 1 && preprocessing_size > 1)
    {
        ensemble_size = preprocessing_size;
    }
    // Case 3: both active → must match
    else if (decoder_ensemble_size > 1 && preprocessing_size > 1)
    {
        if (decoder_ensemble_size != preprocessing_size)
        {
            throw std::runtime_error(
                "Decoder ensemble size must match processing diversity size.");
        }
        ensemble_size = decoder_ensemble_size;
    }
    // Case 4: trivial single path
    else
    {
        ensemble_size = 1;
    }

    decoder_selector_config->save(file);

    // Write config parameters into txt file
    file << "(Ensemble) ensemble_size: " << ensemble_size << std::endl;

    file << "(Ensemble) processing ensemble size: " << preprocessing_size << std::endl;
    file << "(Ensemble) decoder ensemble size: " << decoder_ensemble_size << std::endl;

    std::string path_base_type = "";
    size_t j = 0;
    for (size_t i = 0; i < dec_config_set.size(); i++)
    {
        if (!iequals(path_base_type, dec_config_set[i]->decoder_base_type))
        {
            path_base_type = dec_config_set[i]->decoder_base_type;
            file << "(Ensemble) path " << j << "-" << i << " " << path_base_type;
            j = i + 1;
        }
    }
}

Ensemble_Decoder::Ensemble_Decoder(std::vector<std::unique_ptr<Decoder>> decoder_set)
{
    this->decoder_set = std::move(decoder_set);
    decoder_ensemble_size = this->decoder_set.size();
}

Ensemble_Decoder::~Ensemble_Decoder() = default;
// init executed after set processing diversity!
void Ensemble_Decoder::init(const Decoder_config &config)
{
    const auto &ensemble_config = static_cast<const Ensemble_config &>(config);
    H = ensemble_config.H;

    // Case 1: decoder diversity only
    if (decoder_ensemble_size > 1 && preprocessing_size == 1)
    {
        ensemble_size = decoder_ensemble_size;
    }
    // Case 2: processing diversity only
    else if (decoder_ensemble_size == 1 && preprocessing_size > 1)
    {
        ensemble_size = preprocessing_size;
    }
    // Case 3: both active → must match
    else if (decoder_ensemble_size > 1 && preprocessing_size > 1)
    {
        if (decoder_ensemble_size != preprocessing_size)
        {
            throw std::runtime_error(
                "Decoder ensemble size must match processing diversity size.");
        }
        ensemble_size = decoder_ensemble_size;
    }
    // Case 4: trivial single path
    else
    {
        ensemble_size = 1;
    }

    selector = DecoderSelectorFactory::createDecoderSelector(*this, ensemble_config.decoder_selector_config);

    paths_for_ml.resize(ensemble_size);
    std::iota(paths_for_ml.begin(), paths_for_ml.end(), 0);

    // Init Statistics
    ensemble_decoding_effort_counter = 0;
    ensemble_latency_counter = 0;
    path_decoding_effort_counter.assign(ensemble_size, 0);

    number_active_path_counter = 0;
    number_converged_path_counter = 0;
}

void Ensemble_Decoder::set_processing_diversity(std::unique_ptr<Processing_Diversity> processing_diversity)
{

    
    this->processing_diversity = std::move(processing_diversity);
    this->preprocessing_size = this->processing_diversity->processing_ensemble_size;
}

void Ensemble_Decoder::adapt_to_channel(double L_c)
{
    for (auto &decoder : decoder_set)
        decoder->adapt_to_channel(L_c);
}

void Ensemble_Decoder::execute_group(Group &group, const std::vector<std::vector<double>> &processed_llrs)
{

    // Get codeword candidates from constituent decoders via decoder diversity

    // default of active path is std::iota(active_paths.begin(), active_paths.end(), 0); set in init
    //
    // Execute all decoding paths unconditionally; convergence and effort
    // statistics are collected per path and processed afterwards.
    //* EXECUTE ALL PATHS FROM A GROUP*//

    size_t temp_group_latency = 0;
    if (decoder_set.size() == 1)
    {

        for (auto i : group.members)
        {
            decoder_set[0]->decode(processed_llrs[i]);
            decoder_set[0]->get_codeword(codeword_candidates[i]);

            ensemble_convergence_info[group.group_index][i].converged = decoder_set[0]->decoder_conv_info.converged;
            ensemble_convergence_info[group.group_index][i].decoding_effort = decoder_set[0]->decoder_conv_info.decoding_effort;
            if (decoder_set[0]->decoder_conv_info.decoding_effort > temp_group_latency)
            {
                temp_group_latency = decoder_set[0]->decoder_conv_info.decoding_effort;
            }
            if (ensemble_convergence_info[group.group_index][i].converged)
            {
                converged_paths[group.group_index].push_back(i);
                this->decoder_conv_info.converged = true;
            }
            else
                non_converged_paths[group.group_index].push_back(i);
        }
    }
    else if (processed_llrs.size() == 1)
    {

        for (auto i : group.members)
        {
            decoder_set[i]->decode(processed_llrs[0]);
            // codeword_candidates[i] = decoder_set[i]->codeword;
            decoder_set[i]->get_codeword(codeword_candidates[i]);

            ensemble_convergence_info[group.group_index][i].converged = decoder_set[i]->decoder_conv_info.converged;
            ensemble_convergence_info[group.group_index][i].decoding_effort = decoder_set[i]->decoder_conv_info.decoding_effort;
            if (decoder_set[0]->decoder_conv_info.decoding_effort > temp_group_latency)
            {
                temp_group_latency = decoder_set[0]->decoder_conv_info.decoding_effort;
            }
            if (ensemble_convergence_info[group.group_index][i].converged)
            {
                converged_paths[group.group_index].push_back(i);
                this->decoder_conv_info.converged = true; // at least one path converged!
            }
            else
                non_converged_paths[group.group_index].push_back(i);
        }
    }
    else if (decoder_set.size() == processed_llrs.size())
    {

        for (auto i : group.members)
        {
            decoder_set[i]->decode(processed_llrs[i]);
            // codeword_candidates[i] = decoder_set[i]->codeword;
            decoder_set[i]->get_codeword(codeword_candidates[i]);

            ensemble_convergence_info[group.group_index][i].converged = decoder_set[i]->decoder_conv_info.converged;
            ensemble_convergence_info[group.group_index][i].decoding_effort = decoder_set[i]->decoder_conv_info.decoding_effort;
            if (decoder_set[0]->decoder_conv_info.decoding_effort > temp_group_latency)
            {
                temp_group_latency = decoder_set[0]->decoder_conv_info.decoding_effort;
            }
            if (ensemble_convergence_info[group.group_index][i].converged)
            {
                converged_paths[group.group_index].push_back(i);
                this->decoder_conv_info.converged = true;
            }
            else
                non_converged_paths[group.group_index].push_back(i);
        }
    }
    else
    {
        throw std::runtime_error("Decoder set size does not match ensemble size.");
    }
    group.group_latency = temp_group_latency;
}

void Ensemble_Decoder::update_paths_for_ml(size_t last_group_idx)
{
    // in this method, update paths_for_ml
    //  if no path converged, all paths are used for paths_for_ml
    //  otherwise, all converged paths are used for paths_for_ml

    if (!this->decoder_conv_info.converged)
    {
        // no path of ensemble converged
        this->decoder_conv_info.converged = false;
        paths_for_ml.resize(ensemble_size);
        std::iota(paths_for_ml.begin(), paths_for_ml.end(), 0);
    }
    else
    {
        paths_for_ml.clear();
        paths_for_ml.reserve(ensemble_size);

        // go over all groups and extend paths for ml by all the ones that converged
        for (size_t i = 0; i < last_group_idx; i++)
        {
            paths_for_ml.insert(paths_for_ml.end(), converged_paths[i].begin(), converged_paths[i].end());
        }
    }
}

void Ensemble_Decoder::update_statistics_and_ensemble_effort(size_t last_group_idx)
{
    for (size_t group_idx = 0; group_idx < last_group_idx; group_idx++)
    // only update parameters for executed groups; others are not executed and thus their effort remains not updated
    {

        for (auto i : group_ordering_of_decoders[group_idx].members)
        {

            ensemble_decoding_effort_counter += ensemble_convergence_info[group_idx][i].decoding_effort; //(STATISTICS)
            path_decoding_effort_counter[i] += ensemble_convergence_info[group_idx][i].decoding_effort;  //(STATISTICS)

            this->decoder_conv_info.decoding_effort += ensemble_convergence_info[group_idx][i].decoding_effort; // Total effort of ensemble
        }
        ensemble_latency_counter += group_ordering_of_decoders[group_idx].group_latency;
    }
    // More Stats
    for (auto conv_group : converged_paths)
    {
        number_converged_path_counter += conv_group.size();
    }
    number_active_path_counter += paths_for_ml.size();
}

void Ensemble_Decoder::decode(const std::vector<double> &LLR_received)
{
    // store LLR for ML-criterion
    this->LLR_received = LLR_received;

    // Get diverse LLRs for diversity via LLR manipulation (e.g. NED, SBP) from processing_diversity
    // TODO: Make processed llrs decoder state to avoid allocation

    
    processing_diversity->get_LLR_variants(processed_llrs,LLR_received);

    // Allocate memory
    group_ordering_of_decoders = selector->select();
    auto group_shape = selector->get_group_shapes();

    assert(group_ordering_of_decoders.size() == group_shape[0]);

    converged_paths.clear();
    non_converged_paths.clear();

    converged_paths.resize(group_shape[0]);           // reserve shape group_shape[0] x group_shape[1]
    non_converged_paths.resize(group_shape[0]);       // reserve shape group_shape[0] x group_shape[1]
    ensemble_convergence_info.resize(group_shape[0]); // resize to  group_shape[0] x group_shape[1] (does this call the constructor again?)f

    for (size_t i = 0; i < group_shape[0]; i++)
    {
        converged_paths[i].reserve(group_shape[1]);
        non_converged_paths[i].reserve(group_shape[1]);

        ensemble_convergence_info[i].resize(group_shape[1]);
    }
    codeword_candidates.resize(ensemble_size);

    // default and baseline of ensemble
    this->decoder_conv_info.decoding_effort = 0;
    this->decoder_conv_info.converged = false;

    size_t idx_executed_group; // all groups are executed

    // each group of decoder paths is handled in parallel whereas groups are executed sequentially
    for (idx_executed_group = 0; idx_executed_group < group_shape[0]; idx_executed_group++)
    {
        execute_group(group_ordering_of_decoders[idx_executed_group], processed_llrs);
    }

    update_paths_for_ml(idx_executed_group);

    update_statistics_and_ensemble_effort(idx_executed_group);

    //* POST-PROCESS + ML*//
    // Post-process codeword candidates from decoding paths via processing_diversity (e.g., for diversity via post-processing)
    processing_diversity->post_process_codeword_candidates(codeword_candidates);

    // check prior to ml which of the converged (not necessarily active paths) have valid codeword
    //  used to compute path error rate
    ML_criterion();
}

/**
 * @brief Computes ML-metric between a codeword and received LLR vector
 *
 * @param codeword Binary codeword
 * @param LLR_received Received channel LLRs
 */
double Ensemble_Decoder::compute_ML_metric(const GF2::Vector &codeword, const std::vector<double> &LLR_received)
{

    if (codeword.size() != LLR_received.size())
        throw std::invalid_argument("compute_ML_metric: codeword and LLR_received must have the same size.");

    double metric = 0.0;
    for (size_t i = 0; i < codeword.size(); ++i)
    {
        if (codeword[i] == 1)
            metric -= LLR_received[i];
        else
            metric += LLR_received[i];
    }
    return metric;
}

/**
 * @brief Uses ML-criterion to select best codeword candidate
 */
void Ensemble_Decoder::ML_criterion()
{
    // Evaluate ML metric only over active decoding paths.
    // If no valid codeword is found among them,
    // fall back to all candidates to minimize BER.
    // Find codeword with max. ML-metric in ensemble:

    size_t dec_best = paths_for_ml[0];
    double max_metric = -DBL_MAX;
    double metric = 0.0;

    bool at_least_one_valid_codeword = false;
    for (auto dec : paths_for_ml)
    {
        // Skip invalid codewords
        if (!H.isOrthogonalTo(codeword_candidates[dec]))
        {
            continue;
        }

        // Only reaches here if at least one cw in the list is valid
        at_least_one_valid_codeword = true;
        metric = compute_ML_metric(codeword_candidates[dec], LLR_received);
        if (metric > max_metric)
        {
            max_metric = metric;
            dec_best = dec;
        }
    }
    if (!at_least_one_valid_codeword)
    {
        // if no
        //     valid;
        // frame error but select candidate with maximum metric to minimize bit errors
        for (size_t dec = 0; dec < codeword_candidates.size(); ++dec)
        {
            metric = compute_ML_metric(codeword_candidates[dec], LLR_received);
            if (metric > max_metric)
            {
                max_metric = metric;
                dec_best = dec;
            }
        }
    }

    codeword = codeword_candidates[dec_best];
}

//-------------------------------------------------------------------------------------------------
//                                      ENSEMBLE_DECODER_STATS
//-------------------------------------------------------------------------------------------------

void Ensemble_Decoder_Stats::get(const std::vector<std::unique_ptr<Decoder>> &multithreading_decoder_set, size_t trials, double snr)
{
    Decoder_Stats::get(multithreading_decoder_set, trials, snr);

    // Init vectors for summation of thread-local counters
    auto &ensemble = static_cast<Ensemble_Decoder &>(*multithreading_decoder_set[0]);

    size_t number_active_path_counter = 0;
    size_t number_converged_path_counter = 0;

    size_t ensemble_effort_count = 0;
    size_t ensemble_latency_count = 0;
    std::vector<size_t> decoding_effort_count(ensemble.ensemble_size, 0);

    // Sum & reset thread-local counters
    for (const auto &decoder_thread : multithreading_decoder_set)
    {
        auto &ensemble = static_cast<Ensemble_Decoder &>(*decoder_thread.get());

        ensemble_effort_count += ensemble.ensemble_decoding_effort_counter;
        ensemble_latency_count += ensemble.ensemble_latency_counter;

        number_active_path_counter += ensemble.number_active_path_counter;
        number_converged_path_counter += ensemble.number_converged_path_counter;

        for (size_t path = 0; path < ensemble.ensemble_size; ++path)
        {
            decoding_effort_count[path] += ensemble.path_decoding_effort_counter[path];
        }

        // Reset counters since read out
        ensemble.number_active_path_counter = 0;
        ensemble.number_converged_path_counter = 0;

        ensemble.ensemble_decoding_effort_counter = 0;
        ensemble.ensemble_latency_counter = 0;
        ensemble.path_decoding_effort_counter.assign(ensemble.ensemble_size, 0);
    }

    // counted per trial
    avg_number_converged_paths[snr] = (static_cast<double>(number_converged_path_counter) / static_cast<double>(trials));

    avg_ensemble_effort[snr] = (static_cast<double>(ensemble_effort_count) / static_cast<double>(trials));
    avg_ensemble_latency[snr] = (static_cast<double>(ensemble_latency_count) / static_cast<double>(trials));

    avg_number_active_paths[snr] = (static_cast<double>(number_active_path_counter) / static_cast<double>(trials));

    // Compute & store ensemble statistics
    for (size_t path = 0; path < ensemble.ensemble_size; ++path)
    {
        avg_decoding_effort_per_path[snr].push_back(static_cast<double>(decoding_effort_count[path]) / static_cast<double>(trials));
    }
}

void Ensemble_Decoder_Stats::save(double snr, const std::string &save_dir)
{
    Decoder_Stats::save(snr, save_dir);

    // Save stats in save_dir/decoder_stats.json
    std::string key = double_to_string(snr);
    update_json(save_dir + "/decoder_stats.json", [&](json &stats)
                {
        stats["average_number_converged_path"][key]=avg_number_converged_paths.at(snr);
        stats["average_ensemble_effort"][key]=avg_ensemble_effort.at(snr);
        stats["average_ensemble_latency"][key]=avg_ensemble_latency.at(snr);
        stats["average_number_active_path"][key]=avg_number_active_paths.at(snr);
        size_t ensemble_size = avg_decoding_effort_per_path.at(snr).size();
        for (size_t path = 0; path < ensemble_size; ++path)
        {
            stats["average_decoding_effort"][key][path] = avg_decoding_effort_per_path.at(snr)[path];
        } });
}