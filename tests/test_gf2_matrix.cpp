#include <cassert>
#include <iostream>

#include <GF2/Matrix.h>
#include <GF2/Vector.h>

int main() {
    using GF2::Matrix;
    using GF2::Vector;

    // ---- Identity ----
    Matrix I = Matrix::Identity(4);
    assert(I.rows() == 4 && I.cols() == 4);
    assert(I.is_identity());

    // ---- transpose ----
    Matrix A(2,3);
    A.zeros();
    A(0,1) = 1;
    A(1,2) = 1;
    Matrix AT = A.transpose();
    assert(AT.rows() == 3 && AT.cols() == 2);
    assert(AT(1,0) == 1);
    assert(AT(2,1) == 1);

    // ---- inverse of identity ----
    Matrix InvI = I.inverse();
    assert(InvI.is_identity());

    // ---- inverse small known nonsingular matrix ----
    // A = [1 1
    //      1 0]  over GF(2)  has inverse itself
    Matrix M(2,2);
    M.zeros();
    M(0,0)=1; M(0,1)=1;
    M(1,0)=1; M(1,1)=0;

    Matrix Minv = M.inverse();
    // Check M * Minv = I
    Matrix Prod = M * Minv;
    assert(Prod.is_identity());

    // ---- matrix-vector multiply ----
    Vector x(2); x.zeros(); x[0]=1; x[1]=0;
    Vector y = M * x; // should equal first column of M: [1;1]
    assert(y.size() == 2);
    assert(y[0] == 1);
    assert(y[1] == 1);

    std::cout << "[OK] test_gf2_matrix passed\n";
    return 0;
}
