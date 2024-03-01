#include "QPSolverWrapper.h"

#include <cmath>

void QPSolverWrapper::QPSolve(QPSolverWrapper *&curSolver)
{

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> &CGSolver = curSolver->CGSolver;
    std::vector<Eigen::Triplet<float>> &objectiveMatrixTripletList = curSolver->solverData.objectiveMatrixTripletList;
    Eigen::VectorXd &objectiveVector = curSolver->solverData.objectiveVector;

    // min_x 0.5 * x'Px + q'x
    // s.t.  l <= Ax <= u

    // objective_matrix is P.
    // objective_vector is q.
    // constraint_matrix is A.
    // lower_bounds is l.
    // upper_bounds is u.

    Eigen::SparseMatrix<double> objective_matrix(objectiveVector.size(), objectiveVector.size());
    for (unsigned int i = 0; i < objectiveVector.size(); i++)
        objectiveMatrixTripletList.push_back(Eigen::Triplet<float>(i, i, curSolver->solverData.objectiveMatrixDiag[i]));
    objective_matrix.setFromTriplets(objectiveMatrixTripletList.begin(), objectiveMatrixTripletList.end());

    if (curSolver->solverSettings.useUnconstrainedCG)
    {
        /////////////////////////////////////////////////////////////////////////
        // Conjugate Gradient (does not support constraint yet.)
        if (curSolver->solverSettings.verbose)
            print_status("Unconstrained CG Solver Started.");
        CGSolver.setMaxIterations(curSolver->solverSettings.maxIters);
        CGSolver.setTolerance(curSolver->solverSettings.tolerence);
        CGSolver.compute(objective_matrix);
        if (curSolver->solverSettings.solutionForward)
            curSolver->solverData.oriSolution =
                CGSolver.solveWithGuess(-objectiveVector, curSolver->solverData.oriSolution);
        else
            curSolver->solverData.solution =
                CGSolver.solveWithGuess(-objectiveVector, curSolver->solverData.oriSolution);
        if (curSolver->solverSettings.verbose)
            print_status("Unconstrained CG Solver Done.");
    }
}
