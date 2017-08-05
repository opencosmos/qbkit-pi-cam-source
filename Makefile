### Master makefile

# Some variables (e.g. libs, cflags) are lists that can be appended (+=),
# others are plain strings (strip_debug, sync_mode) that can be reassibned (=)
# as needed.

# Included makefiles might override this with "line" or "none"
sync_mode = target

# Configure make
MAKEFLAGS += -rR --output-sync=$(sync_mode)
.DELETE_ON_ERROR:
.SECONDARY:

# Configure shell
SHELL = bash
.SHELLFLAGS = -euo pipefail -c

# Space and non-breaking space
spacebound =
space = $(spacebound) $(spacebound)
nbsp =  

# -g or -s
strip_debug = -g

# Warning flags
w_flags = -Wall -Wextra -Wno-unused-parameter -Werror

# Common to both cflags and cxxflags
c_flags = -ffunction-sections -fdata-sections -O$(o) $(strip_debug)
# Linker flags
ldflags = -Wl,--gc-sections -O$(o) $(strip_debug)
# Assembler flags
asflags =
# Specific to either cflags or cxxflags
cflags =
cxxflags =

# C / C++ standards to use
c_std = gnu11
cxx_std = gnu++14

# Preprocessor definitions
defines =
# Includes
include_dirs = $(shell find -type d -name 'include')
includes =
# Libraries to link against
libs =

ifeq ($(V),)
MAKEFLAGS += -s
endif

# Language of program (used to choose gcc/g++ for linking)
language ?= c

# Name of program (used to generate output filename(s))
project_name = program

# Optimisiation level
o := $(firstword $(o) $(O) g)

# Build directory
build_base = .build
build_profile = $(o)
build_dir = $(build_base)/$(build_profile)

# Output directory
outdir = bin

# Temporary directory
tmpdir = tmp

# Useful logging functions
shell_quote = '$(subst ','\\'',$(strip $(1)))'

define log_action
	@printf "  $$(tput bold)[%s]$$(tput sgr0)\t %s\n" $(call shell_quote, $(1)) $(call shell_quote, $(2))
endef

define log_error
	printf "      \t $$(tput bold)$$(tput setaf 1)%s$$(tput sgr0)\n" $(call shell_quote, $(1)); false
endef

define log_info
$(shell [ "$(V)" ] && printf " $$(tput bold)-$$(tput sgr0) $$(tput sitm)%s$$(tput sgr0)\n" $(call shell_quote, $(1)) >&2)
endef

define log_var
$(call log_info, $(1) = $($(strip $1)))
endef

# Output filenames
program = $(outdir)/$(project_name)

# Source file collection parameters
source_root = .
source_exclude = $(tmpdir) $(outdir)

# Include project makefile if one exists
-include Makefile.project

# Include metadata makefile if one exists
-include Makefile.metadata

$(call log_var, o)

$(call log_var, build_profile)
$(call log_var, build_dir)

# Create directories as needed, update symlinks
$(shell ( \
	mkdir -p -- $(build_dir)/$(outdir); \
	mkdir -p -- $(build_dir)/$(tmpdir); \
	rm -f $(outdir) $(tmpdir); \
	ln -sf $(build_dir)/$(outdir) $(outdir); \
	ln -sf $(build_dir)/$(tmpdir) $(tmpdir); \
) >&2 )

# Function to make list of source files
collect_files = $(patsubst $(source_root)/%.$(strip $1), $(tmpdir)/%.$(strip $2), $(shell find $(source_root) -name '*.$(strip $1)' -type f $(source_exclude:%=-not -path './%/*' )))

$(call log_var, program)
$(call log_var, source_root)
$(call log_var, source_exclude)

# Objects
objects += $(call collect_files, s, os)
objects += $(call collect_files, c, o)
objects += $(call collect_files, cpp, oxx)

$(call log_var, objects)

# Toolchain configuration
CROSS_COMPILE ?= $(cross_compile)

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
OBJCOPY = $(CROSS_COMPILE)objcopy
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
GDB = $(CROSS_COMPILE)gdb

ifeq ($(language),c)
LD = $(CC)
endif

ifeq ($(language),c++)
LD = $(CXX)
endif

$(call log_var, CROSS_COMPILE)
$(call log_var, AS)
$(call log_var, CC)
$(call log_var, CXX)
$(call log_var, AR)
$(call log_var, LD)
$(call log_var, OBJCOPY)
$(call log_var, GDB)

-include Makefile.objects

# Compiler/linker flags
c_flags += $(w_flags) -MMD -MP -MF $@.d -c
c_flags += $(addprefix -I,$(include_dirs))
c_flags += $(addprefix -i,$(includes))
c_flags += $(subst $(nbsp),$(space),$(addprefix -D,$(defines)))

cflags += $(c_flags) -std=$(c_std)

cxxflags += $(c_flags) -std=$(cxx_std)

ldflags += $(w_flags)
ldflags += $(addprefix -l,$(libs))

# Link-time optimisation enabled if optimisation level is above "debug"
ifneq ($(filter-out 0 g, $(O)),)
use_lto := $(firstword $(use_lto) y)
else
use_lto := $(firstword $(use_lto) n)
endif

# Note: LTO breaks the symbol redefinition trick used to generate test binary
ifeq ($(use_lto),y)
c_flags += -flto
ldflags += -flto
endif

$(call log_var, cflags)
$(call log_var, cxxflags)
$(call log_var, ldflags)

.PHONY: all
all: $(program)

.PHONY: build
build: $(program)

.PHONY: clean
clean:: cleanlinks
	$(call log_action, RM, $(build_dir))
	rm -rf $(build_dir)

.PHONY: cleanlinks
cleanlinks::
	$(call log_action, RM, $(outdir))
	rm -rf $(outdir)
	$(call log_action, RM, $(tmpdir))
	rm -rf $(tmpdir)

.PHONY: cleanall
cleanall:: cleanlinks
	$(call log_action, RM, $(build_base))
	rm -rf $(build_base)

$(program): $(objects)
	@mkdir -p -- $(@D)
	$(call log_action, LD, $@)
	$(LD) $^ -o $@ $(ldflags)
	@size $@

$(filter %.o, $(objects)): $(tmpdir)/%.o: %.c
	@mkdir -p -- $(@D)
	$(call log_action, CC, $@)
	$(CC) $(cflags) -o $@ $<

$(filter %.oxx, $(objects)): $(tmpdir)/%.oxx: %.cpp
	@mkdir -p -- $(@D)
	$(call log_action, CXX, $@)
	$(CXX) $(cxxflags) -o $@ $<

$(filter %.os, $(objects)): $(tmpdir)/%.os: %.s
	@mkdir -p -- $(@D)
	$(call log_action, AS, $@)
	$(AS) $(asflags) -o $@ $<

# Include makefile for test runner, if available
-include Makefile.test

-include $(shell find $(tmpdir) -name '*.d' -type f 2>/dev/null)
