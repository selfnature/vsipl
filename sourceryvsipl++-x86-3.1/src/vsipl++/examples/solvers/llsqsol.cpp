/* Copyright (c) 2010 by CodeSourcery.  All rights reserved. */

/// VSIPL++ Library: Least Squares solver example.
///
/// This example illustrates how to use the VSIPL++ Least Squares
/// Solver facility to solve the least-squares system
/// 
///   min_x norm-2 (A x - b)
/// 
/// for x.  A is MxN; b and x are NxP.  One can transform that
/// system to this form
///
///   A'Ax = A'b
///
/// by a sequence of operations that are beyond the scope of this
/// commentary.
///
/// Now use the QR method (see that example) to
/// decompose A into matrices Q and R.
///
///   A = QR
///   A' = R'Q' by normal matrix-product-transpose
///   Q'Q = I by definition of Q.
/// 
///   A'A = R'Q'QR = R'IR by applying the above
///   R'IR = R'R by definition of identity matrix
/// 
/// Thus A'Ax = A'b is R'Rx = R'Q'b.  Solve for x.
/// 
/// The expected result is
///   x = 
///     0:  -2
///     1:  0
///     2:  1

#include <iostream>
#include <vsip/initfin.hpp>
#include <vsip/support.hpp>
#include <vsip/signal.hpp>
#include <vsip/math.hpp>
#include <vsip/solvers.hpp>
#include <vsip_csl/output.hpp>


using namespace vsip;


int
main(int argc, char **argv)
{
  vsipl init(argc, argv);

  length_type m =  3;   // Rows in A.
  length_type n =  3;	// Cols in A; rows in b.
  length_type p =  1;	// Cols in b.

  // Create some inputs, a 3x3 matrix and a 3x1 matrix.
  Matrix<float> A(m, n);
  Matrix<float> b(n, p);

  // Initialize the inputs.
  A(0, 0) =  1; A(0, 1) = -2; A(0, 2) =  3;
  A(1, 0) =  4; A(1, 1) =  0; A(1, 2) =  6;
  A(2, 0) =  2; A(2, 1) = -1; A(2, 2) =  3;

  b(0, 0) =  1; b(1, 0) = -2; b(2, 0) = -1;
  
  // Create a 3x1 matrix for output.
  Matrix<float> x(n, p);

  // Solve min_x norm-2 (A x - b) for x.
  llsqsol(A, b, x);

  // Display the results.
  std::cout
    << std::endl
    << "Least Squares Solver example" << std::endl
    << "----------------------------" << std::endl
    << "A = " << std::endl << A << std::endl
    << "b = " << std::endl << b << std::endl
    << "x = " << std::endl << x << std::endl;

  return 0;
}
