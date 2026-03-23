//
// Created by User on 2026/3/22.
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 100      // number of samples
#define DIM 3      // feature dimension
#define EPOCHS 1000
#define LR 0.1     // Learning rate


double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dot(const double *x, const double *y) {
    double result = 0.0;
    for (int i = 0; i < DIM; i++) {
        result = result + x[i] * y[i];
    }
    return result;
}

double compute_loss(double X[N][DIM], const double y[N], double w[DIM]) {
    double loss = 0.0;
    for (int i = 0; i < N; i++) {
        double z = dot(X[i], w);
        double p = sigmoid(z);

        // prevent 0
        if (p < 1e-10) p = 1e-10;
        if (p > 1 - 1e-10) p = 1 - 1e-10;

        loss += -y[i] * log(p) - (1 - y[i]) * log(1 - p);
    }
    return loss / N;
}

void compute_gradient(double X[N][DIM], const double y[N], double w[DIM], double grad[DIM]) {

    // init grad
    for (int j = 0; j < DIM; j++) {
        grad[j] = 0.0;
    }

    for (int i = 0; i < N; i++) {
        double z = dot(X[i], w);
        double p = sigmoid(z);

        for (int j = 0; j < DIM; j++) {
            grad[j] += (p - y[i]) * X[i][j];
        }
    }

    // average
    for (int j = 0; j < DIM; j++) {
        grad[j] /= N;
    }
}

double *train(double X[N][DIM], double y[N]) {

    double *w = malloc(sizeof(double) * DIM);  // initialize weights
    for (int i = 0; i < DIM; i++) {
        w[i] = 0.0;
    }
    double grad[DIM];

    for (int epoch = 0; epoch < EPOCHS; epoch++) {

        compute_gradient(X, y, w, grad);

        // update
        for (int j = 0; j < DIM; j++) {
            w[j] -= LR * grad[j];
        }

        if (epoch % 100 == 0) {
            double loss = compute_loss(X, y, w);
            printf("Epoch %d, Loss = %f\n", epoch, loss);
        }
    }

    printf("\nFinal weights:\n");
    for (int j = 0; j < DIM; j++) {
        printf("w[%d] = %f\n", j, w[j]);
    }

    return w;
}

double accuracy(double X[N][DIM], const double y[N], double w[DIM]) {

    int correct = 0;
    for (int i = 0; i < N; i++) {
        double z = dot(X[i], w);
        double p = sigmoid(z);


        if ((p > 0.5 && y[i]==1) || (p <=0.5 && y[i]==0)) {
            correct++;
        }
    }
    return (double)correct / N ;
}

int main() {

    double X[N][DIM];
    double y[N];

    for (int i = 0; i < N; i++) {
        double x1 = (double)rand() / RAND_MAX;
        double x2 = (double)rand() / RAND_MAX;

        X[i][0] = 1.0;   // bias term
        X[i][1] = x1;
        X[i][2] = x2;

        // linear boundary
        if (x1 + x2 > 1.0)
            y[i] = 1.0;
        else
            y[i] = 0.0;
    }

    double *w = train(X, y);

    printf("Accuracy: %f\n", accuracy(X, y, w));

    free(w);

    return 0;
}