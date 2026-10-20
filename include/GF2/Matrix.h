#ifndef GF2_MATRIX_H
#define GF2_MATRIX_H

#include "Vector.h"

namespace GF2
{
    template <std::unsigned_integral Block = uint64_t, typename Allocator = std::allocator<Block>>
    class Matrix_
    {
    private:

        using VectorType = Vector_<Block, Allocator>;    // Pass template parameters to vector type used in matrix

        std::vector<VectorType> vector_set;     // Set of bit-vectors representing rows of matrix



        // Enable cross-template friendship between bit-matrix instances.
        template <std::unsigned_integral BlockType, typename AllocatorType>
        friend class Matrix_;

    public:

        //-------------------------------------------------------------------------------------------------------------
        //                                  CONSTRUCTORS & FACTORIES
        //-------------------------------------------------------------------------------------------------------------

        // Default constructor for bit-matrix.
        constexpr explicit Matrix_(size_t rows = 0, size_t cols = 0) 
        { 
            if (rows != 0 && cols != 0) resize(rows, cols); 
        };

        // Factory method for an identity matrix.
        static constexpr Matrix_ Identity(size_t size)
        {
            Matrix_ I(size, size);
            for (size_t j = 0; j < size; ++j)
                I(j, j) = 1;
            return I;
        }

        // factory method for an "all-one" matrix.
        static constexpr Matrix_ Ones(size_t rows, size_t cols)
        {
            Matrix_ mat(rows, cols);
            mat.ones();
            return mat;
        }



        //-------------------------------------------------------------------------------------------------------------
        //                                      MODIFIERS & ARITHMETICS
        //-------------------------------------------------------------------------------------------------------------

        // Resize bit-matrix. Extend by adding "zeros".
        constexpr Matrix_& resize(size_t new_rows, size_t new_cols)
        {
            // Check for change
            if (rows() == new_rows && cols() == new_cols) return *this;

            // Adjust number of vectors if needed
            if (rows() != new_rows) vector_set.resize(new_rows);

            // Adjust vector sizes
            for (size_t j = 0; j < new_rows; ++j)
                row(j).resize(new_cols);

            return *this;
        }

        // Return transpose of matrix.
        constexpr Matrix_ transpose() const
        {
            Matrix_ mat_T(cols(), rows());
            for (size_t j = 0; j < cols(); ++j)
                for (size_t i = 0; i < rows(); ++i)
                    mat_T(j, i) = (*this)(i, j);
            return mat_T;
        }

        // Return inverse of matrix.
        constexpr Matrix_ inverse() const
        {
            if (cols() != rows()) throw std::invalid_argument("Matrix must be square.");

            size_t m = rows();
             
            // Create augmented matrix
            Matrix_ mat_inv(*this); 
            mat_inv.appendCols(Matrix_::Identity(m));

            mat_inv.setRowCanonical();

            if (mat_inv.slice(0, m, 0, m).is_identity()) return mat_inv.slice(0, m, m, 2*m);
            else throw std::invalid_argument("Matrix must be non-singular.");
        }

        // Transform a square matrix into an identity matrix.
        constexpr Matrix_& setIdentity()
        {
            if (rows() != cols()) throw std::invalid_argument("Matrix must be square.");

            zeros();
            for (size_t j = 0; j < rows(); ++j)
                vector_set[j][j] = 1;
            return *this;
        }

        // Set all bits of matrix to "zero".
        constexpr Matrix_& zeros()
        {
            for (size_t j = 0; j < rows(); ++j)
                row(j).zeros();
            return *this;
        }

        // Set all bits of matrix to "one".
        constexpr Matrix_& ones()
        {
            for (size_t j = 0; j < rows(); ++j)
                row(j).ones();
            return *this;
        }

        // Swap positions of two rows in matrix.
        constexpr Matrix_& swapRows(size_t j_1, size_t j_2)
        {
            row(j_1).swapVectors(row(j_2));
            return *this;
        }

        // Swap positions of two columns in matrix.
        constexpr Matrix_& swapCols(size_t i_1, size_t i_2)
        {
            for (size_t j = 0; j < rows(); ++j)
                row(j).swapBits(i_1, i_2);
            return *this;
        }

        // Set target column of matrix to given bit-vector.
        constexpr Matrix_& setCol(size_t i, VectorType vec)
        {
            assert(("Input vector must be of same size as column.", vec.size() == rows()));
            for (size_t j = 0; j < rows(); ++j)
                vector_set[j][i] = vec[j];
            return *this;
        }

