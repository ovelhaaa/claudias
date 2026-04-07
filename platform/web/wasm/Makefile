EMCC ?= em++

ROOT := ../../..
OUT_DIR := $(ROOT)/build/wasm
TARGET := $(OUT_DIR)/clouds_wasm_engine

CXXFLAGS := -O3 -DNDEBUG -std=c++11 -Wall -Wextra -I$(ROOT) -I$(ROOT)/..
LDFLAGS := \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="CloudsWasm" \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
	-s EXPORTED_FUNCTIONS='["_malloc","_free","_clouds_create_engine","_clouds_destroy_engine","_clouds_init_engine","_clouds_set_parameter","_clouds_set_playback_mode","_clouds_set_quality","_clouds_set_freeze","_clouds_set_gate","_clouds_trigger_pulse","_clouds_reset_state","_clouds_process_interleaved"]'

SRCS := \
	$(ROOT)/dsp/mu_law.cc \
	$(ROOT)/dsp/correlator.cc \
	$(ROOT)/dsp/granular_processor.cc \
	$(ROOT)/dsp/pvoc/frame_transformation.cc \
	$(ROOT)/dsp/pvoc/phase_vocoder.cc \
	$(ROOT)/dsp/pvoc/stft.cc \
	$(ROOT)/control/parameter_mapper.cc \
	$(ROOT)/resources.cc \
	$(ROOT)/platform/web/wasm/clouds_wasm_engine.cc

all: $(TARGET).js

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(TARGET).js: $(OUT_DIR) $(SRCS)
	$(EMCC) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET).js $(TARGET).wasm

.PHONY: all clean
