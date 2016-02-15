/** @file
    @brief Header

    @date 2016

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2016 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_EigenQuatExponentialMap_h_GUID_7E15BC44_BCFB_438B_902B_BA0787BEE405
#define INCLUDED_EigenQuatExponentialMap_h_GUID_7E15BC44_BCFB_438B_902B_BA0787BEE405

// Internal Includes
#include <osvr/Util/EigenCoreGeometry.h>

// Library/third-party includes
// - none

// Standard includes
// - none

namespace osvr {
namespace util {
    namespace ei_quat_exp_map {

        template <typename Scalar> struct FourthRootMachineEps;
        template <> struct FourthRootMachineEps<double> {
            /// machine epsilon is 1e-53, so fourth root is roughly 1e-13
            static double get() { return 1.e-13; }
        };
        template <> struct FourthRootMachineEps<float> {
            /// machine epsilon is 1e-24, so fourth root is 1e-6
            static float get() { return 1.e-6f; }
        };
        /// Computes sinc(Theta) (roughly sine(theta)/theta except defined at
        /// theta = 0)
        template <typename Scalar> inline Scalar sinc(Scalar theta) {
            /// fourth root of machine epsilon is recommended cutoff for taylor
            /// series expansion vs. direct computation per
            /// Grassia, F. S. (1998). Practical Parameterization of Rotations
            /// Using the Exponential Map. Journal of Graphics Tools, 3(3),
            /// 29–48. http://doi.org/10.1080/10867651.1998.10487493
            Scalar ret;
            if (theta < FourthRootMachineEps<Scalar>::get()) {
                // taylor series expansion.
                ret = Scalar(1.f) - theta * theta / Scalar(6.f);
                return ret;
            }
            // direct computation.
            ret = std::sin(theta) / theta;
            return ret;
        }

        /// fully-templated free function for quaternion expontiation, intended
        /// for implementation use within the class.
        template <typename Scalar, typename InputVec>
        inline Eigen::Quaternion<Scalar> quat_exp(InputVec &&vec) {
            Scalar theta = vec.norm();
            Scalar vecscale = sinc(theta / 2) / 2;
            Eigen::Quaternion<Scalar> ret;
            ret.vec() = vecscale * vec;
            ret.w() = std::cos(theta / 2);
            return ret.normalized();
        }
        /// fully-templated free function for quaternion log map, intended for
        /// implementation use within the class.
        template <typename Scalar>
        inline Eigen::Matrix<Scalar, 3, 1>
        quat_ln(Eigen::Quaternion<Scalar> const &quat) {
            Scalar vecnorm = quat.vec().norm();
            Scalar phi = std::atan2(vecnorm, quat.w());
            Scalar phiOverNorm =
                vecnorm < 1e-4 ? (phi / std::sin(phi)) : phi / vecnorm;
            return quat.vec() * phiOverNorm;
        }

        template <typename Derived> struct ScalarTrait;

        template <typename Derived>
        using ScalarType = typename ScalarTrait<Derived>::type;

        template <typename Derived>
        using QuatType = Eigen::Quaternion<ScalarType<Derived>>;

        /// CRTP base for quaternion exponential map
        template <typename Derived_> class QuatExpMapBase {
          public:
            using Derived = Derived_;

            QuatType<Derived> exp() const {
                return quat_exp<ScalarType<Derived>>(derived().vec());
            }

            Derived const &derived() const {
                return *static_cast<Derived const *>(this);
            }
        };
        // forward declaration
        template <typename Scalar_> class VecWrapper;
        // traits declaration
        template <typename Scalar_> struct ScalarTrait<VecWrapper<Scalar_>> {
            using type = Scalar_;
        };

        template <typename Scalar_>
        class VecWrapper : public QuatExpMapBase<VecWrapper<Scalar_>> {
          public:
            using Scalar = Scalar_;
            using VecType = Eigen::Matrix<Scalar, 3, 1>;
            explicit VecWrapper(VecType const &vec) : m_vec(vec) {}

            VecType const &vec() const { return m_vec; }

          private:
            VecType const &m_vec;
        };

        // forward declaration
        template <typename Scalar_> class QuatWrapper;
        template <typename Scalar_> struct ScalarTrait<QuatWrapper<Scalar_>> {
            using type = Scalar_;
        };

        /// Wrapper class for a quaternion to provide access to ln() in its
        /// exponential-map meaning.
        template <typename Scalar_>
        class QuatWrapper : public QuatExpMapBase<QuatWrapper<Scalar_>> {
          public:
            using Scalar = Scalar_; // typename ScalarType<Derived>::type;
            using QuatType = Eigen::Quaternion<Scalar>;
            using LogVectorType = Eigen::Matrix<Scalar, 3, 1>;
            using QuatCoefficients = typename QuatType::Coefficients;
            using VecBlock = Eigen::VectorBlock<const QuatCoefficients, 3>;

            explicit QuatWrapper(QuatType const &quat) : m_quat(quat) {}

            /// Gets the log of the quat, in the exponential-map sense, as a 3d
            /// vector.
            LogVectorType ln() const {
                Scalar vecnorm = m_quat.vec().norm();
                Scalar phi = std::atan2(vecnorm, m_quat.w());
                Scalar phiOverNorm =
                    vecnorm < 1e-4 ? (phi / std::sin(phi)) : phi / vecnorm;
                return m_quat.vec() * phiOverNorm;
            }

            /// Access to just the vector part, in case we're actually trying to
            /// exponentiate here.
            VecBlock vec() const { return m_quat.vec(); }

          private:
            QuatType const &m_quat;
        };

        template <typename Scalar>
        inline QuatWrapper<Scalar>
        quat_exp_map(Eigen::Quaternion<Scalar> const &q) {
            return QuatWrapper<Scalar>(q);
        }

        template <typename Scalar>
        inline VecWrapper<Scalar>
            quat_exp_map(Eigen::Matrix<Scalar, 3, 1> const &v) {
            return VecWrapper<Scalar>(v);
        }
    } // namespace ei_quat_exp_map
    using ei_quat_exp_map::quat_exp_map;
} // namespace util
} // namespace osvr

#endif // INCLUDED_EigenQuatExponentialMap_h_GUID_7E15BC44_BCFB_438B_902B_BA0787BEE405