        // Permute rows of matrix based on given row-order.
        template <typename unsigned_int>
        constexpr Matrix_& permuteRows(std::vector<unsigned_int> row_order)
        {
            static_assert((std::is_unsigned<unsigned_int>::value, "Input-vector must contain unsigned integers."));

            if (row_order.size() > rows()) throw std::invalid_argument("Number of elements in input-vector can't exceed number of rows in matrix.");

            Matrix_ original_mat(*this);
            if (row_order.size() != rows()) resize(row_order.size(), cols());
            for (size_t j = 0; j < rows(); ++j)
                row(j) = original_mat.row(row_order[j]);
            return *this;
        }

        // Permute cols of matrix based on given column-order.
        template <typename unsigned_int>
        constexpr Matrix_& permuteCols(std::vector<unsigned_int> col_order)
        {
            static_assert((std::is_unsigned<unsigned_int>::value, "Input-vector must contain unsigned integers."));

            if (col_order.size() > cols()) throw std::invalid_argument("Number of elements in input-vector can't exceed number of columns in matrix.");

            Matrix_ original_mat(*this);
            if (col_order.size() != cols()) resize(rows(), col_order.size());
            for (size_t i = 0; i < cols(); ++i)
                setCol(i, original_mat.col(col_order[i]));
            return *this;
        }

        // Bring matrix in row echelon form.
        constexpr Matrix_& setRowEchelon()
        {
            for (size_t i = 0; i < cols(); ++i)
            {
                // Select first row below main diagonal with "one" in column as pivot
                size_t pivot_row = rows();  // Init as out of bounds
                for (size_t j = i; j < rows(); ++j)
                {
                    if((*this)(j, i) == 1)
                    {
                        pivot_row = j;
                        break;
                    }
                }

                // Skip empty columns (no pivot found)
                if (pivot_row >= rows()) continue;

                // Permute pivot row to main diagonal
                if (pivot_row != i) swapRows(pivot_row, i);

                // Use pivot row to eliminate entries below
                for (size_t j = i + 1; j < rows(); ++j)
                    if ((*this)(j, i) == 1)
                        row(j) ^= row(i);
            }
            return *this;
        }

        // Bring matrix in reduced row echelon form.
        constexpr Matrix_& setRowCanonical()
        {
            setRowEchelon();

            for (size_t pivot_row = rows(); pivot_row-- > 0;)
            {
                // Select "ones" on main diagonal as pivot element
                size_t pivot_col = cols();  // Init as out of bounds
                for (size_t i = 0; i < cols(); ++i)
                {
                    if ((*this)(pivot_row, i) == 1)
                    {
                        pivot_col = i;
                        break;
                    }
                }

                // Skip empty rows (no pivot found)
                if (pivot_col >= cols()) continue;

                // Use pivot row to eliminate "ones" above pivot element
                for (size_t j = 0; j < pivot_row; ++j)
                    if ((*this)(j, pivot_col) == 1)
                        row(j) ^= row(pivot_row);
            }
            return *this;
        }

        // Append another bit-matrix from the right.
        constexpr Matrix_& appendCols(const Matrix_& other_mat)
        {
            if (rows() != other_mat.rows()) throw std::invalid_argument("Matrices must have same number of rows.");

            for (size_t j = 0; j < rows(); ++j)
                row(j).append(other_mat.row(j));

            return *this;
        }

        // Append another bit-matrix from below.
        constexpr Matrix_& appendRows(const Matrix_& other_mat)
        {
            if (cols() != other_mat.cols()) throw std::invalid_argument("Matrices must have same number of columns.");

            size_t old_rows = rows();
            resize(old_rows + other_mat.rows(), cols());
            for (size_t j = 0; j < other_mat.rows(); ++j)
                row(old_rows + j) = other_mat.row(j);
            
            return *this;
        }

        // Delete row from matrix.
        constexpr Matrix_& deleteRow(size_t j)
        {
            if (j >= rows()) throw std::invalid_argument("Index must be within row-size of matrix.");

            vector_set.erase(vector_set.begin() + j);
            return *this;
        }

