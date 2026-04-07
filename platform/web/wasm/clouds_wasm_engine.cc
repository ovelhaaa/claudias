#include "clouds/platform/web/wasm/clouds_wasm_engine.h"

#include <algorithm>
#include <cstring>

#include "clouds/control/parameter_mapper.h"
#include "clouds/dsp/frame.h"
#include "clouds/dsp/granular_processor.h"

namespace {

using clouds::ControlInputs;
using clouds::GranularProcessor;
using clouds::Parameters;
using clouds::PlaybackMode;
using clouds::ShortFrame;

template <typename T>
T Clamp(T value, T lo, T hi) {
  return std::max(lo, std::min(value, hi));
}

int16_t FloatToS16(float value) {
  value = Clamp(value, -1.0f, 1.0f);
  int32_t scaled = static_cast<int32_t>(value * 32767.0f);
  return static_cast<int16_t>(Clamp(scaled, -32768, 32767));
}

float S16ToFloat(int16_t value) {
  return static_cast<float>(value) / 32768.0f;
}

struct CloudsWasmEngine {
  static const int32_t kProcessBlockSize = 32;

  CloudsWasmEngine()
      : sample_rate(32000.0f),
        block_size(kProcessBlockSize),
        initialized(false),
        trigger_pending(false),
        gate(false),
        freeze(false) {
    std::memset(control, 0, sizeof(control));
    std::memset(&parameters, 0, sizeof(parameters));
    std::memset(block_in, 0, sizeof(block_in));
    std::memset(block_out, 0, sizeof(block_out));
  }

  void Init(float requested_sample_rate, int32_t requested_block_size) {
    sample_rate = requested_sample_rate > 1000.0f ? requested_sample_rate : 32000.0f;
    block_size = requested_block_size > 0 ? requested_block_size : kProcessBlockSize;

    processor.Init(large_buffer, sizeof(large_buffer), small_buffer, sizeof(small_buffer));
    processor.mutable_parameters()->trigger = false;
    processor.mutable_parameters()->gate = false;
    processor.mutable_parameters()->freeze = false;

    mapper.Init();
    controls.pitch_scale = 48.0f;
    controls.pitch_offset = 0.0f;

    controls.freeze_rising_edge = false;
    controls.freeze_falling_edge = false;
    controls.trigger_rising_edge = false;
    controls.gate = false;

    trigger_pending = false;
    gate = false;
    freeze = false;

    processor.Prepare();
    initialized = true;
  }

  void ResetState() {
    Init(sample_rate, block_size);
  }

  void SetParameter(int32_t id, float value) {
    if (id < 0 || id >= CLOUDS_PARAM_LAST) {
      return;
    }
    control[id] = Clamp(value, 0.0f, 1.0f);
  }

  void SetPlaybackMode(int32_t mode) {
    mode = Clamp(mode, 0, static_cast<int32_t>(clouds::PLAYBACK_MODE_LAST) - 1);
    processor.set_playback_mode(static_cast<PlaybackMode>(mode));
    processor.Prepare();
  }

  void SetQuality(int32_t quality_mode) {
    quality_mode = Clamp(quality_mode, 0, 3);
    processor.set_quality(quality_mode);
    processor.Prepare();
  }

  void SetFreeze(bool freeze_value) {
    freeze = freeze_value;
  }

  void SetGate(bool gate_value) {
    gate = gate_value;
  }

  void TriggerPulse() {
    trigger_pending = true;
  }

