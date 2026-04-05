# CSC209 A3 Project

Distributed logistic regression in C using a parameter-server architecture over TCP sockets.

## Overview

This project trains a small logistic regression model across multiple worker processes. Each worker loads a local CSV partition, computes a gradient and local loss, and sends the result to a central server. The server aggregates worker updates synchronously, applies gradient descent, and stops when the loss threshold is reached or the iteration limit is exhausted.

The implementation matches a bulk synchronous parallel (BSP) design:

- `server`: accepts worker connections, broadcasts parameters, aggregates gradients with `select()`, updates the model
- `worker`: connects to the server, loads one data partition, computes local gradient/loss, sends updates back
- `model`: shared logistic regression helpers for sigmoid, loss, dot product, and gradient computation

## Architecture

- Training style: synchronous data-parallel training
- Communication: TCP sockets
- Server concurrency model: single-threaded `select()` loop
- Number of workers: fixed at `3` in [`src/server.c`](/home/brozed/CSC209_A3-project/src/server.c)
- Feature dimension: fixed at `3` in [`src/protocol.h`](/home/brozed/CSC209_A3-project/src/protocol.h)
  This corresponds to `bias, x1, x2`
- Worker sample capacity: fixed at `100` rows per worker in [`src/worker.c`](/home/brozed/CSC209_A3-project/src/worker.c)

## Repository Layout

- [`Makefile`](/home/brozed/CSC209_A3-project/Makefile): build rules for `server`, `worker`, and `test_model`
- [`run.sh`](/home/brozed/CSC209_A3-project/run.sh): demo script that generates CSV partitions and launches one server plus three workers
- [`src/model.c`](/home/brozed/CSC209_A3-project/src/model.c): logistic regression math
- [`src/model.h`](/home/brozed/CSC209_A3-project/src/model.h): model function declarations
- [`src/protocol.h`](/home/brozed/CSC209_A3-project/src/protocol.h): shared message and model structs
- [`src/server.c`](/home/brozed/CSC209_A3-project/src/server.c): parameter server implementation
- [`src/worker.c`](/home/brozed/CSC209_A3-project/src/worker.c): worker/client implementation
- [`src/test_model.c`](/home/brozed/CSC209_A3-project/src/test_model.c): small unit-style gradient test
- [`src/minimum_lr.c`](/home/brozed/CSC209_A3-project/src/minimum_lr.c): standalone single-process logistic regression prototype
- [`Design_pipeline.md`](/home/brozed/CSC209_A3-project/Design_pipeline.md): design notes for the distributed training pipeline

## Build

Requirements:

- `gcc`
- `make`
- standard POSIX socket headers/libraries
- `python3` only if you want to use `run.sh`

Build everything:

```bash
make
```

Build with a custom port:

```bash
make clean
make PORT=58800
```

Available targets:

- `server`
- `worker`
- `test_model`
- `clean`

## Run

### 1. Quick model test

```bash
make
./test_model
```

This prints the gradient for a tiny hard-coded example from [`src/test_model.c`](/home/brozed/CSC209_A3-project/src/test_model.c).

### 2. Distributed training demo

Start by ensuring the compiled port matches the runtime port.

Important: [`run.sh`](/home/brozed/CSC209_A3-project/run.sh) uses port `58800`, but the default `Makefile` port is `4242`. To use the script as written:

```bash
make clean
make PORT=58800
./run.sh
```

The script will:

1. generate three CSV partitions in `test_data_large/`
2. start the server
3. start three workers
4. wait for training to finish
5. print the tail of the server and worker logs

### 3. Manual run

Build with a chosen port first:

```bash
make clean
make PORT=4242
```

In one terminal:

```bash
./server
```

In three separate terminals:

```bash
./worker 127.0.0.1 path/to/data_1.csv
./worker 127.0.0.1 path/to/data_2.csv
./worker 127.0.0.1 path/to/data_3.csv
```

The server waits until all three workers connect before beginning training.

## Data Format

Each worker reads a CSV file where every row has:

```text
bias,x1,x2,label
```

Example:

```text
1.0,0.24,0.81,1.0
1.0,0.72,0.11,0.0
```

Notes:

- the current protocol assumes exactly `DIM == 3` features
- the worker reads up to `100` samples from its assigned CSV file
- the label is expected after the three feature values

## Protocol

Shared message types are defined in [`src/protocol.h`](/home/brozed/CSC209_A3-project/src/protocol.h):

- `PARAM`: server sends the current model weights to a worker
- `SEND_GRAD`: worker sends gradient, local loss, and sample count back to the server
- `TRAIN_DONE`: server sends the final model and loss, then workers exit

Each message is a fixed-size `Message` struct carrying:

- `type`
- `weight[DIM]`
- `gradient[DIM]`
- `loss`
- `n_samples`

## Training Behavior

Current constants are defined in source:

- learning rate: `LR = 1` in [`src/server.c`](/home/brozed/CSC209_A3-project/src/server.c)
- max iterations: `10000` in [`src/server.c`](/home/brozed/CSC209_A3-project/src/server.c)
- stopping threshold: `LOSS_THRESHOLD = 0.05` in [`src/server.c`](/home/brozed/CSC209_A3-project/src/server.c)

Per iteration:

1. server broadcasts weights
2. each worker computes local gradient and loss
3. server waits for all active workers with `select()`
4. server sums gradients and computes weighted average loss
5. server updates the model weights

## Known Limitations

- worker count is hard-coded to `3`
- feature dimension is hard-coded to `3`
- worker sample capacity is fixed at `100`
- messages are exchanged as raw C structs, so portability across different architectures is limited
- gradients from workers are summed directly; there is no explicit normalization by number of workers at the server
- no command-line configuration for learning rate, thresholds, or worker count

## Verification

Verified locally:

- `make`
- `./test_model`
