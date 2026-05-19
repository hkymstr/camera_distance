# ─── Versal AI Core stereo pipeline Makefile ───────────────────────────────
# Targets:
#   make sim        – build and run x86 software simulation (testbench)
#   make hls        – run Vitis HLS synthesis for PL kernels
#   make aie        – compile AIE kernels (requires Vitis 2024.1+)
#   make system     – full Vitis system link (PL + AIE + PS)
#   make clean      – remove build artifacts

VITIS       ?= vitis
HLS         ?= vitis_hls
AIECC       ?= aiecc.py
PLATFORM    ?= xilinx_vck190_base_202410_1  # Versal AI Core VCK190 eval board

CXX         ?= g++
CXXFLAGS    := -O2 -std=c++17 -I src/include

# ─── x86 software simulation ────────────────────────────────────────────────
sim: build/stereo_tb
	./build/stereo_tb

build/stereo_tb: tb/testbench.cpp src/pl/depth_convert.cpp src/pl/post_process.cpp \
                 src/pl/rectification.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -D__x86_sim__ $^ -o $@

# ─── Vitis HLS: synthesise PL kernels ────────────────────────────────────────
hls: build/hls_rectification build/hls_depth_convert build/hls_post_process

build/hls_%: src/pl/%.cpp src/pl/%.h
	@mkdir -p build
	$(HLS) -f scripts/hls_$*.tcl

# ─── AIE compilation ──────────────────────────────────────────────────────────
aie: build/libadf.a

build/libadf.a: src/aie/census_kernel.cpp src/aie/hamming_kernel.cpp \
                src/aie/sgm_path_kernel.cpp src/aie/stereo_graph.cpp
	@mkdir -p build
	$(AIECC) --platform=$(PLATFORM) \
	         --aie-target=aie \
	         --include=src/include \
	         --workdir=build/aie_work \
	         src/aie/stereo_graph.cpp \
	         -o build/libadf.a

# ─── Full system link ─────────────────────────────────────────────────────────
system: build/stereo_system.xsa

build/stereo_system.xsa: build/libadf.a
	$(VITIS) -f scripts/system_link.cfg

# ─── HLS Tcl scripts (generated stubs; fill in for your flow) ────────────────
scripts/hls_rectification.tcl:
	@mkdir -p scripts
	@printf 'open_project rectification\nset_top rectify\nadd_files src/pl/rectification.cpp\ncsynth_design\nexit\n' > $@

scripts/hls_depth_convert.tcl:
	@mkdir -p scripts
	@printf 'open_project depth_convert\nset_top depth_convert\nadd_files src/pl/depth_convert.cpp\ncsynth_design\nexit\n' > $@

scripts/hls_post_process.tcl:
	@mkdir -p scripts
	@printf 'open_project post_process\nset_top post_process\nadd_files src/pl/post_process.cpp\ncsynth_design\nexit\n' > $@

clean:
	rm -rf build scripts

.PHONY: sim hls aie system clean
