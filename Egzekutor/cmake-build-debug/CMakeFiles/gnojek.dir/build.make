# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /tmp/tmp.OU9zFcOwvk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /tmp/tmp.OU9zFcOwvk/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/gnojek.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/gnojek.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/gnojek.dir/flags.make

CMakeFiles/gnojek.dir/gnojek.c.o: CMakeFiles/gnojek.dir/flags.make
CMakeFiles/gnojek.dir/gnojek.c.o: ../gnojek.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/tmp/tmp.OU9zFcOwvk/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/gnojek.dir/gnojek.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/gnojek.dir/gnojek.c.o -c /tmp/tmp.OU9zFcOwvk/gnojek.c

CMakeFiles/gnojek.dir/gnojek.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gnojek.dir/gnojek.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /tmp/tmp.OU9zFcOwvk/gnojek.c > CMakeFiles/gnojek.dir/gnojek.c.i

CMakeFiles/gnojek.dir/gnojek.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gnojek.dir/gnojek.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /tmp/tmp.OU9zFcOwvk/gnojek.c -o CMakeFiles/gnojek.dir/gnojek.c.s

# Object files for target gnojek
gnojek_OBJECTS = \
"CMakeFiles/gnojek.dir/gnojek.c.o"

# External object files for target gnojek
gnojek_EXTERNAL_OBJECTS =

gnojek: CMakeFiles/gnojek.dir/gnojek.c.o
gnojek: CMakeFiles/gnojek.dir/build.make
gnojek: liberr.a
gnojek: CMakeFiles/gnojek.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/tmp/tmp.OU9zFcOwvk/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable gnojek"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gnojek.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/gnojek.dir/build: gnojek

.PHONY : CMakeFiles/gnojek.dir/build

CMakeFiles/gnojek.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/gnojek.dir/cmake_clean.cmake
.PHONY : CMakeFiles/gnojek.dir/clean

CMakeFiles/gnojek.dir/depend:
	cd /tmp/tmp.OU9zFcOwvk/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.OU9zFcOwvk /tmp/tmp.OU9zFcOwvk /tmp/tmp.OU9zFcOwvk/cmake-build-debug /tmp/tmp.OU9zFcOwvk/cmake-build-debug /tmp/tmp.OU9zFcOwvk/cmake-build-debug/CMakeFiles/gnojek.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/gnojek.dir/depend
