//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include "model.h"

int main() {
    double X[] = {1,2, 3,4}; // 2 samples, d=2
    double y[] = {0, 1};
    double w[] = {0.1, 0.2};
    double grad[2];

    compute_gradient(X, y, w, grad, 2, 2);

    printf("grad: %f %f\n", grad[0], grad[1]);
    return 0;
}