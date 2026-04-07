#include "clouds/platform/web/wasm/clouds_wasm_engine.h"

#include <algorithm>
#include <cstring>

#include "clouds/control/parameter_mapper.h"
#include "clouds/dsp/granular_processor.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace clouds {
namespace {

const size_t kFirmwareLargeBufferSize = 118784;
const size_t kFirmwareSmallBufferSize = 65536 - 128;
const size_t kProcessorBlockSize = 32;

inline float Clamp(float value, float lo, float hi) {
  return std::max(lo, std::min(value, hi));
}

inline short FloatToShort(float v) {
  v = Clamp(v, -1.0f, 1.0f);
  if (v >= 0.0f) {
    return static_cast<short>(v * 32767.0f);
  }
  return static_cast<short>(v * 32768.0f);
}

inline float ShortToFloat(short v) {
  return static_cast<float>(v) / 32768.0f;
}

}  // namespace

struct CloudsWasmEngine {
  GranularProcessor processor;
  ParameterMapper mapper;
  ControlInputs controls;

  int sample_rate;
  int host_block_size;
  int prepare_divider;
  int prepare_counter;

  PlaybackMode playback_mode;
  int quality_mode;

  uint8_t large_buffer[kFirmwareLargeBufferSize];
  uint8_t small_buffer[kFirmwareSmallBufferSize];

  ShortFrame in[kProcessorBlockSize];
  ShortFrame out[kProcessorBlockSize];

  CloudsWasmEngine()
      : sample_rate(32000),
        host_block_size(kProcessorBlockSize),
        prepare_divider(1),
        prepare_counter(0),
        playback_mode(PLAYBACK_MODE_GRANULAR),
        quality_mode(0) {
    std::memset(&controls, 0, sizeof(controls));
    controls.pitch_scale = 12.0f;
    mapper.Init();
    processor.Init(
        large_buffer,
        sizeof(large_buffer),
        small_buffer,
        sizeof(small_buffer));
    processor.set_playback_mode(playback_mode);
    processor.set_quality(quality_mode);
    processor.Prepare();
  }

  void Reset() {
    std::memset(&controls, 0, sizeof(controls));
    controls.pitch_scale = 12.0f;
    mapper.Init();
    processor.Init(
        large_buffer,
        sizeof(large_buffer),
        small_buffer,
        sizeof(small_buffer));
    processor.set_playback_mode(playback_mode);
    processor.set_quality(quality_mode);
    processor.Prepare();
    prepare_counter = 0;
  }

  void UpdatePrepareCadence() {
    if (sample_rate <= 0) {
      prepare_divider = 1;
      return;
    }
    int base = sample_rate / 1000;
    if (base < 1) {
      base = 1;
    }
    prepare_divider = std::max(1, base / static_cast<int>(kProcessorBlockSize));
  }

  void MaybePrepare() {
    if (prepare_counter == 0) {
      processor.Prepare();
    }
    prepare_counter = (prepare_counter + 1) % prepare_divider;
  }

  void MapControls() {
    mapper.MapToParameters(controls, processor.mutable_parameters());
    controls.freeze_rising_edge = false;
    controls.freeze_falling_edge = false;
    controls.trigger_rising_edge = false;
  }
};

}  // namespace clouds

using clouds::CloudsWasmEngine;

