#ifndef _QPSOLVER
#define _QPSOLVER

#include "Eigen/Eigen"
#include "Eigen/SparseCore"
#include "strPrint.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class QPSolverWrapper
{
  public:
    typedef struct
    {
        std::vector<Eigen::Triplet<float>> objectiveMatrixTripletList;
        std::vector<float> objectiveMatrixDiag;
        Eigen::VectorXd objectiveVector;
        Eigen::VectorXd solution;
        Eigen::VectorXd oriSolution;
    } solverDataType;

    solverDataType solverData;

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> CGSolver;
    typedef struct
    {
        bool useUnconstrainedCG = true;
        bool MKLorNot = false;
        float lowerbound;
        float upperbound;
        int maxIters = 500;
        float tolerence = 0.001;
        bool solutionForward = false;
        bool verbose = false;
    } solverSettingsType;
    solverSettingsType solverSettings;

    QPSolverWrapper(bool useUnconstrainedCG, bool MKLorNot, float lowerbound, float upperbound, int elementNum,
                    bool verbose)
    {
        solverSettings.useUnconstrainedCG = useUnconstrainedCG;
        solverSettings.MKLorNot = MKLorNot;
        solverSettings.lowerbound = lowerbound;
        solverSettings.upperbound = upperbound;
        solverSettings.verbose = verbose;
        solverData.solution.resize(elementNum);
        solverData.oriSolution.resize(elementNum);
        solverData.objectiveVector.resize(elementNum);
    }
    ~QPSolverWrapper()
    {
    }

    static void QPSolve(QPSolverWrapper *&curSolver);
};

#endif