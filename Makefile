# Command aliases only: CMakeLists.txt owns the complete build configuration.
CMAKE ?= cmake

.PHONY: all debug clean

# Release applications, without test executables.
all:
	$(CMAKE) -DCFD_ACTION=build -P "$(CURDIR)/CMakeLists.txt"

# Debug applications and the CTest suite.
debug:
	$(CMAKE) -DCFD_ACTION=debug -P "$(CURDIR)/CMakeLists.txt"

# Generated build directories and application copies.
clean:
	$(CMAKE) -DCFD_ACTION=clean -P "$(CURDIR)/CMakeLists.txt"
