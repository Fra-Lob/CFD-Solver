CMAKE ?= cmake

.PHONY: all debug clean
all:
	$(CMAKE) -DCFD_ACTION=build -P "$(CURDIR)/CMakeLists.txt"

debug:
	$(CMAKE) -DCFD_ACTION=debug -P "$(CURDIR)/CMakeLists.txt"

clean:
	$(CMAKE) -DCFD_ACTION=clean -P "$(CURDIR)/CMakeLists.txt"