        // Delete multiple rows from matrix.
        template <typename unsigned_int>
        constexpr Matrix_& deleteRows(std::vector<unsigned_int> del_rows)
        {
            static_assert((std::is_unsigned<unsigned_int>::value, "Input-vector must contain unsigned integers."));

            // Delte rows by excluding targets from row-order
            std::vector<unsigned_int> row_order(rows() - del_rows.size());
            size_t shift = 0;
            for (size_t j = 0; j < rows(); ++j)
            {
                bool delete_row = false;
                for (size_t ind = 0; ind < del_rows.size(); ++ind)
                    if (j == del_rows[ind])
                    {
                        delete_row = true;
                        shift++;
                        break;
                    }
                if (!delete_row)
                    row_order[j - shift] = j;            
            }
            permuteRows(row_order);

            return *this;
        }

        // Delete multiple cols from matrix.
        template <typename unsigned_int>
        constexpr Matrix_& deleteCols(std::vector<unsigned_int> del_cols)
        {
            static_assert((std::is_unsigned<unsigned_int>::value, "Input-vector must contain unsigned integers."));

            // Delte cols by excluding targets from col-order 
            std::vector<unsigned_int> col_order(cols() - del_cols.size());
            size_t shift = 0;
            for (size_t i = 0; i <  cols(); ++i)
            {
                bool delete_col = false;
                for (size_t ind = 0; ind < del_cols.size(); ++ind)
                    if (i == del_cols[ind])
                    {
                        delete_col = true;
                        shift++;
                        break;
                    }
                if (!delete_col)
                    col_order[i - shift] = i;            
            }
            permuteCols(col_order);

            return *this;
        }

        // Slice sub-matrix from matrix.
        constexpr Matrix_ slice(size_t j_start, size_t j_end, size_t i_start, size_t i_end) const
        {
            if (j_end > rows() || i_end > cols())
                throw std::invalid_argument("Indices must be within matrix dimensions.");
            if (j_start > j_end || i_start > i_end) 
                throw std::invalid_argument("Lower bounds can't exceed upper bounds.");
            
            Matrix_ sub_mat(j_end - j_start, i_end - i_start);
            for (size_t j = j_start; j < j_end; ++j)
                sub_mat.row(j - j_start) = row(j).slice(i_start, i_end);
            return sub_mat;
        }

        // Return true if matrix is identity.
        constexpr bool is_identity() const
        {
            if (rows() != cols()) return false;
            for (size_t j = 0; j < rows(); ++j)
                for (size_t i = 0; i < cols(); ++i)
                {
                    if (i == j) 
                        if ((*this)(j, i) == 0) return false;
                    else if (i != j)
                        if ((*this)(j, i) == 1) return false;
                }
            return true;
        }

        // Return true if matrix is singular.
        constexpr bool is_singular() const
        {
            if (cols() != rows()) return true;

            size_t m = rows();
             
            // Create augmented matrix
            Matrix_ mat_inv(*this); 
            mat_inv.appendCols(Matrix_::Identity(m));

            mat_inv.setRowCanonical();

            return !mat_inv.slice(0, m, 0, m).is_identity();
        }



        //-------------------------------------------------------------------------------------------------------------
        //                                      ACCESSORS
        //-------------------------------------------------------------------------------------------------------------

        // Return number of rows.
        constexpr size_t rows() const { return vector_set.size(); }

        // Return number of columns.
        constexpr size_t cols() const { return vector_set[0].size(); }

        // Return number of bits per block.
        constexpr size_t blockType() const { return vector_set[0].blockType(); }

        // Read-write access target row.
        constexpr VectorType& row(size_t j)
        {
            assert(("Index must be within row-size of matrix.", j < rows()));
            return vector_set[j];
        }

        // Read-only access target row.
        constexpr const VectorType row(size_t j) const
        {
            assert(("Index must be within row-size of matrix.", j < rows()));
            return vector_set[j];           
        }

        // Assign [] operator to row-accessor.
        constexpr VectorType& operator[](size_t j) { return row(j); }
        constexpr const VectorType operator[](size_t j) const { return row(j); }

        // Read-only access target column.
        // As columns aren't represented as vectors, returning 
        // a reference for write access is not possible.
        constexpr VectorType col(size_t i) const
        {
            assert(("Index must be within column-size of matrix.", i < cols()));
            VectorType col(rows());
            for (size_t j = 0; j < rows(); ++j)
                col[j] = vector_set[j][i];
            return col;
        }

        // Assign () operator to bit-accessor.
        constexpr VectorType::Reference operator()(size_t j, size_t i) { return vector_set[j][i]; }
        constexpr const bool operator()(size_t j, size_t i) const { return vector_set[j][i]; }

        // Return number of "ones" in target row.
        constexpr size_t rowDegree(size_t j) const { return row(j).degree(); }

