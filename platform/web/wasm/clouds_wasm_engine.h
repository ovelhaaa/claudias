#ifndef CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_
#define CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CloudsWasmEngine CloudsWasmEngine;

enum CloudsWasmParameterId {
  CLOUDS_PARAM_POSITION = 0,
  CLOUDS_PARAM_DENSITY = 1,
  CLOUDS_PARAM_SIZE = 2,
  CLOUDS_PARAM_PITCH = 3,
  CLOUDS_PARAM_BLEND = 4,
  CLOUDS_PARAM_TEXTURE = 5,
  CLOUDS_PARAM_V_OCT = 6,
  CLOUDS_PARAM_FREEZE_CV = 7,
  CLOUDS_PARAM_TRIGGER = 8,
  CLOUDS_PARAM_GATE = 9,
  CLOUDS_PARAM_BLEND_MODE = 10,
  CLOUDS_PARAM_PITCH_OFFSET = 11,
  CLOUDS_PARAM_PITCH_SCALE = 12,
};

CloudsWasmEngine* clouds_create_engine();
void clouds_destroy_engine(CloudsWasmEngine* engine);

int clouds_init_engine(
    CloudsWasmEngine* engine,
    int sample_rate,
    int block_size);

void clouds_set_parameter(CloudsWasmEngine* engine, int id, float value);
void clouds_set_playback_mode(CloudsWasmEngine* engine, int mode);
void clouds_set_quality(CloudsWasmEngine* engine, int quality_mode);

void clouds_set_freeze(CloudsWasmEngine* engine, int freeze);
void clouds_set_gate(CloudsWasmEngine* engine, int gate);
void clouds_trigger_pulse(CloudsWasmEngine* engine);

void clouds_reset_state(CloudsWasmEngine* engine);

void clouds_process_interleaved(
    CloudsWasmEngine* engine,
    const float* input_f32,
    float* output_f32,
    size_t num_frames);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_
