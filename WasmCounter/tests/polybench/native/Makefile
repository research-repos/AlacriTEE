CLANG    := $(shell which clang)
CLANGPP  := $(shell which clang++)
LD       := $(shell which clang++)

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURR_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))

TARGET_NAME      := x86_64-unknown-linux
DEFINES          := -DDATA_TYPE_IS_DOUBLE \
					-DPOLYBENCH_WASM
#					-DMEDIUM_DATASET
INCLUDE_DIRS := -I $(CURR_DIR)/../
COMPILE_FLAG := -O3 \
				--target=$(TARGET_NAME) \
				$(DEFINES) \
				$(INCLUDE_DIRS)
# sudo apt-get install libc++-dev libc++abi-dev
CXX_COMPILE_FLAG := $(COMPILE_FLAG) \
					-stdlib=libc++ \
					-std=c++11 \
					-static

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

all: $(ALL_TESTCASES:=.app)


include WasmNative.dependpp
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


%.app: %.o WasmNative.o polybench.o enclave_wasm.o
	$(CLANGPP) $(CXX_COMPILE_FLAG) $^ -o $@


%.dependpp: $(CURR_DIR)/../%.cpp
	$(CLANGPP) $(CXX_COMPILE_FLAG) -MM $^ -MF "$@"


%.depend: $(CURR_DIR)/../%.c
	$(CLANG) $(COMPILE_FLAG) -MM $^ -MF "$@"


%.o: $(CURR_DIR)/../%.c
	$(CLANG) $(COMPILE_FLAG) -c $< -o $@


WasmNative.o: $(CURR_DIR)/../WasmNative.cpp
	$(CLANGPP) $(CXX_COMPILE_FLAG) -c $< -o $@


clean:
	rm -f *.o *.app *.depend *.dependpp


%.run : %.app
	./$<


run_all : $(ALL_TESTCASES:=.run)

PHONY: clean all run_all

