//
// Created by User on 2026/3/26.
//

#ifndef CSC209_A3_PROJECT_PROTOCOL_H
#define CSC209_A3_PROJECT_PROTOCOL_H

#define DIM 10 // Fixed dimension for feature

typedef enum {
    PARAM = 1,
    SEND_GRAD = 2,
    TRAIN_DONE = 3,
} MessageType;

typedef struct {
    int type;

    // Send weight from server to worker.
    double weight[DIM];

    // Send gradient and loss from worker to server.
    double gradient[DIM];
    double loss;
    int n_samples;

} Message;

#endif //CSC209_A3_PROJECT_PROTOCOL_H