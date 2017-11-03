# Pineapple

Pineapple was a standalone Jupyter notebook client originally written by [Nathan Whitehead](http://github.com/nwhitehead), but as of mid-2017 the original project appears to be abandoned. This is a fork that has some modifications to play nice with linux in particular.

### Key changes

- System `python3` is used instead of a bundled installation (to simplify maintenance for me). Effectively this is now a thin GUI client for Jupyter notebooks. I still find this preferable to electron, to the extent that my native look-and-feel is better preserved. Note that `python3` is only used for running the network server and Jupyter notebook; `python2` and other kernels can be used (as is already the case with Jupyter) regardless.

### Known issues

- Jupyter 5 broke many of Nathan's original scripts. In particular:
  - in-notebook tabs no longer work
  - The original callback to get the kernel list on page load broke. The kernel list is now populated the first time you re-enter command mode.
- The `gtk3` version spawns a WebKitWebProcess that will reliably shoot up to 100% CPU utilisation after pressing a modifier key. This seems to be a problem with `webkit2gtk`, since the GTK2 version (for which `wxGTK` was built with `webkitgtk2`) worked just fine. Unfortunately, since the last time I updated this, `webkitgtk2` has been removed from the official Arch repositories (and in general most distributions now build wxGTK with `webkit2gtk`) because of security concerns. This is a known bug with `wxGTK` that I have reported upstream and [as of now has no resolution](https://trac.wxwidgets.org/ticket/17932).

## Building Prerequisites

General requirements:
* C++11 compiler (e.g. g++-4.9 or later)
* wxWidgets 3.x (source compile)
* lessc (for compiling .less files, get it with npm)

### Mac OS X

See the [original README](README.md.old) for build instructions on OS X (specifically on how to get wxWidgets). I don't have a Mac to test this on, and pull requests welcome.

### Ubuntu

Again, see the old README for instructions. I don't use Ubuntu and have done no testing; you might need to modify `CMakeLists.txt`

### Fedora

Depending on whether you want to build this for gtk2 or gtk3, you will need either `wxGTK-devel` or `wxGTK3-devel`, and correspondingly either `webkitgtk` or `webkitgtk3`.

Once those are installed, this builds and installs with no modifications as

```
mkdir build
cd build && cmake .. -DwxWidgets_CONFIG_OPTIONS="--toolkit=gtk3" -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
```

### Arch Linux

`pineapple` is [available on the AUR](https://aur.archlinux.org/packages/pineapple/).

## Distribution

Redistributable packages are built using CPack. With `-DCMAKE_INSTALL_PREFIX` not supplied, do the following:

```
make install
make package
```

The final redistributable files will be placed at the top level of the
build directory. Final packages will be compressed tar files for
Linux, DMG images for Mac.

## Contact

Pineapple was a project of Nathan Whitehead, from which this was forked.
