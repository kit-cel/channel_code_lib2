#include <channel_coding_lib/Encoder/PCM_based_Encoder.h>

// INIT
void PCM_based_Encoder::init(const Encoder_config &config)
{
    const auto &pcm_based_config = static_cast<const PCM_Encoder_config &>(config);

    
    this->H_ALT = pcm_based_config.H;
    this->k = pcm_based_config.k;
    this->n = pcm_based_config.n;
    this->min_col = pcm_based_config.min_col;
    this->m = pcm_based_config.H.rows();

    this->preprocess();
}

//---------------------------------------------------------------------------------------------------------------------
//                                      ALT ALGORITHMS
//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Get approximate lower triangular (ALT) form of parity check matrix, using deterministic approach
 */
void PCM_based_Encoder::ALT_deterministic()
{
    // Bring parity check matrix in approximate lower triangular form, starting from the most right column
    int right_bound = n - 1; // Most right column that hasn't been processed yet
    int lower_bound = m - 1; // Row of main diagonal in column of right bound

    std::vector<size_t> row_order(m);  // Stores new row order for permutation
    std::vector<size_t> upper_ones(m); // Stores "ones" above main diagonal in pivot column

    while (lower_bound >= 0)
    {
        // Select column with smallest number of "ones" as pivot
        // (Exclude columns corresponding to a punctured message bit)
        size_t colDegree = 0;
        size_t min_col_degree = m + 1;
        size_t pivot_col = right_bound;
        for (int i = right_bound; i > min_col; --i)
        {
            // Get column degree within bounds
            colDegree = 0;
            for (int j = 0; j <= lower_bound; ++j)
                colDegree += H_ALT(j, i);

            // If column of degree 1 found, select as pivot and break
            if (colDegree == 1)
            {
                min_col_degree = 1;
                pivot_col = i;
                break;
            }

            // Update pivot column if column with less "ones" found
            if (colDegree > 0 && colDegree < min_col_degree)
            {
                min_col_degree = colDegree;
                pivot_col = i;
            }
        }

        // Permute pivot column to the most right column in current bounds
        if (pivot_col != right_bound)
        {
            H_ALT.swapCols(pivot_col, right_bound);
            std::swap(column_permutation[pivot_col], column_permutation[right_bound]);
        }

        // Process pivot column by performing row permutations
        size_t shift = 0;
        if (right_bound == n - 1)
        {
            // Set row order for permutation
            for (int j = 0; j < m; ++j)
            {
                if (H_ALT(j, right_bound) == 0)
                    row_order[j - shift] = j;
                else
                {
                    row_order[m - min_col_degree + shift] = j;
                    shift++;
                }
            }

            // Perform row permutation
            H_ALT.permuteRows(row_order);

            // Update lower bound for left-hand column in next iteration
            lower_bound = m - min_col_degree - 1;
        }
        else if (right_bound < n - 1)
        {
            // Get rows with "ones" on or above main diagonal in pivot column
            size_t ind = 0;
            for (int j = 0; j <= lower_bound; ++j)
            {
                if (H_ALT(j, right_bound) == 1)
                {
                    upper_ones[ind] = j;
                    ind++;
                }
            }

            // Fill main diagonal in pivot column with a "one" if empty
            size_t lowest_upper_one = upper_ones[ind - 1];
            if (lowest_upper_one != lower_bound)
                H_ALT.swapRows(lowest_upper_one, lower_bound);

            // Move any remaining "ones" from above main diagonal to the bottom of matrix
            if (min_col_degree > 1)
            {
                // Set row order
                for (int j = 0; j < lower_bound; ++j)
                {
                    if (j != upper_ones[shift])
                        row_order[j - shift] = j;
                    else
                    {
                        row_order[m - (min_col_degree - 1) + shift] = j;
                        if (shift < min_col_degree - 1)
                            shift++;
                    }
                }
                for (int j = lower_bound; j < m; ++j)
                    row_order[j - (min_col_degree - 1)] = j;

                // Perform row permutation
                H_ALT.permuteRows(row_order);
            }

            // Update lower bound for left-hand column in next iteration
            lower_bound -= min_col_degree;
        }

        // Finally, move right bound to left-hand column
        right_bound--;
    }

    // Compute resulting gap
    g = m - (n - (right_bound + 1));
}

//---------------------------------------------------------------------------------------------------------------------
//                                      PREPROCESSING
//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Wrapper for permoring a specified ALT algorithm
 */
void PCM_based_Encoder::get_ALT()
{
    // Perform any ALT algorithm
    ALT_deterministic();
}

/**
 * @brief Get systematic ALT form of parity check matrix
 */
void PCM_based_Encoder::get_SALT()
{
    // Init elimination matrix to clear "ones" in submatrix E
    GF2::Matrix M = GF2::Matrix::Identity(m - g);
    M.appendCols(GF2::Matrix(m - g, g));
    GF2::Matrix M_sub = E * T_inv;
    M_sub.appendCols(GF2::Matrix::Identity(g));
    M.appendRows(M_sub);

    // Perform gaussian elimination to get SALT form
    H_SALT = M * H_ALT;
}

/**
 * @brief Remove linear dependent rows from ALT and systematic ALT form
 */
