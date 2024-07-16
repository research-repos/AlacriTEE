CLANG    := $(shell which clang)
WASMLD   := $(shell which wasm-ld)
WASM2WAT := $(shell which wasm2wat)
EMCC     := $(shell which emcc) # install by apt install emscripten

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURR_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))


# ==============================================================================
# Configurable variables
EXECUTABLE_NAME  := enclave_event_data
# ==============================================================================


TARGET_NAME      := wasm32-unknown-emscripten
ENTRY_FUNC_NAME  := enclave_wasm_main
DEFINES          := -D_LIBCPP_ABI_VERSION=2
SYSROOT_PATH     := /usr/share/emscripten/cache/sysroot
LIB_SUBDIR       := lib/wasm32-emscripten
INCLUDE_DIRS     := -I $(CURR_DIR) \
					-I $(abspath $(CURR_DIR)/../EnclaveWasmCommon) \
					--sysroot=$(SYSROOT_PATH) \
					-Xclang -iwithsysroot/include/SDL \
					-Xclang -iwithsysroot/include/compat
LINK_LIBS        := -lc \
					-lstandalonewasm \
					-lemmalloc
NATIVE_FUNC_LIST := $(CURR_DIR)/../EnclaveWasmCommon/natives.syms

COMPILE_FLAG := -O3 \
				--target=$(TARGET_NAME) \
				$(DEFINES) \
				$(INCLUDE_DIRS) \

LINKER_FLAG  := --entry=$(ENTRY_FUNC_NAME) \
				--allow-undefined-file=$(NATIVE_FUNC_LIST) \
				--export=enclave_wasm_prerequisite_imports \
				-L$(SYSROOT_PATH)/$(LIB_SUBDIR) \
				$(LINK_LIBS)

all: $(EXECUTABLE_NAME).wasm

%.d: %.c
	$(CLANG) $(COMPILE_FLAG) -MM $^ -MF "$@"

include main.d

%.o: %.c
	$(CLANG) $(COMPILE_FLAG) -c $< -o $@

enclave_common.o: ../EnclaveWasmCommon/common.c
	$(CLANG) $(COMPILE_FLAG) -c $< -o $@

$(EXECUTABLE_NAME).wasm: main.o enclave_common.o
	$(WASMLD) $(LINKER_FLAG) -o $@ $^

clean:
	rm -f *.o *.wasm *.wat *.d

PHONY: clean all