        // Return number of "ones" in target column.
        constexpr size_t colDegree(size_t i) const { return col(i).degree(); }



        //-------------------------------------------------------------------------------------------------------------
        //                                      OUTPUTS
        //-------------------------------------------------------------------------------------------------------------

        // Overloads << operator to print bit-matrix.
        friend std::ostream& operator<<(std::ostream& os, const Matrix_ mat)
        {
            for (size_t j = 0; j < mat.rows(); ++j)
                os << mat.row(j);
            os << "\n";

            return os;
        }



        //-------------------------------------------------------------------------------------------------------------
        //                                      OPERATIONS
        //-------------------------------------------------------------------------------------------------------------

        // Return dot-product with another matrix from the right.
        // Enhances runtime by modifying matrix in place instead of returning an instanced matrix.
        constexpr Matrix_& dot(const Matrix_& rhs_mat)
        {
            if (cols() != rhs_mat.rows()) throw std::invalid_argument("Matrix dimensions must match (m x n)*(n x p).");

            size_t m = rows();
            size_t p = rhs_mat.cols();
            Matrix_ lhs_mat(*this);
            resize(m, p);

            for (size_t i = 0; i < p; ++i)
            {
                auto rhs_col = rhs_mat.col(i);
                for (size_t j = 0; j < m; ++j)
                    vector_set[j][i] = rhs_col.dot(lhs_mat.row(j));
            }
            return *this;
        }

        // Return dot-product with a bit-vector from the right.
        constexpr VectorType dot_rhs(const VectorType& rhs_vec) const
        {
            if (cols() != rhs_vec.size()) throw std::invalid_argument("Vector size must match number of columns in matrix.");

            size_t m = rows();
            VectorType result_vec(m);

            for (size_t j = 0; j < m; ++j)
                result_vec[j] = row(j).dot(rhs_vec);
            return result_vec;
        }

        // Assign * operator to matrix-vector multiplication.
        constexpr VectorType operator*(const VectorType& rhs_vec) const { return dot_rhs(rhs_vec); }

        // Performs dot-product with a bit-vector from the right to check if resulting vector is "all-zero". 
        // Breaks prematurely at first "one" in result vector.
        constexpr bool isOrthogonalTo(const VectorType& rhs_vec) const
        {
            if (cols() != rhs_vec.size()) throw std::invalid_argument("Vector size must match number of columns in matrix.");

            for (size_t j = 0; j < rows(); ++j)
                if (row(j).dot(rhs_vec)) return false;
            return true;          
        }

        // Return dot-product with a bit-vector from the left.
        constexpr VectorType dot_lhs(const VectorType& lhs_vec) const
        {
            if (rows() != lhs_vec.size()) throw std::invalid_argument("Vector size must match number of rows in matrix.");

            size_t n = cols();
            VectorType result_vec(n);

            for (size_t i = 0; i < n; ++i)
                result_vec[i] = lhs_vec.dot(col(i));
            return result_vec;
        }

        // Assign += operator to in-place matrix addition.
        // A += B
        constexpr Matrix_& operator+=(const Matrix_& rhs_mat)
        {
            if (rows() != rhs_mat.rows() || cols() != rhs_mat.cols()) throw std::invalid_argument("Matrices must be of same size.");

            for (size_t j = 0; j < rows(); ++j)
                row(j) ^= rhs_mat.row(j);
            return *this;
        }


    };

    using Matrix = Matrix_<>;

    // Assign * operator to matrix-matrix multiplication.
    // X = A * B
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Matrix_<Block, Allocator> operator*(const Matrix_<Block, Allocator>& A, const Matrix_<Block, Allocator>& B)
    {
        Matrix_<Block, Allocator> X(A);
        X.dot(B);
        return X;
    } 

    // NOTE: Assignment of * operator for matrix-vector multiplication 
    // can benefit from inlining and is therefore defined inside class.

    // Assign * operator to vector-matrix multiplication.
    // x = a * B
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator*(const Vector_<Block, Allocator> a, const Matrix_<Block, Allocator>& B)
    {
        return B.dot_lhs(a);
    } 

    // Assign + operator to matrix addition.
    // X = A + B
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Matrix_<Block, Allocator> operator+(const Matrix_<Block, Allocator>& A, const Matrix_<Block, Allocator>& B)
    {
        Matrix_<Block, Allocator> X(A);
        X += B;
        return X;
    }

}

#endif // GF2_MATRIX_H
