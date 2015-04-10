/*
 * StepFilter.cpp
 *
 *  Created on: Mar 11, 2015
 *      Author: Martin Wermelinger
 *   Institute: ETH Zurich, Autonomous Systems Lab
 */

#include "filters/StepFilter.hpp"
#include <pluginlib/class_list_macros.h>
#include <algorithm>

// Grid Map
#include <grid_map/grid_map.hpp>

using namespace grid_map;

namespace filters {

template<typename T>
StepFilter<T>::StepFilter()
    : criticalValue_(0.3),
      firstWindowRadius_(0.08),
      secondWindowRadius_(0.08),
      nCellCritical_(5),
      type_("traversability_step")
{

}

template<typename T>
StepFilter<T>::~StepFilter()
{

}

template<typename T>
bool StepFilter<T>::configure()
{
  if (!FilterBase<T>::getParam(std::string("critical_value"), criticalValue_)) {
    ROS_ERROR("Step filter did not find param critical_value.");
    return false;
  }

  if (criticalValue_ < 0.0) {
    ROS_ERROR("Critical step height must be greater than zero.");
    return false;
  }

  ROS_INFO("Critical step height = %f.", criticalValue_);

  if (!FilterBase<T>::getParam(std::string("first_window_radius"), firstWindowRadius_)) {
    ROS_ERROR("Step filter did not find param 'first_window_radius'.");
    return false;
  }

  if (firstWindowRadius_ < 0.0) {
    ROS_ERROR("'first_window_radius' must be greater than zero.");
    return false;
  }

  ROS_INFO("First window radius of step filter = %f.", firstWindowRadius_);

  if (!FilterBase<T>::getParam(std::string("second_window_radius"), secondWindowRadius_)) {
    ROS_ERROR("Step filter did not find param 'second_window_radius'.");
    return false;
  }

  if (secondWindowRadius_ < 0.0) {
    ROS_ERROR("'second_window_radius' must be greater than zero.");
    return false;
  }

  ROS_INFO("Second window radius of step filter = %f.", secondWindowRadius_);

  if (!FilterBase<T>::getParam(std::string("critical_cell_number"), nCellCritical_)) {
    ROS_ERROR("Step filter did not find param 'critical_cell_number'.");
    return false;
  }

  if (nCellCritical_ <= 0) {
    ROS_ERROR("'critical_cell_number' must be greater than zero.");
    return false;
  }

  ROS_INFO("Number of critical cells of step filter = %d.", nCellCritical_);

  if (!FilterBase<T>::getParam(std::string("map_type"), type_)) {
    ROS_ERROR("Step filter did not find param map_type.");
    return false;
  }

  ROS_INFO("Step map type = %s.", type_.c_str());

  return true;
}

template<typename T>
bool StepFilter<T>::update(const T& mapIn, T& mapOut)
{
  // Add new layer to the elevation map.
  mapOut = mapIn;
  mapOut.add(type_);
  mapOut.add("step_height");

  double height, step;

  // First iteration through the elevation map.
  for (GridMapIterator iterator(mapOut); !iterator.isPassedEnd(); ++iterator) {
    if (!mapOut.isValid(*iterator, "elevation"))
      continue;
    height = mapOut.at("elevation", *iterator);
    double stepMax = 0.0;

    // Requested position (center) of circle in map.
    Eigen::Vector2d center;
    mapOut.getPosition(*iterator, center);

    int nCells = 0;
    // Get the highest step in the circular window.
    for (CircleIterator submapIterator(mapOut, center, firstWindowRadius_);
        !submapIterator.isPassedEnd(); ++submapIterator) {
      nCells++;
      if (!mapOut.isValid(*submapIterator, "elevation"))
        continue;
      step = std::abs(height - mapOut.at("elevation", *submapIterator));
      if (step > stepMax)
        stepMax = step;
    }
//    ROS_INFO("Cells in circle = %d", nCells);

    if (stepMax > 0.0)
      mapOut.at("step_height", *iterator) = stepMax;
  }

  // Second iteration through the elevation map.
  for (GridMapIterator iterator(mapOut); !iterator.isPassedEnd(); ++iterator) {
    int nCells = 0;
    double stepMax = 0.0;
    bool isValid = false;

    // Requested position (center) of circle in map.
    Eigen::Vector2d center;
    mapOut.getPosition(*iterator, center);

    // Compute the step height.
    for (CircleIterator submapIterator(mapOut, center, secondWindowRadius_);
        !submapIterator.isPassedEnd(); ++submapIterator) {
      if (!mapOut.isValid(*submapIterator, "step_height"))
        continue;
      isValid = true;
      if (mapOut.at("step_height", *submapIterator) > stepMax) {
        stepMax = mapOut.at("step_height", *submapIterator);
        if (stepMax > criticalValue_) {
          ROS_INFO_STREAM("Step max = " << stepMax);
          nCells++;
        }
      }
    }
    ROS_INFO_STREAM(nCells);

    if (isValid) {
      step = std::min(stepMax, nCells / nCellCritical_ * stepMax);
      ROS_INFO_STREAM("Step = " << step);
      if (step < criticalValue_) {
        mapOut.at(type_, *iterator) = 1.0 - step / criticalValue_;
      } else {
        mapOut.at(type_, *iterator) = 0.0;
      }
    }
  }
  mapOut.erase("step_height");
  return true;
}

} /* namespace */

PLUGINLIB_REGISTER_CLASS(StepFilter, filters::StepFilter<grid_map::GridMap>, filters::FilterBase<grid_map::GridMap>)
