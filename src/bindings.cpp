#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

#include <channel_coding_lib/Simulation_Env.h>
#include <channel_coding_lib/Encoder/Encoder.h>
#include <channel_coding_lib/Decoder/Decoder_Factory.h>
#include <channel_coding_lib/conversions.h>

struct ParityMatrix
{
    GF2::Matrix H;
    size_t rows;
    size_t cols;
};
/*
static GF2::Matrix numpy_to_gf2_matrix(pybind11::array_t<int>& H_np)
{
    auto buf = H_np.request();
    if (buf.ndim != 2)
        throw std::invalid_argument("H must be a 2D numpy array");

    const size_t rows = static_cast<size_t>(buf.shape[0]);
    const size_t cols = static_cast<size_t>(buf.shape[1]);

    GF2::Matrix H(rows, cols);

    auto H_u = H_np.unchecked<2>();
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            H(i, j) = (H_u(i, j) & 1);   // force 0/1

    return H;
}*/

static ParityMatrix numpy_to_parity_matrix(const pybind11::array_t<int> &H_np)
{
    auto buf = H_np.request();
    if (buf.ndim != 2)
        throw std::invalid_argument("H must be a 2D numpy array");

    const size_t rows = static_cast<size_t>(buf.shape[0]);
    const size_t cols = static_cast<size_t>(buf.shape[1]);

    GF2::Matrix H(rows, cols);

    auto H_u = H_np.unchecked<2>();
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            H(i, j) = (H_u(i, j) & 1);

    return ParityMatrix{std::move(H), rows, cols};
}
static GF2::Matrix numpy_to_gf2_matrix(const pybind11::array_t<int> &H_np)
{
    return numpy_to_parity_matrix(H_np).H;
}

