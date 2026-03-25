
# Distributed Logistic Regression (C + TCP) Project Pipeline

## Goal
Build a TCP-based parameter-server distributed system in C for training logistic regression.

---

## Phase 1: Single Machine Logistic Regression
- Implement sigmoid, loss, gradient, gradient descent
- Verify loss decreases

---

## Phase 2: Modularize Model
- model.c / model.h
- reusable functions: sigmoid, dot, gradient, loss

---

## Phase 3: Basic TCP Communication
- server: socket, bind, listen, accept
- client: socket, connect
- test hello/ok

---

## Phase 4: Communication Protocol
Define:
- Message struct
- Types: GET_PARAM, SEND_GRAD, PARAM

---

## Phase 5: Multi-threaded Server
- pthread for each client
- global parameter w with mutex

---

## Phase 6: Worker Logic
Loop:
1. GET_PARAM
2. receive w
3. compute gradient
4. SEND_GRAD
5. receive updated w

---

## Phase 7: Integrate ML
- worker computes gradient
- server updates w

---

## Phase 8: Distributed Data
- split dataset into multiple files
- each worker loads its partition

---

## Phase 9: Sync vs Async
- Async: update immediately
- Sync: aggregate then update

---

## Phase 10: Validation
- check loss decreasing
- compare with single machine

---

## Challenges
- partial send/recv
- race conditions
- convergence issues
- struct padding

---

## Final Structure
project/
- server.c
- worker.c
- model.c/h
- data/
- Makefile

---

## Resume Line
Built a distributed ML system in C using TCP parameter-server architecture.
