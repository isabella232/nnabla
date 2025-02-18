// Copyright 2018,2019,2020,2021 Sony Corporation.
// Copyright 2021 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// dropout.cpp

#include <nbla/array.hpp>
#include <nbla/function/dropout.hpp>
#include <nbla/function/utils/dropout_workaround.hpp>
#include <nbla/random_manager.hpp>
#include <nbla/variable.hpp>

#include <algorithm>

namespace nbla {

NBLA_REGISTER_FUNCTION_SOURCE(Dropout, double, int);

template <typename T>
void Dropout<T>::setup_impl(const Variables &inputs, const Variables &outputs) {
  NBLA_CHECK(p_ >= 0. && p_ < 1., error_code::value,
             "p must be between 0.0 and 1.0. p: %f.", p_);
  outputs[0]->reshape(inputs[0]->shape(), true);

  // This is a temporary workaround to share the member variable "mask" of
  // the function Dropout to the derivative function dropout_backward
  // without changing the backward compatibility of their user interfaces.
  // This workaround depends on GradEndFunction of the nnabla.grad scheme.
  // It gurantees the computation order (rank) from Dropout to dropout_backward.
  // However this workaround is dangerous because the dependency is implicit.
  // TODO: the overall refactoring of foward/backward/nnabla.grad to solve
  //       this problem fundamentally.
  mask_ = make_shared<Variable>(inputs[0]->shape());
  set_dropout_mask(inputs[0], mask_);

  std::random_device rdev_;
  rgen_ = std::mt19937((seed_ == -1 ? rdev_() : seed_));
  rdist_ = std::bernoulli_distribution(1 - p_);
  scale_ = 1. / (1. - p_);
}

template <typename T>
void Dropout<T>::setup_recompute_impl(const Variables &inputs,
                                      const Variables &outputs) {
  save_rng_ = true;
}

template <class T>
void Dropout<T>::dropout(const Variables &inputs, const Variables &outputs,
                         std::mt19937 &rgen) {
  const T *x = inputs[0]->get_data_pointer<T>(this->ctx_);
  T *y = outputs[0]->cast_data_and_get_pointer<T>(this->ctx_, true);
  T *m = this->mask_->template cast_data_and_get_pointer<T>(this->ctx_, true);
  for (int s = 0; s < inputs[0]->size(); s++) {
    m[s] = rdist_(rgen);
    y[s] = x[s] * m[s] * scale_;
  }
}

template <typename T>
void Dropout<T>::forward_impl(const Variables &inputs,
                              const Variables &outputs) {
  std::mt19937 &rgen =
      seed_ == -1 ? SingletonManager::get<RandomManager>()->get_rand_generator()
                  : rgen_;
  // Remember the random state for recomputation.
  if (save_rng_) {
    rgen_for_recompute_ = rgen;
  }

  dropout(inputs, outputs, rgen);
}

template <typename T>
void Dropout<T>::recompute_impl(const Variables &inputs,
                                const Variables &outputs) {
  auto rgen = rgen_for_recompute_;
  dropout(inputs, outputs, rgen);
}

template <typename T> void Dropout<T>::clear_buffer() {
  // mask can be cleared after backward_impl because GradEndFunction guarantees
  // that mask is used lastly here even when the second derivative of Dropout
  // is computed. Additionally, the mask will be released when
  // forward(clear_buffer=True). But this is not problem because Dropout is
  // only used when training with backward(clear_buffer=True).
  this->mask_->data()->array()->clear();
}

template <class T>
void Dropout<T>::backward_impl(const Variables &inputs,
                               const Variables &outputs,
                               const vector<bool> &propagate_down,
                               const vector<bool> &accum) {
  if (!propagate_down[0]) {
    return;
  }
  T *dx = inputs[0]->cast_grad_and_get_pointer<T>(this->ctx_, !accum[0]);
  const T *dy = outputs[0]->get_grad_pointer<T>(this->ctx_);
  const T *m = this->mask_->template get_data_pointer<T>(this->ctx_);
  for (int s = 0; s < inputs[0]->size(); ++s) {
    dx[s] = (accum[0] ? dx[s] : (T)0) + dy[s] * m[s] * scale_;
  }

  clear_buffer();
}
}
