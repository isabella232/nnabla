// Copyright 2017,2018,2019,2020,2021 Sony Corporation.
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

/** Pow Scalar
 */
#ifndef __NBLA_FUNCTION_POW_SCALAR_HPP__
#define __NBLA_FUNCTION_POW_SCALAR_HPP__

#include <nbla/function/utils/base_transform_unary.hpp>

#include <cmath>

namespace nbla {

/** @class PowScalar
@brief Elementwise pow scalar defined as
@f[
y_i = x_i ^ v.
@f]

Inputs:
- N-D array.

Outputs:
- N-D array.

@tparam T Data type for computation.
@param val Value of the scalar.
\ingroup FunctionImplGrp
 */
// Inplacing is obsoleted.
NBLA_DEFINE_TRANSFORM_UNARY_1_INPLACE(PowScalar,
                                      a0 == 0.5f ? std::sqrt(x)
                                                 : std::pow(x, (T)a0),
                                      dy *(T)a0 *std::pow(x, (T)a0 - (T)1),
                                      false, true, double, true);
}
#endif
