# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/lincongli/Desktop/CSE124/starter_code

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/starter_code.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/starter_code.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/starter_code.dir/flags.make

CMakeFiles/starter_code.dir/echo_client.c.o: CMakeFiles/starter_code.dir/flags.make
CMakeFiles/starter_code.dir/echo_client.c.o: ../echo_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/starter_code.dir/echo_client.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/starter_code.dir/echo_client.c.o   -c /Users/lincongli/Desktop/CSE124/starter_code/echo_client.c

CMakeFiles/starter_code.dir/echo_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/starter_code.dir/echo_client.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/lincongli/Desktop/CSE124/starter_code/echo_client.c > CMakeFiles/starter_code.dir/echo_client.c.i

CMakeFiles/starter_code.dir/echo_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/starter_code.dir/echo_client.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/lincongli/Desktop/CSE124/starter_code/echo_client.c -o CMakeFiles/starter_code.dir/echo_client.c.s

CMakeFiles/starter_code.dir/echo_client.c.o.requires:

.PHONY : CMakeFiles/starter_code.dir/echo_client.c.o.requires

CMakeFiles/starter_code.dir/echo_client.c.o.provides: CMakeFiles/starter_code.dir/echo_client.c.o.requires
	$(MAKE) -f CMakeFiles/starter_code.dir/build.make CMakeFiles/starter_code.dir/echo_client.c.o.provides.build
.PHONY : CMakeFiles/starter_code.dir/echo_client.c.o.provides

CMakeFiles/starter_code.dir/echo_client.c.o.provides.build: CMakeFiles/starter_code.dir/echo_client.c.o


CMakeFiles/starter_code.dir/echo_server.c.o: CMakeFiles/starter_code.dir/flags.make
CMakeFiles/starter_code.dir/echo_server.c.o: ../echo_server.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/starter_code.dir/echo_server.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/starter_code.dir/echo_server.c.o   -c /Users/lincongli/Desktop/CSE124/starter_code/echo_server.c

CMakeFiles/starter_code.dir/echo_server.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/starter_code.dir/echo_server.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/lincongli/Desktop/CSE124/starter_code/echo_server.c > CMakeFiles/starter_code.dir/echo_server.c.i

CMakeFiles/starter_code.dir/echo_server.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/starter_code.dir/echo_server.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/lincongli/Desktop/CSE124/starter_code/echo_server.c -o CMakeFiles/starter_code.dir/echo_server.c.s

CMakeFiles/starter_code.dir/echo_server.c.o.requires:

.PHONY : CMakeFiles/starter_code.dir/echo_server.c.o.requires

CMakeFiles/starter_code.dir/echo_server.c.o.provides: CMakeFiles/starter_code.dir/echo_server.c.o.requires
	$(MAKE) -f CMakeFiles/starter_code.dir/build.make CMakeFiles/starter_code.dir/echo_server.c.o.provides.build
.PHONY : CMakeFiles/starter_code.dir/echo_server.c.o.provides

CMakeFiles/starter_code.dir/echo_server.c.o.provides.build: CMakeFiles/starter_code.dir/echo_server.c.o


# Object files for target starter_code
starter_code_OBJECTS = \
"CMakeFiles/starter_code.dir/echo_client.c.o" \
"CMakeFiles/starter_code.dir/echo_server.c.o"

# External object files for target starter_code
starter_code_EXTERNAL_OBJECTS =

starter_code: CMakeFiles/starter_code.dir/echo_client.c.o
starter_code: CMakeFiles/starter_code.dir/echo_server.c.o
starter_code: CMakeFiles/starter_code.dir/build.make
starter_code: CMakeFiles/starter_code.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable starter_code"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/starter_code.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/starter_code.dir/build: starter_code

.PHONY : CMakeFiles/starter_code.dir/build

CMakeFiles/starter_code.dir/requires: CMakeFiles/starter_code.dir/echo_client.c.o.requires
CMakeFiles/starter_code.dir/requires: CMakeFiles/starter_code.dir/echo_server.c.o.requires

.PHONY : CMakeFiles/starter_code.dir/requires

CMakeFiles/starter_code.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/starter_code.dir/cmake_clean.cmake
.PHONY : CMakeFiles/starter_code.dir/clean

CMakeFiles/starter_code.dir/depend:
	cd /Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/lincongli/Desktop/CSE124/starter_code /Users/lincongli/Desktop/CSE124/starter_code /Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug /Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug /Users/lincongli/Desktop/CSE124/starter_code/cmake-build-debug/CMakeFiles/starter_code.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/starter_code.dir/depend

