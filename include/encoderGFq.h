#ifndef ENCODER_GFQ_H
#define ENCODER_GFQ_H

#include <vector>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <cstddef>

// Assumes you already have:
// - invPerm(...)
// - Matrix / Vector types with the same API style as your GF2 classes
#include <channel_coding_lib/Utility.h>

template <typename MatrixT, typename VectorT>
class EncoderGFq
{
public:
    using Matrix = MatrixT;
    using Vector = VectorT;
    using elem_t = typename Matrix::value_type;

    // Default constructor
    EncoderGFq(Matrix& H, std::size_t k, std::size_t n, std::size_t min_col)
        : H_ALT(H), k(k), n(n), m(H.rows()), min_col(min_col), g(0)
    {
    }

    // Copy constructor
    EncoderGFq(const EncoderGFq&) = default;

    // Code parameters
    std::size_t n;        // Number of symbols in codeword
    std::size_t k;        // Number of data symbols
    std::size_t m;        // Number of rows in parity check matrix
    std::size_t min_col;  // Last column corresponding to punctured message symbol

    // Equivalent parity check matrix in ALT form
    // H_ALT = | A | B | T |
    //         | C | D | E |
    Matrix H_ALT;
    Matrix H_SALT;
    std::size_t g;  // gap

    std::vector<std::size_t> ALT_perm;
    std::vector<std::size_t> inv_ALT_perm;

    Matrix T_inv;
    Matrix E;

    // Precomputed matrices
    Matrix PreComp;
    Matrix TA;
    Matrix TB;

    // ALT/SALT preprocessing
    void ALT_deterministic();
    void get_ALT();
    void get_SALT();
    void rank_reduction();
    void remove_singularity();
    void preprocess();

    // Encoding
    Vector encode(const Vector& s);

private:
    static bool is_nonzero(const elem_t& x)
    {
        return x != elem_t{};
    }
};

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::ALT_deterministic()
{
    int right_bound = static_cast<int>(n) - 1;
    int lower_bound = static_cast<int>(m) - 1;

    std::vector<std::size_t> row_order(m);
    std::vector<std::size_t> upper_nonzeros(m);

    while (lower_bound >= 0)
    {
        std::size_t col_degree = 0;
        std::size_t min_col_degree = m + 1;
        std::size_t pivot_col = static_cast<std::size_t>(right_bound);

        for (int i = right_bound; i > static_cast<int>(min_col); --i)
        {
            col_degree = 0;
            for (int j = 0; j <= lower_bound; ++j)
            {
                if (is_nonzero(H_ALT(j, static_cast<std::size_t>(i))))
                    ++col_degree;
            }

            if (col_degree == 1)
            {
                min_col_degree = 1;
                pivot_col = static_cast<std::size_t>(i);
                break;
            }

            if (col_degree > 0 && col_degree < min_col_degree)
            {
                min_col_degree = col_degree;
                pivot_col = static_cast<std::size_t>(i);
            }
        }

        if (pivot_col != static_cast<std::size_t>(right_bound))
        {
            H_ALT.swapCols(pivot_col, static_cast<std::size_t>(right_bound));
            std::swap(ALT_perm[pivot_col], ALT_perm[static_cast<std::size_t>(right_bound)]);
        }

        std::size_t shift = 0;

        if (right_bound == static_cast<int>(n) - 1)
        {
            for (std::size_t j = 0; j < m; ++j)
            {
                if (!is_nonzero(H_ALT(j, static_cast<std::size_t>(right_bound))))
                {
                    row_order[j - shift] = j;
                }
                else
                {
                    row_order[m - min_col_degree + shift] = j;
                    ++shift;
                }
            }

            H_ALT.permuteRows(row_order);
            lower_bound = static_cast<int>(m - min_col_degree - 1);
        }
        else
        {
            std::size_t ind = 0;
            for (int j = 0; j <= lower_bound; ++j)
            {
                if (is_nonzero(H_ALT(static_cast<std::size_t>(j), static_cast<std::size_t>(right_bound))))
                {
                    upper_nonzeros[ind] = static_cast<std::size_t>(j);
                    ++ind;
                }
            }

            if (ind == 0)
            {
                --right_bound;
                continue;
            }

            const std::size_t lowest_upper_nonzero = upper_nonzeros[ind - 1];
            if (lowest_upper_nonzero != static_cast<std::size_t>(lower_bound))
            {
                H_ALT.swapRows(lowest_upper_nonzero, static_cast<std::size_t>(lower_bound));
            }

            if (min_col_degree > 1)
            {
                shift = 0;
                for (int j = 0; j < lower_bound; ++j)
                {
                    if (shift < min_col_degree - 1 && static_cast<std::size_t>(j) == upper_nonzeros[shift])
                    {
                        row_order[m - (min_col_degree - 1) + shift] = static_cast<std::size_t>(j);
                        ++shift;
                    }
                    else
                    {
                        row_order[j - static_cast<int>(shift)] = static_cast<std::size_t>(j);
                    }
                }

                for (int j = lower_bound; j < static_cast<int>(m); ++j)
                {
                    row_order[j - static_cast<int>(min_col_degree - 1)] = static_cast<std::size_t>(j);
                }

                H_ALT.permuteRows(row_order);
            }

            lower_bound -= static_cast<int>(min_col_degree);
        }

        --right_bound;
    }

    g = m - (n - static_cast<std::size_t>(right_bound + 1));
}

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::get_ALT()
{
    ALT_deterministic();
}

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::get_SALT()
{
    Matrix M = Matrix::Identity(m - g);
    M.appendCols(Matrix(m - g, g));

    Matrix M_sub = E * T_inv;
    M_sub.appendCols(Matrix::Identity(g));
    M.appendRows(M_sub);

    H_SALT = M * H_ALT;
}

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::rank_reduction()
{
    const std::size_t j_start = m - g;
    std::vector<std::size_t> del_rows;
    std::vector<bool> is_dependent(g, false);

    for (std::size_t pivot = j_start; pivot < m; ++pivot)
    {
        if (is_dependent[pivot - j_start])
            continue;

        if (H_SALT.row(pivot).isEqualTo(Vector(n)))
        {
            del_rows.push_back(pivot);
            is_dependent[pivot - j_start] = true;
        }

        for (std::size_t j = pivot + 1; j < m; ++j)
        {
            if (is_dependent[j - j_start])
                continue;

            if (H_SALT.row(j).isEqualTo(H_SALT.row(pivot)))
            {
                del_rows.push_back(j);
                is_dependent[j - j_start] = true;
            }
        }
    }

    H_SALT.deleteRows(del_rows);
    H_ALT.deleteRows(del_rows);

    g -= del_rows.size();
    m -= del_rows.size();
}

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::remove_singularity()
{
    const std::size_t j_start = m - g;
    const std::size_t i_start = n - m;

    for (std::size_t pivot = 0; pivot < g; ++pivot)
    {
        const std::size_t pivot_row = j_start + pivot;
        const std::size_t pivot_col = i_start + pivot;

        bool pivot_valid = true;

        if (!is_nonzero(H_SALT(pivot_row, pivot_col)))
        {
            pivot_valid = false;
        }
        else
        {
            for (std::size_t j = j_start; j < pivot_row; ++j)
            {
                if (is_nonzero(H_SALT(j, pivot_col)))
                {
                    pivot_valid = false;
                    break;
                }
            }
        }

        if (!pivot_valid)
        {
            std::size_t valid_col = static_cast<std::size_t>(-1);

            for (std::size_t i = 0; i < i_start; ++i)
            {
                bool col_valid = true;

                if (!is_nonzero(H_SALT(pivot_row, i)))
                {
                    col_valid = false;
                }
                else
                {
                    for (std::size_t j = j_start; j < pivot_row; ++j)
                    {
                        if (is_nonzero(H_SALT(j, i)))
                        {
                            col_valid = false;
                            break;
                        }
                    }
                }

                if (!col_valid)
                    continue;

                valid_col = i;
                break;
            }

            if (valid_col == static_cast<std::size_t>(-1))
            {
                throw std::runtime_error("Could not find valid column to remove singularity in D_SALT.");
            }

            H_SALT.swapCols(pivot_col, valid_col);
            H_ALT.swapCols(pivot_col, valid_col);
            std::swap(ALT_perm[pivot_col], ALT_perm[valid_col]);
        }
    }
}

