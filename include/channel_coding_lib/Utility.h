#ifndef UTILITY_H
#define UTILITY_H

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
//---------------------------------------------------------------------------------------------------------------------
//                                      CONSTANTS
//---------------------------------------------------------------------------------------------------------------------

constexpr double DBL_MAX = std::numeric_limits<double>::max();
constexpr double NEAR_ZERO = 1e-10;
constexpr size_t SIZE_T_UNSET = std::numeric_limits<size_t>::max();
//---------------------------------------------------------------------------------------------------------------------
//                                      CLIPPING
//---------------------------------------------------------------------------------------------------------------------

namespace safe
{
    // Numerical constants
    constexpr double TANH_UPPER_LIMIT = 20.0;
    constexpr double ATANH_LOWER_LIMIT = 1e-10;
    constexpr double PHI_UPPER_LIMIT = 35.0;
    constexpr double PHI_LOWER_LIMIT = 1.3e-15;

    // Numerical safe tanh
    constexpr inline double tanh(double x)
    {
        if (x > TANH_UPPER_LIMIT)
            return 1.0;
        else if (x < -TANH_UPPER_LIMIT)
            return -1.0;
        else
            return std::tanh(x);
    }

    // Numerical safe atanh
    constexpr inline double atanh(double x)
    {
        if (x >= 1.0)
            return std::atanh(1.0 - ATANH_LOWER_LIMIT);
        else if (x <= -1.0)
            return std::atanh(-1.0 + ATANH_LOWER_LIMIT);
        else
            return std::atanh(x);
    }

    // Numerical safe phi-function
    constexpr inline double phi_function(double tau)
    {
        if (tau >= PHI_UPPER_LIMIT)
            return 0.0;
        else if (tau <= PHI_LOWER_LIMIT)
            return 35.0;

        double exp_tau = std::exp(tau);
        double numerator = std::log(exp_tau + 1.0);
        double denumerator = std::log(exp_tau - 1.0);
        return numerator - denumerator;
    }
}

//---------------------------------------------------------------------------------------------------------------------
//                                      VECTOR OPERATIONS
//---------------------------------------------------------------------------------------------------------------------

// Returns inverse of a permutation set
inline std::vector<size_t> invPerm(const std::vector<size_t> &permutation)
{
    std::vector<size_t> inv_permutation(permutation.size());
    for (size_t ind = 0; ind < permutation.size(); ++ind)
        inv_permutation[permutation[ind]] = ind;
    return inv_permutation;
}

// Permutes elements in vector
template <typename T>
inline std::vector<T> permute(const std::vector<T> &vec, const std::vector<size_t> &permutation)
{
    if (permutation.size() != vec.size())
        throw std::invalid_argument("Size of vector and permutation must match!");
    std::vector<T> vec_permuted(vec.size());
    for (size_t ind = 0; ind < permutation.size(); ++ind)
        vec_permuted[ind] = vec[permutation[ind]];
    return vec_permuted;
}

//---------------------------------------------------------------------------------------------------------------------
//                                      OUTPUT
//---------------------------------------------------------------------------------------------------------------------

// Type trait to check if input is a vector
template <typename T>
struct is_vector : std::false_type
{
};
template <typename T>
struct is_vector<std::vector<T>> : std::true_type
{
};

// General print function
template <typename T>
inline void print(const T &value)
{
    // Input is a vector...
    if constexpr (is_vector<T>::value)
    {
        std::cout << "[";
        for (size_t i = 0; i < value.size(); ++i)
        {
            // Recursion to deal with nesting
            print(value[i]);
            if (i != value.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]";
    }
    // Input is a scalar...
    else
        std::cout << value;
}

//-------------------------------------------------------------------------------------------------
//                                      STRING OPERATIONS
//-------------------------------------------------------------------------------------------------

// Compares two strings case-insensitive (like boost::iequals)
inline bool iequals(const std::string &a, const std::string &b)
{
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b)
                      { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); });
}

// Convert double to string
inline std::string double_to_string(double dbl)
{
    std::ostringstream oss;
    oss << std::setprecision(10) << dbl;
    return oss.str();
}
//-------------------------------------------------------------------------------------------------
//                                      JSON
//-------------------------------------------------------------------------------------------------

// Load existing JSON file (return empty object if file is missing/corrupted)
inline json load_json(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return json::object();
    try
    {
        return json::parse(file);
    }
    catch (...)
    {
        return json::object();
    }
}

// Write JSON to file
inline void save_json(const std::string &path, const json &json)
{
    std::ofstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open " + path + " for writing!");
    file << json.dump(4) << "\n";
}

// Add entries into JSON
inline void update_json(const std::string &path, std::function<void(json &)> update)
{
    json stats = load_json(path);
    update(stats);
    save_json(path, stats);
}

//-------------------------------------------------------------------------------------------------
//                                      TXT
//-------------------------------------------------------------------------------------------------

// Overwrite lines of txt file containing keyword string with input line
inline void overwrite_txt(std::fstream &file, const std::string &keyword, const std::string &input_line)
{
    file.seekg(0, std::ios::beg);

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line))
    {
        // Overwrite line containing keyword with input
        if (line.find(keyword) != std::string::npos)
            line = input_line;
        lines.push_back(line);
    }

    file.clear();
    file.seekp(0, std::ios::beg);
    for (const auto &l : lines)
        file << l << "\n";
}

#endif // UTILITY_H