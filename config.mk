#### PROJECT SETTINGS ####
# The name of the executable to be created
BIN_NAME := test
# Compiler used
CXX ?= g++
# Extension of source files used in the project
SRC_EXT = cpp
# Path to the source directory, relative to the makefile
SRC_PATH = source
# Space-separated pkg-config libraries used by this project
LIBS = opencv4
# General compiler flags
COMPILE_FLAGS = -std=c++17 -Wall -Wextra
# Additional release-specific flags
RCOMPILE_FLAGS = -D NDEBUG
# Additional debug-specific flags
DCOMPILE_FLAGS = -g -D DEBUG -DDEBUGG
# Add additional include paths
INCLUDES = -I $(SRC_PATH)
# General linker settings
LINK_FLAGS =
# Additional release-specific linker settings
RLINK_FLAGS =
# Additional debug-specific linker settings
DLINK_FLAGS =
# Destination directory, like a jail or mounted system
DESTDIR = /
# Install path (bin/ is appended automatically)
INSTALL_PREFIX = usr/local
#### END PROJECT SETTINGS ####
