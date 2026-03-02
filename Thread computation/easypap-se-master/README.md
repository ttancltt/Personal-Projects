[EasyPAP](https://gforgeron.gitlab.io/easypap) aims at providing students with an easy-to-use programming environment
to learn parallel programming.

The idea is to parallelize sequential computations on 2D matrices (including images) or 3D unstructured meshes over multicore and GPU platforms. At each iteration, the current data can be visualized, allowing to visually check the correctness of the computation method. Multiple variants can easily been developed (e.g. sequential, OpenMP, MPI, OpenCL, CUDA, etc.) and compared. EasyPAP is written in C, uses the SDL and OpenGL libraries for fast 2D/3D graphics rendering, and is available on Linux and MacOS platforms. Most of the parameters can be specified as command line arguments, which facilitates automation of experiments through scripts:

* size of input data, image/mesh file to be loaded;
* kernel to use (e.g. blur, pixelize, mandelbrot, life, …);
* variant to use (e.g. seq, omp, omp_task, pthread, mpi, ocl, …);
* code optimization to use (e.g. default, AVX, MIPP, …)
* interactive mode / performance mode;
* monitoring mode;
* tracing mode;
* and much more!

Please read the [Getting Started Guide](https://gforgeron.gitlab.io/easypap/doc/Getting_Started.pdf) before you start!


############### Student's description and command line ##################"
Dưới đây là một README mẫu bằng tiếng Anh để mô tả cách sử dụng `spin.c` và `mandel.c` trong môi trường EasyPAP.
Bạn có thể copy chỉnh sửa theo đúng implementation của bạn.

---

# README – Spin and Mandel Kernels (EasyPAP)

## Overview

This project implements and studies parallel acceleration of two 2D image processing kernels:

* **Spin kernel** (`spin.c`)
* **Mandelbrot kernel** (`mandel.c`)

Both kernels are executed inside the EasyPAP framework and support multiple computation variants (sequential and multithreaded).

The goal is to compare different parallelization strategies and measure their performance.

---

# 1. Compilation

After cloning EasyPAP, compile the environment using:

```bash
./script/cmake-easypap.sh
make -C build -j
```

To recompile after modifying source files:

```bash
make -C build -j
```

---

# 2. Running the Spin Kernel

## Sequential Version

The sequential implementation is located in:

```
kernel/c/spin.c
```

The main function for the sequential version is:

```c
spin_compute_seq()
```

Run it with:

```bash
./run -k spin -v seq
```

To measure performance without display:

```bash
./run -k spin -v seq -n -i 500
```

Options:

* `-k spin` → selects the spin kernel
* `-v seq` → selects the sequential variant
* `-n` → disables display (faster benchmarking)
* `-i 500` → number of iterations

---

## Multithreaded Version – Horizontal Block Distribution

Implemented function:

```c
spin_compute_thread_block()
```

Each thread computes a horizontal band of the image.

Run with:

```bash
./run -k spin -v thread_block
```

To specify number of threads:

```bash
OMP_NUM_THREADS=2 ./run -k spin -v thread_block
```

To measure performance:

```bash
./run -k spin -v thread_block -n -i 500
```

---

## Work Visualization

To visualize which thread processes which region:

```bash
./run -k spin -v thread_block -m
```

The `-m` option activates monitoring.

---

# 3. Mandelbrot Kernel

The Mandelbrot implementation is located in:

```
kernel/c/mandel.c
```

The sequential version:

```c
mandel_compute_seq()
```

Run with:

```bash
./run -k mandel
```

---

## Multithreaded Version – Block Strategy

The first parallel implementation follows the same block strategy as spin:

```c
mandel_compute_thread_block()
```

Run:

```bash
./run -k mandel -v thread_block -m
```

### Observation

Unlike the spin kernel, Mandelbrot has **irregular workload distribution**.
Some image areas require significantly more iterations than others.

As a result:

* Horizontal block distribution leads to load imbalance.
* Some threads finish much earlier than others.

---

## Cyclic Distribution Strategy

Improved strategy:

Each thread computes:

```
line = me
line = me + nb_threads
line = me + 2 * nb_threads
...
```

Variant:

```c
mandel_compute_thread_cyclic()
```

Run:

```bash
./run -k mandel -v thread_cyclic -n -i 50
```

This approach improves load balancing.

---

## Dynamic Distribution Strategy

Advanced strategy using a shared index distributor:

Functions implemented:

```c
int distrib_get(void);
void distrib_reset(void);
```

Variant:

```c
mandel_compute_thread_dyn()
```

Run:

```bash
./run -k mandel -v thread_dyn -n -i 50
```

This version typically provides the best acceleration because:

* Threads dynamically request new lines
* Workload is balanced at runtime
* No thread remains idle

---

# 4. Performance Comparison

Acceleration is measured as:

```
Speedup = Sequential time / Parallel time
```

Expected results:

* **Spin** → Good speedup with block distribution (uniform workload)
* **Mandelbrot (block)** → Poor speedup (load imbalance)
* **Mandelbrot (cyclic)** → Better speedup
* **Mandelbrot (dynamic)** → Best speedup

---

# 5. Key Differences Between Spin and Mandel

| Kernel     | Workload Type | Best Strategy    |
| ---------- | ------------- | ---------------- |
| Spin       | Uniform       | Block            |
| Mandelbrot | Irregular     | Cyclic / Dynamic |

---

# Conclusion

* Spin benefits from simple horizontal partitioning.
* Mandelbrot requires load balancing strategies due to non-uniform computation cost.
* Dynamic scheduling achieves the best performance for irregular workloads.


