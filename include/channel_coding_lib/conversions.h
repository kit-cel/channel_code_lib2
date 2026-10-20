#include <GF2/GF2.h>

// //-------------------------------------------------------------------------------------------------------------
// //                                      CONVERSIONS
// //-------------------------------------------------------------------------------------------------------------

// // Cast python input into bit-matrix.
// constexpr Matrix_ cast(pybind11::array_t<int> &pyArray) {
//   auto input = pyArray.request();
//   assert(("Input array must be 2D.", input.ndim == 2));

//   Matrix_ mat(input.shape[0], input.shape[1]);

//   // Fill in bits one by one
//   auto input_vals = pyArray.unchecked<2>();
//   for (size_t j = 0; j < mat.rows(); ++j)
//     for (size_t i = 0; i < mat.cols(); ++i)
//       mat(j, i) = input_vals(j, i);

//   return mat;
// }
// //-------------------------------------------------------------------------------------------------------------
// //                                      CONVERSIONS
// //-------------------------------------------------------------------------------------------------------------

// // Cast python input into bit-vector.
// constexpr Vector_ cast(const pybind11::array_t<int> &pyArray) {
//   auto input = pyArray.request();
//   if (input.ndim != 1)
//     throw std::invalid_argument("Input array must be 1D.");

//   Vector_ vec(input.shape[0]);

//   // Fill in bits one by one.
//   auto input_vals = pyArray.unchecked<1>();
//   for (size_t i = 0; i < vector_size; ++i)
//     vec[i] = input_vals[i];

//   return vec;
// }

// -------------------------
// Helpers: numpy <-> GF2
// -------------------------

inline GF2::Vector stdvec_to_gf2_vector(const std::vector<int> &v)
{
    GF2::Vector out(v.size());
    for (size_t i = 0; i < v.size(); ++i)
        out(i) = (v[i] & 1);
    return out;
}

inline std::vector<int> gf2_vector_to_stdvec(const GF2::Vector &v)
{
    std::vector<int> out(v.size());
    for (size_t i = 0; i < v.size(); ++i)
        out[i] = static_cast<int>(v(i)); // const access returns bool
    return out;
}

inline std::vector<int> gf2_vector_to_stdvec_anysize(const GF2::Vector &v)
{

    return gf2_vector_to_stdvec(v);
}