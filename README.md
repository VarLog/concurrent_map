# concurrent_map
=================

Simple implementation of the concurrent thread-safe map with exclusive access to the entries on C++11.

## Source structure

* `src/concurrent_map` contains all the implementation into `concurrent_map.h` file and the example into main.cpp;
* `src/test` contains Google C++ Testing Framework tests.

## Build

Use CMake:

```
> mkdir build
> cd build
> cmake -DENABLE_TESTS=ON ..
> make -j8
> concurrent_map
...
```

## Tests

For testing purposes this project uses Google C++ Testing Framework. To enable tests you should define `ENABLE_TESTS=ON` variable:

```
> cmake -DENABLE_TESTS=ON ..
> make -j8
> ./concurrent_map_test
...
```
