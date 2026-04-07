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
//
// -----------------------------------------------------------------------------
//
// Calibration settings.

#include "clouds/cv_scaler.h"

namespace clouds {

namespace {

const bool kFlip[ADC_CHANNEL_LAST] = {
  true,
  true,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false
};

const bool kRemoveOffset[ADC_CHANNEL_LAST] = {
  false,
  false,
  false,
  true,
  false,
  false,
  false,
  true,
  false,
  true
};

}  // namespace

void CvScaler::Init(CalibrationData* calibration_data) {
  adc_.Init();
  gate_input_.Init();
  calibration_data_ = calibration_data;
  parameter_mapper_.Init();
}

void CvScaler::Read(Parameters* parameters) {
  ControlInputs inputs;
  for (size_t i = 0; i < ADC_CHANNEL_LAST; ++i) {
    float value = adc_.float_value(i);
    if (kFlip[i]) {
      value = 1.0f - value;
    }
    if (kRemoveOffset[i]) {
      value -= calibration_data_->offset[i];
    }
    inputs.control[i] = value;
  }

  gate_input_.Read();
  inputs.freeze_rising_edge = gate_input_.freeze_rising_edge();
  inputs.freeze_falling_edge = gate_input_.freeze_falling_edge();
  inputs.trigger_rising_edge = gate_input_.trigger_rising_edge();
  inputs.gate = gate_input_.gate();
  inputs.pitch_scale = calibration_data_->pitch_scale;
  inputs.pitch_offset = calibration_data_->pitch_offset;

  parameter_mapper_.MapToParameters(inputs, parameters);

  adc_.Convert();
}

}  // namespace clouds
