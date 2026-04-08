TARGET := clouds_wasm_engine
OUT_DIR := build
INCLUDE_DIR := $(OUT_DIR)/include
ROOT_DIR := $(abspath ../../..)
WORKSPACE_DIR := $(abspath ../../../..)

STMLIB_LOCAL_HEADER := $(ROOT_DIR)/stmlib/stmlib.h
EURORACK_REPO ?= https://github.com/pichenettes/eurorack.git
EURORACK_DIR := $(OUT_DIR)/deps/eurorack

ifeq ($(wildcard $(STMLIB_LOCAL_HEADER)),)
STMLIB_INCLUDE_ROOT := $(EURORACK_DIR)
STMLIB_PREREQ := $(EURORACK_DIR)/stmlib/stmlib.h
else
STMLIB_INCLUDE_ROOT := $(ROOT_DIR)
STMLIB_PREREQ :=
endif

EMXX ?= em++
CXXFLAGS := -O3 -DNDEBUG -std=c++11 -Wall -Werror -I$(ROOT_DIR) -I$(WORKSPACE_DIR) -I$(STMLIB_INCLUDE_ROOT) -I$(INCLUDE_DIR)
LDFLAGS := \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT='web,worker' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='[_malloc,_free,_clouds_create_engine,_clouds_destroy_engine,_clouds_init_engine,_clouds_set_parameter,_clouds_set_playback_mode,_clouds_set_quality,_clouds_set_freeze,_clouds_set_gate,_clouds_trigger_pulse,_clouds_reset_state,_clouds_process_interleaved]' \
  -s EXPORTED_RUNTIME_METHODS='[ccall,cwrap]'

SOURCES := \
  clouds_wasm_engine.cc \
  ../../../control/parameter_mapper.cc \
  ../../../dsp/correlator.cc \
  ../../../dsp/granular_processor.cc \
  ../../../dsp/mu_law.cc \
  ../../../dsp/pvoc/frame_transformation.cc \
  ../../../dsp/pvoc/phase_vocoder.cc \
  ../../../dsp/pvoc/stft.cc \
  ../../../resources.cc

all: $(OUT_DIR)/$(TARGET).js

$(EURORACK_DIR)/stmlib/stmlib.h:
	mkdir -p $(dir $(EURORACK_DIR))
	if [ ! -d $(EURORACK_DIR)/.git ]; then \
		git clone --depth 1 --recurse-submodules --shallow-submodules $(EURORACK_REPO) $(EURORACK_DIR); \
	fi

$(OUT_DIR)/$(TARGET).js: $(SOURCES) clouds_wasm_engine.h $(STMLIB_PREREQ)
	mkdir -p $(OUT_DIR) $(INCLUDE_DIR)
	ln -sfn $(ROOT_DIR) $(INCLUDE_DIR)/clouds
	$(EMXX) $(CXXFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