void PCM_based_Encoder::rank_reduction()
{
    // Mark linear dependent rows below main diagonal
    size_t j_start = m - g;
    std::vector<size_t> del_rows;
    std::vector<bool> is_dependent(g, false);
    for (size_t pivot = j_start; pivot < m; ++pivot)
    {
        // Skip rows already marked as linear dependent
        if (is_dependent[pivot - j_start])
            continue;

        // Check if row is empty ("all-zero")
        if (H_SALT.row(pivot).isEqualTo(GF2::Vector(n)))
        {
            del_rows.push_back(pivot);
            is_dependent[pivot - j_start] = 1;
        }

        // Check for identical rows below
        for (size_t j = pivot + 1; j < m; ++j)
        {
            if (is_dependent[j - j_start])
                continue;
            if (H_SALT.row(j).isEqualTo(H_SALT.row(pivot)))
            {
                del_rows.push_back(pivot);
                is_dependent[j - j_start] = 1;
            }
        }
    }

    // Delete marked rows
    H_SALT.deleteRows(del_rows);
    H_ALT.deleteRows(del_rows);

    // Update parameters of matrix
    g -= del_rows.size();
    m -= del_rows.size();
}

/**
 * @brief Make D_SALT non-singular by permuting columns of C and D
 */
void PCM_based_Encoder::remove_singularity()
{
    // Start indices of submatrix D_SALT
    size_t j_start = m - g;
    size_t i_start = n - m;

    for (size_t pivot = 0; pivot < g; ++pivot)
    {
        size_t pivot_row = j_start + pivot;
        size_t pivot_col = i_start + pivot;

        // Check if pivot column is valid for non-singular D_SALT
        bool pivot_valid = true;
        if (H_SALT(pivot_row, pivot_col) == 0)
            pivot_valid = false;
        else
            for (size_t j = j_start; j < pivot_row; ++j)
                if (H_SALT(j, pivot_col) == 1)
                {
                    pivot_valid = false;
                    break;
                }

        // If pivot column is not valid, permute with a valid one
        if (!pivot_valid)
        {
            // Find a valid column
            size_t valid_col = -1;
            for (size_t i = 0; i < i_start; ++i)
            {
                // If column isn't valid, move to next one
                bool col_valid = true;
                if (H_SALT(pivot_row, i) == 0)
                    col_valid = false;
                else
                    for (size_t j = j_start; j < pivot_row; ++j)
                        if (H_SALT(j, i) == 1)
                        {
                            col_valid = false;
                            break;
                        }
                if (!col_valid)
                    continue;

                // Otherwise, get column index and stop
                valid_col = i;
                break;
            }

            // Permute
            H_SALT.swapCols(pivot_col, valid_col);
            H_ALT.swapCols(pivot_col, valid_col);
            std::swap(column_permutation[pivot_col], column_permutation[valid_col]);
        }
    }
}

/**
 * @brief Preprocess parity check matrix for encoding
 *
 * Get parity check matrix into systematic ALT form and perform all pre-computations
 * to allow for efficient systematic encoding.
 */
void PCM_based_Encoder::preprocess()
{
    // Init vector for tracking column permutations
    column_permutation.resize(n);
    std::iota(column_permutation.begin(), column_permutation.end(), 0);

    get_ALT();

    T_inv = H_ALT.slice(0, m - g, n - (m - g), n).inverse();

    if (g != 0)
    {
        E = H_ALT.slice(m - g, m, n - (m - g), n);

        // Get D_SALT = -E*T_inv*B + D
        get_SALT();
        rank_reduction();
        GF2::Matrix D_SALT = H_SALT.slice(m - g, m, n - m, n - (m - g));

        // Make D_SALT non-singular if not already
        if (D_SALT.is_singular())
        {
            remove_singularity();
            D_SALT = H_SALT.slice(m - g, m, n - m, n - (m - g));
        }

        // Get submatrices for encoding. T_inv didn't change
        GF2::Matrix A = H_ALT.slice(0, m - g, 0, n - m);
        GF2::Matrix B = H_ALT.slice(0, m - g, n - m, n - (m - g));

        GF2::Matrix C = H_ALT.slice(m - g, m, 0, n - m);
        GF2::Matrix D_SALT_inv = D_SALT.inverse();
        E = H_ALT.slice(m - g, m, n - (m - g), n);

        // Pre-compute for encoding
        PreComp = (D_SALT_inv * (E * (T_inv * A))) + (D_SALT_inv * C);
        TA = T_inv * A;
        TB = T_inv * B;
    }
    else
    {
        // If ALT form has no gap, systematic ALT form is unnecessary

        // Only upper submatrices A & T_inv are existent
        GF2::Matrix A = H_ALT.slice(0, m - g, 0, n - m);

        // Pre-compute for encoding
        TA = T_inv * A;
    }

    // Get column permutation from H_ALT to original H
    inv_column_permutation = invPerm(column_permutation);
}

//---------------------------------------------------------------------------------------------------------------------
//                                      ENCODING
//---------------------------------------------------------------------------------------------------------------------

/**
 * @brief Encode data into a valid codeword
 *
 * Get valid codeword x = (s, p1, p2) by generating parity bits in p1 & p2 according
 * to data bits in s. p1 contains the first g parity bits and is therefore non-existent
 * for g = 0. p2 contains the remaining m-g parity bits.
 */
GF2::Vector PCM_based_Encoder::encode(const GF2::Vector &s)
{
    GF2::Vector codeword = s;

    if (g != 0)
    {
        // Get p1 according to s
        GF2::Vector p1 = PreComp * s;

        // Get p2 according to s & p1
        GF2::Vector p2 = (TA * s) + (TB * p1);

        // Assemble codeword
        codeword.append(p1);
        codeword.append(p2);
    }
    else
    {
        // Get p2 according to s
        GF2::Vector p2 = TA * s;

        // Assemble codeword
        codeword.append(p2);
    }

    // Permute codeword to be valid for original H

    codeword.permute(inv_column_permutation);
    return codeword;
}
