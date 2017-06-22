# Pineapple

This project is a standalone Jupyter environment for doing data science
using Python. It aims to include many useful working libraries
and packages, while remaining super easy to install and use.

Pineapple was originally written by [Nathan Whitehead](http://github.com/nwhitehead), but as of mid-2017 the original project appears to be abandoned. 

### Key changes

- System python is used instead of a bundled installation (to simplify maintenance for me). Effectively this is now a thin GUI client for Jupyter notebooks. I still find this preferable to electron, to the extent that my native look-and-feel is better preserved.

## Building Prerequisites

General requirements:
* C++11 compiler (e.g. g++-4.9 or later)
* wxWidgets 3.x (source compile)
* lessc (for compiling .less files, get it with npm)

### Mac OS X

From the original README:

> For wxWidgets, I downloaded the source then used:
> 
> ```
> mkdir build-release
> cd build-release
> ../configure --enable-shared --enable-monolithic --with-osx_cocoa CXX='clang++ -std=c++11 -stdlib=libc++' CC=clang --with-macosx-version-min=10.8 --disable-debug --without-liblzma
> make -j4
> sudo make install
> ```

I don't have a Mac to test this on. Pull requests welcome.

### Ubuntu 14.04

See the old README for instructions. I don't use Ubuntu and have done no testing; you might need to modify `CMakeLists.txt`.

## Building for local testing

If prerequisites are met, you should be able to do:

```
mkdir build
cd build
cmake ..
make
```

This builds the `Pineapple` executable on Linux (I have no idea what it does on Mac OS tbh)

## Distribution

Redistributable packages are built using CPack.

```
make install
make package
```

The final redistributable files will be placed at the top level of the
build directory. Final packages will be compressed tar files for
Linux, DMG images for Mac.

## Contact

Pineapple was a project of Nathan Whitehead, from which this was forked.