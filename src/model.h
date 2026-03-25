//
// Created by User on 2026/3/25.
//

#ifndef CSC209_A3_PROJECT_MODEL_H
#define CSC209_A3_PROJECT_MODEL_H

double sigmoid(double x);
double dot(double *x, double *y, int dim);
double compute_loss(double *X, double *y, double *weight, int n, int dim);
void compute_gradient(double *X, double *y, double *weight, double *grad, int n, int dim);

#endif //CSC209_A3_PROJECT_MODEL_H