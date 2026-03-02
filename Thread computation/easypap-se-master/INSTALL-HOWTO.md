# Configuring, Building and Installing EasyPAP

Depending on your needs, there are several ways to configure, build and install EasyPAP, or a subset of its components.

The EasyPAP environments sits on top of two components that can be used separately:
* **EZV**: an OpenGL-based interactive visualization library for 2D images and 3D meshes
* **EZM**: a monitoring/tracing library + the `easyview` trace visualization utility


## Building the whole EasyPAP environment

This section describes how to configure and compile EasyPAP to write and experiment with kernels written in Pthreads, OpenMP, CUDA, OpenCL, MPI, MIPP or Raja, or a combination of some of them.

You may start by customizing the root `CMakeLists.txt` file:
```cmake
# options
option(ENABLE_SCOTCH "Use Scotch library"       ON)
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


## Building and installing EZV and EZM only

In case you only need to use some components of EasyPAP (e.g. EZV) in your application, you can use `cmake` to configure, build and install it. Here is an example that installs both EZV and EZM into the `${INSTALL_DIR}` directory:
```shell
rm -rf build-libs
# Configure
cmake -S . -B build-libs -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" -DCONFIGURE_EASYPAP=OFF
# Build
cmake --build build-libs --parallel
# install
cmake --install build-libs --component ezv 
cmake --install build-libs --component ezm

rm -rf build-libs
```

These instructions are included in the following script that you may customize before use:
```shell
./script/cmake-libs.sh
```


## Using EZV and EZM in your own project

Once the EZV and EZM have been compiled and installed, you can use them the same way as any other library.

As an illustration here is how to extend your `CMakeLists.txt` file to link your application againts EZV:

```cmake
# assuming -DEasypap_ROOT="${INSTALL_DIR}/lib/cmake/Easypap"
find_package(Easypap COMPONENTS ezv REQUIRED)
target_link_libraries(myTarget Easypap::ezv)
```

Examples of how to extend your application's `CMakeLists.txt` file to use these components can be found in `lib/ezv/apps/` and `lib/ezm/apps/`.


## Building EasyPAP using pre-installed EZV and EZM libraries

Like any external application, Easypap can be configured and built using pre-installed EZV and EZM libraries:

```shell
rm -rf build
cmake -S . -B build -DEXTERNAL_EASYTOOLS=ON -DEasypap_ROOT="${INSTALL_DIR}/lib/cmake/Easypap"

# Build
cmake --build build --parallel
```

These instructions are included in the following script that you may customize before use:
```shell
./script/cmake-from-external-easytools.sh
```
