This directory provides a few sample programs using the EZV library.

In practice, adding EZV to the depencency list of your project consists in adding the following
lines to your `CMakeLists.txt` file:
```makefile
# We only need ezv from EasyPap
find_package(Easypap COMPONENTS ezv REQUIRED)

target_link_libraries(myTarget Easypap::ezv)
```

Except if EasyPAP was installed in a standard directory (e.g. `/opt/local`),
you'll have to set Easypap_ROOT to help cmake locating the corresponding configuration files.
For instance:
```shell
cmake -S . -B build -DEasypap_ROOT=${INSTALL_DIR}/lib/cmake/Easypap
```