extern "C" {

EMSCRIPTEN_KEEPALIVE
CloudsWasmEngine* clouds_create_engine() {
  return new CloudsWasmEngine();
}

EMSCRIPTEN_KEEPALIVE
void clouds_destroy_engine(CloudsWasmEngine* engine) {
  delete engine;
}

EMSCRIPTEN_KEEPALIVE
int clouds_init_engine(CloudsWasmEngine* engine, int sample_rate, int block_size) {
  if (!engine) {
    return 0;
  }
  engine->sample_rate = sample_rate;
  engine->host_block_size = block_size > 0 ? block_size : 32;
  engine->UpdatePrepareCadence();
  engine->Reset();
  return sample_rate == 32000 ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void clouds_set_parameter(CloudsWasmEngine* engine, int id, float value) {
  if (!engine) {
    return;
  }
  float v = clouds::Clamp(value, 0.0f, 1.0f);
  switch (id) {
    case CLOUDS_PARAM_POSITION:
      engine->controls.control[clouds::ADC_POSITION_POTENTIOMETER_CV] = v;
      break;
    case CLOUDS_PARAM_DENSITY:
      engine->controls.control[clouds::ADC_DENSITY_POTENTIOMETER_CV] = v;
      break;
    case CLOUDS_PARAM_SIZE:
      engine->controls.control[clouds::ADC_SIZE_POTENTIOMETER] = v;
      break;
    case CLOUDS_PARAM_PITCH:
      engine->controls.control[clouds::ADC_PITCH_POTENTIOMETER] = v;
      break;
    case CLOUDS_PARAM_BLEND:
      engine->controls.control[clouds::ADC_BLEND_POTENTIOMETER] = v;
      break;
    case CLOUDS_PARAM_TEXTURE:
      engine->controls.control[clouds::ADC_TEXTURE_POTENTIOMETER] = v;
      break;
    case CLOUDS_PARAM_V_OCT:
      engine->controls.control[clouds::ADC_V_OCT_CV] = v;
      break;
    case CLOUDS_PARAM_FREEZE_CV:
      engine->controls.control[clouds::ADC_SIZE_CV] = v;
      break;
    case CLOUDS_PARAM_TRIGGER:
      engine->controls.trigger_rising_edge = value > 0.5f;
      break;
    case CLOUDS_PARAM_GATE:
      engine->controls.gate = value > 0.5f;
      break;
    case CLOUDS_PARAM_BLEND_MODE: {
      int mode = static_cast<int>(value);
      mode = std::max(0, std::min(mode, static_cast<int>(clouds::BLEND_PARAMETER_LAST) - 1));
      engine->mapper.set_blend_parameter(static_cast<clouds::BlendParameter>(mode));
      break;
    }
    case CLOUDS_PARAM_PITCH_OFFSET:
      engine->controls.pitch_offset = value;
      break;
    case CLOUDS_PARAM_PITCH_SCALE:
      engine->controls.pitch_scale = value;
      break;
    default:
      break;
  }
}

EMSCRIPTEN_KEEPALIVE
void clouds_set_playback_mode(CloudsWasmEngine* engine, int mode) {
  if (!engine) {
    return;
  }
  mode = std::max(0, std::min(mode, static_cast<int>(clouds::PLAYBACK_MODE_LAST) - 1));
  engine->playback_mode = static_cast<clouds::PlaybackMode>(mode);
  engine->processor.set_playback_mode(engine->playback_mode);
}

EMSCRIPTEN_KEEPALIVE
void clouds_set_quality(CloudsWasmEngine* engine, int quality_mode) {
  if (!engine) {
    return;
  }
  engine->quality_mode = quality_mode & 0x3;
  engine->processor.set_quality(engine->quality_mode);
}

EMSCRIPTEN_KEEPALIVE
void clouds_set_freeze(CloudsWasmEngine* engine, int freeze) {
  if (!engine) {
    return;
  }
  bool target = freeze != 0;
  bool current = engine->processor.parameters().freeze;
  engine->controls.freeze_rising_edge = !current && target;
  engine->controls.freeze_falling_edge = current && !target;
}

EMSCRIPTEN_KEEPALIVE
void clouds_set_gate(CloudsWasmEngine* engine, int gate) {
  if (!engine) {
    return;
  }
  engine->controls.gate = gate != 0;
}

EMSCRIPTEN_KEEPALIVE
void clouds_trigger_pulse(CloudsWasmEngine* engine) {
  if (!engine) {
    return;
  }
  engine->controls.trigger_rising_edge = true;
}

EMSCRIPTEN_KEEPALIVE
void clouds_reset_state(CloudsWasmEngine* engine) {
  if (!engine) {
    return;
  }
  engine->Reset();
}

EMSCRIPTEN_KEEPALIVE
void clouds_process_interleaved(
    CloudsWasmEngine* engine,
    const float* input_f32,
    float* output_f32,
    size_t num_frames) {
  if (!engine || !output_f32) {
    return;
  }

  size_t processed = 0;
  while (processed < num_frames) {
    const size_t chunk = std::min(num_frames - processed, static_cast<size_t>(32));

    for (size_t i = 0; i < chunk; ++i) {
      float l = input_f32 ? input_f32[(processed + i) * 2] : 0.0f;
      float r = input_f32 ? input_f32[(processed + i) * 2 + 1] : 0.0f;
      engine->in[i].l = clouds::FloatToShort(l);
      engine->in[i].r = clouds::FloatToShort(r);
    }

    for (size_t i = chunk; i < 32; ++i) {
      engine->in[i].l = 0;
      engine->in[i].r = 0;
    }

    engine->MapControls();
    engine->MaybePrepare();
    engine->processor.Process(engine->in, engine->out, 32);

    for (size_t i = 0; i < chunk; ++i) {
      output_f32[(processed + i) * 2] = clouds::ShortToFloat(engine->out[i].l);
      output_f32[(processed + i) * 2 + 1] = clouds::ShortToFloat(engine->out[i].r);
    }

    processed += chunk;
  }
}

}  // extern "C"
