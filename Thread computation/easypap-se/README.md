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
