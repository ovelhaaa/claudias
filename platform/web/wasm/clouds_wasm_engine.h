#ifndef CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_
#define CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clouds_engine_t clouds_engine_t;

enum {
  CLOUDS_PARAM_POSITION = 0,
  CLOUDS_PARAM_SIZE = 1,
  CLOUDS_PARAM_PITCH = 2,
  CLOUDS_PARAM_DENSITY = 3,
  CLOUDS_PARAM_TEXTURE = 4,
  CLOUDS_PARAM_DRY_WET = 5,
  CLOUDS_PARAM_SPREAD = 6,
  CLOUDS_PARAM_FEEDBACK = 7,
  CLOUDS_PARAM_REVERB = 8,
  CLOUDS_PARAM_LAST = 9,
};

clouds_engine_t* clouds_create_engine();
void clouds_destroy_engine(clouds_engine_t* engine);

void clouds_init_engine(clouds_engine_t* engine, float sample_rate, int32_t block_size);
void clouds_set_parameter(clouds_engine_t* engine, int32_t id, float value);
void clouds_set_playback_mode(clouds_engine_t* engine, int32_t mode);
void clouds_set_quality(clouds_engine_t* engine, int32_t quality_mode);
void clouds_set_freeze(clouds_engine_t* engine, bool freeze);
void clouds_set_gate(clouds_engine_t* engine, bool gate);
void clouds_trigger_pulse(clouds_engine_t* engine);
void clouds_reset_state(clouds_engine_t* engine);

void clouds_process_interleaved(
    clouds_engine_t* engine,
    const float* input_f32,
    float* output_f32,
    int32_t num_frames);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CLOUDS_PLATFORM_WEB_WASM_CLOUDS_WASM_ENGINE_H_
