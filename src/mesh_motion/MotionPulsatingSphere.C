
#include "mesh_motion/MotionPulsatingSphere.h"

#include <NaluParsing.h>

#include <cmath>

namespace sierra{
namespace nalu{

MotionPulsatingSphere::MotionPulsatingSphere(const YAML::Node& node)
  : MotionBase()
{
  load(node);
}

void MotionPulsatingSphere::load(const YAML::Node& node)
{
  get_if_present(node, "start_time", startTime_, startTime_);

  get_if_present(node, "end_time", endTime_, endTime_);

  get_if_present(node, "amplitude", amplitude_, amplitude_);

  get_if_present(node, "frequency", frequency_, frequency_);

  origin_ = node["origin"].as<ThreeDVecType>();
  assert(origin_.size() == threeDVecSize);
}

void MotionPulsatingSphere::build_transformation(
  const double time,
  const double* xyz)
{
  double eps = std::numeric_limits<double>::epsilon();

  if( (time >= (startTime_-eps)) && (time <= (endTime_+eps)) )
    scaling_mat(time,xyz);
}

void MotionPulsatingSphere::scaling_mat(
  const double time,
  const double* xyz)
{
  reset_mat(transMat_);

  double radius = std::sqrt( std::pow(xyz[0]-origin_[0],2)
                            +std::pow(xyz[1]-origin_[1],2)
                            +std::pow(xyz[2]-origin_[2],2));

  double curr_radius = radius + amplitude_*(1 - std::cos(2*M_PI*frequency_*time));

  double uniform_scaling = curr_radius/radius;

  // Build matrix for translating object to cartesian origin
  transMat_[0][3] = -origin_[0];
  transMat_[1][3] = -origin_[1];
  transMat_[2][3] = -origin_[2];

  // Build matrix for scaling object
  TransMatType currTransMat = {};

  currTransMat[0][0] = uniform_scaling;
  currTransMat[1][1] = uniform_scaling;
  currTransMat[2][2] = uniform_scaling;
  currTransMat[3][3] = 1.0;

  // composite addition of motions in current group
  transMat_ = add_motion(currTransMat,transMat_);

  // Build matrix for translating object back to its origin
  reset_mat(currTransMat);
  currTransMat[0][3] = origin_[0];
  currTransMat[1][3] = origin_[1];
  currTransMat[2][3] = origin_[2];

  // composite addition of motions
  transMat_ = add_motion(currTransMat,transMat_);
}

MotionBase::ThreeDVecType MotionPulsatingSphere::compute_velocity(
  double time,
  const TransMatType&  /* compTrans */,
  double* xyz )
{
  ThreeDVecType vel = {};

  double eps = std::numeric_limits<double>::epsilon();

  if( (time >= (startTime_-eps)) && (time <= (endTime_+eps)) )
  {
    double radius = std::sqrt( std::pow(xyz[0]-origin_[0],2)
                              +std::pow(xyz[1]-origin_[1],2)
                              +std::pow(xyz[2]-origin_[2],2));

    double pulsatingVelocity =
      amplitude_ * std::sin(2*M_PI*frequency_*time) * 2*M_PI*frequency_ / radius;

    for (int d=0; d < threeDVecSize; d++)
    {
      int signum = (-eps < xyz[d]-origin_[d]) - (xyz[d]-origin_[d] < eps);
      vel[d] = signum * pulsatingVelocity * (xyz[d]-origin_[d]);
    }
  }

  return vel;
}

} // nalu
} // sierra
