# Configuring, Building and Installing EasyPAP

Depending on your needs, there are several ways to configure, build and install EasyPAP, or a subset of its components.

The EasyPAP environments sits on top of two components that can be used separately:
* **EZV**: an OpenGL-based interactive visualization library for 2D images and 3D meshes
* **EZM**: a monitoring/tracing library + the `easyview` trace visualization utility


## Building the whole EasyPAP environment

This section describes how to configure and compile EasyPAP to write and experiment with kernels written in Pthreads, OpenMP, CUDA, OpenCL, MPI, MIPP or Raja, or a combination of some of them.

You may start by customizing the root `CMakeLists.txt` file:
```makefile
# options
option(ENABLE_SCOTCH "Use Scotch library" ON)
option(ENABLE_TRACE "Generate execution traces" ON)
```

Configuring and building EasyPAP then follows the steps of a typical CMake workflow:

```shell
rm -rf build
# Configure
cmake -S . -B build
# Build
cmake --build build --parallel
```

To quickly give it a try, you may run the all-in-one Easypap cmake build script:

```shell
./script/cmake-easypap.sh
```

## Building and installing separate EasyPAP components

In case you only need to use some components of EasyPAP (e.g. EZV) in your application, you can use `cmake` to configure, build and install it. Here is an example that installs both EZV and EZM into the `${INSTALL_DIR}` directory:
```shell
rm -rf build
# Configure
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCONFIGURE_EASYPAP=OFF
# Build
cmake --build build --parallel
# install
cmake --install build --component ezv 
cmake --install build --component ezm
```

These instructions are included in the following script that you may customize before use:
```shell
./script/cmake-install-ezv-ezm.sh
```

Examples of how to extend your application's `CMakeLists.txt` file to use these components can be found in `lib/ezv/apps/` and `lib/ezm/apps/`.