PYBIND11_MODULE(channel_coding, m)
{
    py::class_<Simulation_Env>(m, "Simulation_Env")
        .def(py::init([](
                          size_t k,
                          size_t n,
                          const std::string &bit_comparison_scope)
                      { return Simulation_Env(k, n, bit_comparison_scope); }),
             py::arg("k"),
             py::arg("n"),
             py::arg("bit_comparison_scope"))

        .def("puncturing", &Simulation_Env::puncturing, py::arg("bits_to_puncture"))
        .def("all_zero_init", &Simulation_Env::all_zero_init)
        .def("init", &Simulation_Env::init)
        .def("get_error_rates", &Simulation_Env::get_error_rates, py::arg("SNR"))
        .def_readwrite("SNR_type", &Simulation_Env::SNR_type)
        .def_readwrite("target_errors", &Simulation_Env::target_errors)
        .def_readwrite("max_transmissions", &Simulation_Env::max_transmissions)
        .def_readonly("error_rates", &Simulation_Env::error_rates)
        .def_readonly("H", &Simulation_Env::H)
        .def_readonly("k", &Simulation_Env::k)
        .def_readonly("n", &Simulation_Env::n)
        .def_readonly("k_transmitted", &Simulation_Env::k_transmitted)
        .def_readonly("n_transmitted", &Simulation_Env::n_transmitted)
        .def_readonly("r", &Simulation_Env::r)
        .def_readonly("bit_comparison_scope", &Simulation_Env::bit_comparison_scope)
        .def_readwrite("auto_save", &Simulation_Env::auto_save)
        .def_readwrite("save_dir", &Simulation_Env::save_dir);

    // PROCESSING DIVERSITY CONFIGS
    py::class_<Processing_config, std::shared_ptr<Processing_config>>(m, "Processing_config");
    py::class_<Identity_config, Processing_config, std::shared_ptr<Identity_config>>(m, "Identity_config")
        .def(py::init<>());
    py::class_<Automorphism_config, Processing_config, std::shared_ptr<Automorphism_config>>(m, "Automorphism_config")
        .def(py::init<
                 std::vector<std::vector<size_t>>,
                 std::vector<std::vector<size_t>>>(),
             py::arg("permutation_automorphisms"),
             py::arg("inverse_permutation_automorphisms"))
        .def(py::init<
                 std::vector<std::vector<size_t>>>(),
             py::arg("permutation_automorphisms"));

    // DECODER CONFIGS
    py::class_<Decoder_config, std::shared_ptr<Decoder_config>>(m, "Decoder_config");
    py::class_<BP_config, Decoder_config, std::shared_ptr<BP_config>>(m, "BP_config")
        .def(py::init([](const py::array_t<int> &H_np)
                      { return BP_config(numpy_to_gf2_matrix(H_np)); }))
        // Core BP parameters
        .def_readwrite("early_stopping", &BP_config::early_stopping)
        .def_readwrite("max_iterations", &BP_config::max_iterations)
        .def_readwrite("cn_update_type", &BP_config::cn_update_type)
        .def_readwrite("scheduling_type", &BP_config::scheduling_type)

        // affine offset
        .def_readwrite("affine_offset", &BP_config::affine_offset)

        //
        .def_readwrite("use_avns", &BP_config::use_avns)

        // Scheduling
        .def_readwrite("schedule", &BP_config::schedule)
        .def_readwrite("nodes_in_layer", &BP_config::nodes_in_layer)
        .def_readwrite("Z", &BP_config::Z)

        // MSA
        .def_readwrite("norm_factor", &BP_config::norm_factor);
    py::class_<Ensemble_config, Decoder_config, std::shared_ptr<Ensemble_config>>(m, "Ensemble_config")
        .def(py::init([](const py::array_t<int> &H_ML, const std::vector<std::shared_ptr<Decoder_config>> config_set)
                      { return Ensemble_config(numpy_to_gf2_matrix(H_ML), config_set); }))
        .def(py::init([](const py::array_t<int> &H_ML, const std::vector<std::shared_ptr<Decoder_config>> config_set, const std::shared_ptr<Processing_config> processing_config)
                      { return Ensemble_config(numpy_to_gf2_matrix(H_ML), config_set, processing_config); }));

    // ENCODER CONFIGS
    py::class_<Encoder_config, std::shared_ptr<Encoder_config>>(m, "Encoder_config");
    py::class_<PCM_Encoder_config, Encoder_config, std::shared_ptr<PCM_Encoder_config>>(m, "PCM_Encoder_config")
        .def(py::init([](const py::array_t<int> &H, size_t k, size_t n)
                      { return PCM_Encoder_config(numpy_to_gf2_matrix(H), k, n); }));
    py::class_<G_Encoder_config, Encoder_config, std::shared_ptr<G_Encoder_config>>(m, "G_Encoder_config")
        .def(py::init([](const py::array_t<int> &G, size_t k, size_t n)
                      { return G_Encoder_config(numpy_to_gf2_matrix(G), k, n); }));
    // py::class_<Encoder>(m, "Encoder")
    //     .def(py::init([](const py::array_t<int> &H_np, size_t k, size_t n, size_t min_col)
    //                   {
    //              ParityMatrix pm = numpy_to_parity_matrix(H_np);
    //              Encoder enc(pm.H, k, n, min_col);
    //              enc.preprocess();
    //              return enc; }),
    //          py::arg("H"),
    //          py::arg("k"),
    //          py::arg("n"),
    //          py::arg("min_col") = 0)

    //     .def("encode", [](Encoder &enc, const std::vector<int> &s)
    //          {
    //              GF2::Vector s_gf2 = stdvec_to_gf2_vector(s);
    //              GF2::Vector c_gf2 = enc.encode(s_gf2);
    //              return gf2_vector_to_stdvec(c_gf2); }, py::arg("s"))

    //     .def_readonly("k", &Encoder::k)
    //     .def_readonly("n", &Encoder::n)
    //     .def_readonly("m", &Encoder::m)
    //     .def_readonly("min_col", &Encoder::min_col);

    // py::class_<Decoder, std::shared_ptr<Decoder>>(m, "Decoder")
    //     .def("init", [](std::shared_ptr<Decoder> d, const py::array_t<int>& H_np) {
    //             ParityMatrix pm = numpy_to_parity_matrix(H_np);
    //             d->init(pm.H);
    //         },
    //         py::arg("H"))

    //     .def("decode", [](std::shared_ptr<Decoder> d, const std::vector<double>& llr, size_t max_iterations) {
    //             d->decode(llr, max_iterations);
    //         },
    //         py::arg("LLR_received"), py::arg("max_iterations"))

    //     .def("syndrome_is_zero", &Decoder::syndrome_is_zero)
    //     // .def_readwrite("early_stopping", &Decoder::early_stopping)
    //     // .def_readwrite("Z", &Decoder::Z)
    //     .def_readonly("n", &Decoder::n)
    //     .def_readonly("m", &Decoder::m)
    //     .def("get_codeword", [](std::shared_ptr<Decoder> d) {
    //             return gf2_vector_to_stdvec_anysize(d->codeword);
    //         });

    // auto dec = m.def_submodule("decoders", "Decoder constructors / factory");

    // // Python-visible decoder classes
    // //

    // // BPDecoder: Sum-Product Algorithm with flooding schedule ("bp-spa")

    // // // BPFlooding: same as bp-spa in your factory (flooding schedule)
    // // py::class_<Decoder, std::shared_ptr<Decoder>>(dec, "BPFlooding")
    // //     .def(py::init([]() {
    // //         Decoder_Factory f;
    // //         f.decoder_type = "bp-spa";   // flooding schedule is used here
    // //         return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // //     }));

    // // // BPRowLayered: row-layered schedule with SPA ("row-layered bp-spa")
    // // py::class_<Decoder, std::shared_ptr<Decoder>>(dec, "BPRowLayered")
    // //     .def(py::init([]() {
    // //         Decoder_Factory f;
    // //         f.decoder_type = "row-layered bp-spa";
    // //         return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // //     }));

    // // // (Optional) BPMinSum
    // // py::class_<Decoder, std::shared_ptr<Decoder>>(dec, "BPMinSum")
    // //     .def(py::init([]() {
    // //         Decoder_Factory f;
    // //         f.decoder_type = "bp-minsum";
    // //         return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // //     }));

    // // Canonical list (kept in one place)
    // dec.def("supported_decoders", []() {
    //     return std::vector<std::string>{
    //         "bp-spa",
    //         "bp-spa-phi",
    //         "bp-minsum",
    //         "row-layered bp-spa",
    //         "row-layered bp-spa-phi",
    //         "row-layered bp-minsum",
    //         "column-layered bp"
    //     };
    // });

    // // Main factory: mirrors Decoder_Factory options
    // dec.def("make",
    //     [](const std::string& decoder_type,
    //        const std::string& ensemble_type,
    //        size_t ensemble_size,
    //        size_t num_checks_removed,
    //        double sigma_NED,
    //        double saturation_value)
    //     {
    //         Decoder_Factory f;
    //         f.decoder_type = decoder_type;

    //         f.ensemble_type = ensemble_type;
    //         f.ensemble_size = ensemble_size;

    //         f.num_checks_removed = num_checks_removed;  // AED
    //         f.sigma_NED = sigma_NED;                    // NED
    //         f.saturation_value = saturation_value;      // SBP

    //         std::unique_ptr<Decoder> up = f.createDecoder();

    //         // convert unique_ptr -> shared_ptr for pybind polymorphism
    //         return std::shared_ptr<Decoder>(std::move(up));
    //     },
    //     py::arg("decoder_type"),
    //     py::arg("ensemble_type") = "None",
    //     py::arg("ensemble_size") = 1,
    //     py::arg("num_checks_removed") = 0,
    //     py::arg("sigma_NED") = 0.0,
    //     py::arg("saturation_value") = 10.0
    // );

    // // Convenience constructors (optional but matches the issue “feel”)
    // dec.def("BP_decoder", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "bp-spa";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });

    // dec.def("BP_decoder_spa", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "bp-spa";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });

    // dec.def("BP_decoder_minsum", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "bp-minsum";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });

    // dec.def("BP_decoder_row_layered_spa", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "row-layered bp-spa";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });

    // dec.def("BP_decoder_row_layered_minsum", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "row-layered bp-minsum";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });

    // dec.def("BP_decoder_column_layered", []() {
    //     Decoder_Factory f;
    //     f.decoder_type = "column-layered bp";
    //     return std::shared_ptr<Decoder>(std::move(f.createDecoder()));
    // });
}