  void ProcessInterleaved(const float* input_f32, float* output_f32, int32_t num_frames) {
    if (!initialized || num_frames <= 0 || !output_f32) {
      return;
    }

    int32_t frame_index = 0;
    while (frame_index < num_frames) {
      const int32_t chunk = std::min(kProcessBlockSize, num_frames - frame_index);
      for (int32_t i = 0; i < chunk; ++i) {
        float in_l = input_f32 ? input_f32[(frame_index + i) * 2] : 0.0f;
        float in_r = input_f32 ? input_f32[(frame_index + i) * 2 + 1] : in_l;
        block_in[i].l = FloatToS16(in_l);
        block_in[i].r = FloatToS16(in_r);
      }
      for (int32_t i = chunk; i < kProcessBlockSize; ++i) {
        block_in[i].l = 0;
        block_in[i].r = 0;
      }

      controls.control[0] = control[CLOUDS_PARAM_POSITION];
      controls.control[1] = control[CLOUDS_PARAM_DENSITY];
      controls.control[2] = control[CLOUDS_PARAM_SIZE];
      controls.control[3] = 0.0f;
      controls.control[4] = control[CLOUDS_PARAM_PITCH];
      controls.control[5] = 0.0f;
      controls.control[6] = control[CLOUDS_PARAM_BLEND];
      controls.control[7] = 0.0f;
      controls.control[8] = control[CLOUDS_PARAM_TEXTURE];
      controls.control[9] = 0.0f;

      mapper.set_blend_parameter(clouds::BLEND_PARAMETER_DRY_WET);
      mapper.set_blend_value(clouds::BLEND_PARAMETER_DRY_WET, control[CLOUDS_PARAM_BLEND]);
      mapper.set_blend_value(clouds::BLEND_PARAMETER_STEREO_SPREAD, control[CLOUDS_PARAM_SPREAD]);
      mapper.set_blend_value(clouds::BLEND_PARAMETER_FEEDBACK, control[CLOUDS_PARAM_FEEDBACK]);
      mapper.set_blend_value(clouds::BLEND_PARAMETER_REVERB, control[CLOUDS_PARAM_REVERB]);

      controls.freeze_rising_edge = freeze && !processor.parameters().freeze;
      controls.freeze_falling_edge = !freeze && processor.parameters().freeze;
      controls.trigger_rising_edge = trigger_pending;
      controls.gate = gate;

      mapper.MapToParameters(controls, &parameters);
      *processor.mutable_parameters() = parameters;
      processor.Process(block_in, block_out, kProcessBlockSize);
      processor.Prepare();
      trigger_pending = false;

      for (int32_t i = 0; i < chunk; ++i) {
        output_f32[(frame_index + i) * 2] = S16ToFloat(block_out[i].l);
        output_f32[(frame_index + i) * 2 + 1] = S16ToFloat(block_out[i].r);
      }
      frame_index += chunk;
    }
  }

  float sample_rate;
  int32_t block_size;
  bool initialized;
  bool trigger_pending;
  bool gate;
  bool freeze;

  float control[CLOUDS_PARAM_LAST];
  ControlInputs controls;
  Parameters parameters;

  GranularProcessor processor;
  clouds::ParameterMapper mapper;

  alignas(16) uint8_t large_buffer[118784];
  alignas(16) uint8_t small_buffer[65536 - 128];

  ShortFrame block_in[kProcessBlockSize];
  ShortFrame block_out[kProcessBlockSize];
};

}  // namespace

struct clouds_engine_t {
  CloudsWasmEngine impl;
};

extern "C" {

clouds_engine_t* clouds_create_engine() {
  return new clouds_engine_t();
}

void clouds_destroy_engine(clouds_engine_t* engine) {
  delete engine;
}

void clouds_init_engine(clouds_engine_t* engine, float sample_rate, int32_t block_size) {
  if (!engine) {
    return;
  }
  engine->impl.Init(sample_rate, block_size);
}

void clouds_set_parameter(clouds_engine_t* engine, int32_t id, float value) {
  if (!engine) {
    return;
  }
  engine->impl.SetParameter(id, value);
}

void clouds_set_playback_mode(clouds_engine_t* engine, int32_t mode) {
  if (!engine) {
    return;
  }
  engine->impl.SetPlaybackMode(mode);
}

void clouds_set_quality(clouds_engine_t* engine, int32_t quality_mode) {
  if (!engine) {
    return;
  }
  engine->impl.SetQuality(quality_mode);
}

void clouds_set_freeze(clouds_engine_t* engine, bool freeze) {
  if (!engine) {
    return;
  }
  engine->impl.SetFreeze(freeze);
}

void clouds_set_gate(clouds_engine_t* engine, bool gate) {
  if (!engine) {
    return;
  }
  engine->impl.SetGate(gate);
}

void clouds_trigger_pulse(clouds_engine_t* engine) {
  if (!engine) {
    return;
  }
  engine->impl.TriggerPulse();
}

void clouds_reset_state(clouds_engine_t* engine) {
  if (!engine) {
    return;
  }
  engine->impl.ResetState();
}

void clouds_process_interleaved(
    clouds_engine_t* engine,
    const float* input_f32,
    float* output_f32,
    int32_t num_frames) {
  if (!engine) {
    return;
  }
  engine->impl.ProcessInterleaved(input_f32, output_f32, num_frames);
}

}  // extern "C"
