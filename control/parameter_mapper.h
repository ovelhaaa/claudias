// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#ifndef CLOUDS_CONTROL_PARAMETER_MAPPER_H_
#define CLOUDS_CONTROL_PARAMETER_MAPPER_H_

#include "stmlib/stmlib.h"

#include "clouds/drivers/adc.h"
#include "clouds/dsp/parameters.h"

namespace clouds {

enum BlendParameter {
  BLEND_PARAMETER_DRY_WET,
  BLEND_PARAMETER_STEREO_SPREAD,
  BLEND_PARAMETER_FEEDBACK,
  BLEND_PARAMETER_REVERB,
  BLEND_PARAMETER_LAST
};

struct ControlInputs {
  float control[ADC_CHANNEL_LAST];
  float pitch_scale;
  float pitch_offset;
  bool freeze_rising_edge;
  bool freeze_falling_edge;
  bool trigger_rising_edge;
  bool gate;
};

class ParameterMapper {
 public:
  ParameterMapper() { }
  ~ParameterMapper() { }

  void Init();
  void MapToParameters(const ControlInputs& inputs, Parameters* parameters);

  inline void set_blend_parameter(BlendParameter parameter) {
    blend_parameter_ = parameter;
    blend_knob_origin_ = previous_blend_knob_value_;
  }

  inline void MatchKnobPosition() {
    previous_blend_knob_value_ = -1.0f;
  }

  inline BlendParameter blend_parameter() const {
    return blend_parameter_;
  }

  inline float blend_value(BlendParameter parameter) const {
    return blend_[parameter];
  }

  inline void set_blend_value(BlendParameter parameter, float value) {
    blend_[parameter] = value;
  }

  inline bool blend_knob_touched() const {
    return blend_knob_touched_;
  }

  inline void UnlockBlendKnob() {
    previous_blend_knob_value_ = -1.0f;
  }

 private:
  void UpdateBlendParameters(float knob, float cv);

  static const int kAdcLatency = 5;

  float smoothed_adc_value_[ADC_CHANNEL_LAST];
  float note_;

  BlendParameter blend_parameter_;
  float blend_[BLEND_PARAMETER_LAST];
  float blend_mod_[BLEND_PARAMETER_LAST];
  float previous_blend_knob_value_;

  float blend_knob_origin_;
  float blend_knob_quantized_;
  bool blend_knob_touched_;

  bool previous_trigger_[kAdcLatency];
  bool previous_gate_[kAdcLatency];

  DISALLOW_COPY_AND_ASSIGN(ParameterMapper);
};

}  // namespace clouds

#endif  // CLOUDS_CONTROL_PARAMETER_MAPPER_H_