template <typename MatrixT, typename VectorT>
void EncoderGFq<MatrixT, VectorT>::preprocess()
{
    ALT_perm.resize(n);
    std::iota(ALT_perm.begin(), ALT_perm.end(), 0);

    get_ALT();

    T_inv = H_ALT.slice(0, m - g, n - (m - g), n).inverse();

    if (g != 0)
    {
        E = H_ALT.slice(m - g, m, n - (m - g), n);

        get_SALT();
        rank_reduction();

        Matrix D_SALT = H_SALT.slice(m - g, m, n - m, n - (m - g));

        if (D_SALT.is_singular())
        {
            remove_singularity();
            D_SALT = H_SALT.slice(m - g, m, n - m, n - (m - g));
        }

        Matrix A = H_ALT.slice(0, m - g, 0, n - m);
        Matrix B = H_ALT.slice(0, m - g, n - m, n - (m - g));
        Matrix C = H_ALT.slice(m - g, m, 0, n - m);
        Matrix D_SALT_inv = D_SALT.inverse();
        E = H_ALT.slice(m - g, m, n - (m - g), n);

        PreComp = (D_SALT_inv * (E * (T_inv * A))) + (D_SALT_inv * C);
        TA = T_inv * A;
        TB = T_inv * B;
    }
    else
    {
        Matrix A = H_ALT.slice(0, m - g, 0, n - m);
        TA = T_inv * A;
    }

    inv_ALT_perm = invPerm(ALT_perm);
}

template <typename MatrixT, typename VectorT>
VectorT EncoderGFq<MatrixT, VectorT>::encode(const Vector& s)
{
    Vector codeword = s;

    if (g != 0)
    {
        Vector p1 = PreComp * s;
        Vector p2 = (TA * s) + (TB * p1);

        codeword.append(p1);
        codeword.append(p2);
    }
    else
    {
        Vector p2 = TA * s;
        codeword.append(p2);
    }

    codeword.permute(inv_ALT_perm);
    return codeword;
}

#endif // ENCODER_GFQ_H