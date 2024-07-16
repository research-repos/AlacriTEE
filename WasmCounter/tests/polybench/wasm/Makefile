EMSDK_VER := 3.1.52
CLANG    := /opt/emsdk/emsdk-$(EMSDK_VER)/upstream/bin/clang
WASMLD   := /opt/emsdk/emsdk-$(EMSDK_VER)/upstream/bin/wasm-ld
WASM2WAT := $(shell which wasm2wat)

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURR_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))

WASM_COUNTER := $(CURR_DIR)/../../../build/src/WasmCounterUtils

TARGET_NAME      := wasm32-unknown-emscripten
ENTRY_FUNC_NAME  := enclave_wasm_main
DEFINES          := -D_LIBCPP_ABI_VERSION=2 \
					-DEMSCRIPTEN_STANDALONE_WASM \
					-DDATA_TYPE_IS_DOUBLE \
					-DPOLYBENCH_WASM
#					-DMEDIUM_DATASET
#					-DPOLYBENCH_DUMP_ARRAYS
#					-DPOLYBENCH_TIME
SYSROOT_PATH     := /opt/emsdk/emsdk-$(EMSDK_VER)/upstream/emscripten/cache/sysroot
LIB_SUBDIR       := lib/wasm32-emscripten
INCLUDE_DIRS     := -I $(CURR_DIR)/../ \
					--sysroot=$(SYSROOT_PATH) \
					-Xclang -iwithsysroot/include/SDL \
					-Xclang -iwithsysroot/include/compat
LINK_LIBS        := -lc \
					-lstandalonewasm \
					-lemmalloc
NATIVE_FUNC_LIST := $(CURR_DIR)/../enclave_wasm_natives.syms

COMPILE_FLAG := -O3 \
				--target=$(TARGET_NAME) \
				$(DEFINES) \
				$(INCLUDE_DIRS)

LINKER_FLAG  := --entry=$(ENTRY_FUNC_NAME) \
				--allow-undefined-file=$(NATIVE_FUNC_LIST) \
				--export=enclave_wasm_prerequisite_imports \
				--export=enclave_wasm_debug_marked_point \
				-L$(SYSROOT_PATH)/$(LIB_SUBDIR) \
				$(LINK_LIBS)

ALL_TESTCASES := correlation \
		covariance \
		gemm \
		gemver \
		gesummv \
		symm \
		syr2k \
		syrk \
		trmm \
		2mm \
		3mm \
		atax \
		bicg \
		doitgen \
		mvt \
		cholesky \
		durbin \
		gramschmidt \
		lu \
		ludcmp \
		trisolv \
		deriche \
		floyd-warshall \
		nussinov \
		adi \
		fdtd-2d \
		heat-3d \
		jacobi-1d \
		jacobi-2d \
		seidel-2d


all: $(ALL_TESTCASES:=.wasm) $(ALL_TESTCASES:=.nopt.wasm)


%.nopt.wasm: %.wasm
	$(WASM_COUNTER) Instrument $? $@

%.nopt.wat: %.wasm
	$(WASM_COUNTER) Instrument $? $@

# %.wat: %.wasm
# 	$(WASM2WAT) -o $@ $?

%.depend: $(CURR_DIR)/../%.c
	$(CLANG) $(COMPILE_FLAG) -MM $^ -MF "$@"

# datamining - 2
include correlation.depend
include covariance.depend
# linear-algebra - blas - 7
include gemm.depend
include gemver.depend
include gesummv.depend
include symm.depend
include syr2k.depend
include syrk.depend
include trmm.depend
# linear-algebra - kernels - 6
include 2mm.depend
include 3mm.depend
include atax.depend
include bicg.depend
include doitgen.depend
include mvt.depend
# linear-algebra - solvers - 6
include cholesky.depend
include durbin.depend
include gramschmidt.depend
include lu.depend
include ludcmp.depend
include trisolv.depend
# medley - 3
include deriche.depend
include floyd-warshall.depend
include nussinov.depend
# stencils - 6
include adi.depend
include fdtd-2d.depend
include heat-3d.depend
include jacobi-1d.depend
include jacobi-2d.depend
include seidel-2d.depend


%.wasm: %.o polybench.o enclave_wasm.o
	$(WASMLD) $(LINKER_FLAG) -o $@ $^

%.o: $(CURR_DIR)/../%.c
	$(CLANG) $(COMPILE_FLAG) -c $< -o $@

clean:
	rm -f *.o *.wasm *.wat *.depend

PHONY: clean all
