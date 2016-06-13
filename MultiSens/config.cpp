/* Last updated DDK 6/7/16
 * 
 * OVERVIEW:
 * This file contains instantiations of the device objects to be used on the current 
 * session of the ArduFSM protocol MultiSens. These objects manage the behavior
 * of devices like stepper motors, speakers, etc. For full documentation of 
 * these device classes, see the README.md for the devices library. 
 * 
 * This file also places the device objects in an array and returns the array
 * to States.cpp, precluding the need to adjust hardware parameters in States.cpp
 * from experiment to experiment.
 */

#include "config.h"

Device ** config_hw(){ 
  static dummyStepper dmStpr1;
  static dummySpeaker dmSpkr1;
  
  static Device * devPtrs[] = { &dmStpr1, &dmSpkr1 };
  return devPtrs;
}

