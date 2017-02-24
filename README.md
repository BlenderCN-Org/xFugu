xFugu
==

Is a procedural modelling system for 3D graphics. It supports the generation, manipulation and animation of 3D form using scripts written in the Lua programming language. Fugu contains a code editor with all the usual bells and whistles, and a 3D view in which to visualise your design. Fugu's purpose is to support rapid exploration of code-based procedural graphics. It is currently in perpetual alpha, but works well, so check it out.


Directory Structure
==

- src/ contains the source for fugu and the cmake build script
- build-/ will contain the build files after running one of the build batch files
- include/ contains headers for dependencies
- scripts/ contains the fg lua scripts (scripts/ex contains the latest working examples)
- core/ contains the fg lua core functions and documentation
- lib/ contains pre-compiled libraries of the dependencies
- tools/ contains misc tools
- doc/ contains both the doxygen file for c++ comments and the template for the online reference
-site/ contains the source of the website

Building Instructions
==

Fugu is built using Cmake 2.8. As such, it should be possible to output project files for Xcode, Visual Studio, etc. Some build scripts are provided for some of these, to generate other projects you will need to write your own scripts. Some example build sequences (from within the fg/ directory):

OSX/Make
--
``` bash
./osx-make-build.sh
cd build-osx-make
make install
```

Windows/MinGW/Eclipse
--

``` bash
eclipse-msw-build.bat
cd build-eclipse-msw
make install
```

Linux
--

``` bash
sudo bash linux-make-build.sh
cd build-linux-make
make install
```

Fugu uses Qt extensively, so you will need to have that installed (version 4.7.4 or later). For the osx/make and windows/mingw targets, most of the dependencies have been prebuilt and reside in lib/. If you target other systems, e.g., Visual Studio, then you may need to build the dependencies for that target.

For Linux (on Ubuntu), you'll need

``` bash
sudo apt-get install build-essential cmake
sudo apt-get install qt4-dev-tools
sudo apt-get install liblua5.1-0-dev freeglut3-dev libglew-dev libluabind-dev libboost-dev libboost-system-dev libboost-filesystem-dev libqscintilla2-dev
```

On Fedora Linux systems, install cmake, gcc, gcc-c++, boost, boost-devel, qt, qt-devel, lua, luabind, luabind-devel,freeglut, freeglut-devel, glew, qscintilla, qscintilla-devel


> This readme is for people who want to compile and hack the source of fugu. Please see LICENSE.txt for the license governing use of the fg source, and CREDITS.txt for a list of programmers on the project and list of libraries used.
