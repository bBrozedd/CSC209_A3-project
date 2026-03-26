# Distributed Logistic Regression (C + TCP)
## Updated Project Pipeline (Synchronous BSP Design)

---

## Overall Architecture

We implement a **parameter-server architecture** using:

- **Server (single-threaded)**
- **Multiple workers (clients)**
- **TCP sockets for communication**

### Key Design Choices

- **Training mode**: Synchronous (Bulk Synchronous Parallel, BSP)
- **Concurrency model**: Single-threaded server using `select()`
- **Data distribution**: Data parallelism (each worker owns a data partition)

---

## Data Distribution

- The dataset is split into multiple partitions:
  - `data_1.csv`, `data_2.csv`, ...
- Each worker loads its own local dataset at startup
- The **server does NOT store or access training data**

### Rationale

- Matches real-world distributed ML systems
- Avoids data transfer overhead
- Simplifies server design

---

## Training Workflow (Per Iteration)

Each training round proceeds as follows:

1. **Server broadcasts model parameters `w` to all workers**
2. **Workers compute locally**:
   - Gradient on their local dataset
   - Local loss
3. **Workers send results back to server**:
   - Gradient
   - Loss
   - Number of samples
4. **Server aggregates**:
   - Sum gradients
   - Compute weighted average loss
5. **Server updates model `w`**
6. **Server checks stopping condition**

---

## Stopping Condition

Training stops when:

- Global loss < threshold, OR
- Maximum number of iterations reached

---

## Finalization

After training ends:

- Server broadcasts final model parameters
- A special message type `TRAIN_DONE` is sent
- Workers terminate after receiving this message

---

## Communication Protocol

### Message Types

- `GET_PARAM` (optional, may not be used in sync design)
- `PARAM` (server → worker)
- `SEND_GRAD` (worker → server)
- `TRAIN_DONE` (server → worker)

### Message Structure (Example)

```c
typedef struct {
    int type;
    double grad[DIM];
    double loss;
    int n_samples;
} Message;
```

---

## Server Logic (Single-threaded, select-based)

### Initialization

- Create socket
- Bind and listen
- Accept all worker connections
- Store worker file descriptors

### Training Loop

```text
while (!stop_condition) {

    // 1. Broadcast parameters
    send w to all workers

    // 2. Collect gradients using select()
    reset fd_set
    while (not all workers responded) {
        select()
        recv gradient from ready workers
    }

    // 3. Aggregate
    sum gradients
    compute weighted loss

    // 4. Update model
    w = w - lr * grad

    // 5. Check stopping condition
}
```

### After Training

```text
broadcast TRAIN_DONE + final w
close connections
```

---

## Worker Logic

### Initialization

- Connect to server
- Load local dataset from file

### Training Loop

```text
while (true) {

    recv message from server

    if message == TRAIN_DONE:
        break

    // receive parameters w

    compute gradient on local data
    compute local loss

    send (grad, loss, n_samples) to server
}
```

---

## Loss Aggregation

Each worker computes:

- `loss_i` on local data
- `n_i` = number of samples

Server computes global loss:

```
global_loss = sum(loss_i * n_i) / sum(n_i)
```

---

## Phase Breakdown (Updated)

### Phase 1: Single Machine Logistic Regression
- Implement sigmoid, loss, gradient, gradient descent

### Phase 2: Modularization
- `model.c / model.h`

### Phase 3: Basic TCP Communication
- Client-server connection working

### Phase 4: Communication Protocol
- Define message struct and types

### Phase 5: Synchronous Server (No pthread)
- Single-threaded server
- Use `select()` to handle multiple workers

### Phase 6: Worker Training Loop
- Implement training loop and communication

### Phase 7: ML Integration
- Plug in gradient and loss computation

### Phase 8: Data Partitioning
- Split dataset across workers

### Phase 9: (Optional Extension)
- Async training OR partial synchronization

### Phase 10: Validation
- Verify loss decreases
- Compare with single-machine version

---

## Key Challenges

- Partial send/recv handling
- Struct padding issues
- Synchronization logic (iteration correctness)
- Handling slow workers

---

## Resume Description (Draft)

Built a distributed machine learning system in C using a TCP-based parameter-server architecture. Implemented synchronous data-parallel training for logistic regression with a select()-based server and partitioned datasets across workers.

