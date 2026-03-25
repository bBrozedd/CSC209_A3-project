//
// Created by User on 2026/3/25.
//

#include <math.h>

#include "model.h"

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dot(double *x, double *y, int dim) {
    double result = 0.0;
    for (int i = 0; i < dim; i++) {
        result = result + x[i] * y[i];
    }
    return result;
}

double compute_loss(double *X, double *y, double *weight, int n, int dim) {
    double loss = 0.0;
    for (int i = 0; i < n; i++) {
        double *xi = X + i * dim;
        double z = dot(xi, weight, dim);
        double pred = sigmoid(z);

        // prevent 0
        if (pred < 1e-10) pred = 1e-10;
        if (pred > 1 - 1e-10) pred = 1 - 1e-10;

        loss += -y[i] * log(pred) - (1 - y[i]) * log(1 - pred);
    }
    return loss / n;
}

void compute_gradient(double *X, double *y, double *weight, double *grad, int n, int dim) {
    // init grad
    for (int j = 0; j < dim; j++) {
        grad[j] = 0.0;
    }

    for (int i = 0; i < n; i++) {
        double *xi = X + i * dim;
        double z = dot(xi, weight, dim);
        double pred = sigmoid(z);

        for (int j = 0; j < dim; j++) {
            grad[j] += (pred - y[i]) * xi[j];
        }
    }

    // average
    for (int j = 0; j < dim; j++) {
        grad[j] /= n;
    }
}