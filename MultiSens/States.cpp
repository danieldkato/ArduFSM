/* Last updated DDK 6/7/16
 *  
 * OVERVIEW: 
 * This file defines functions that constitute most of the state-dependent 
 * operations for the ArduFSM protocol MultiSens. This includes much of the 
 * protocol's state-transition logic. Trial and response parameters are also 
 * stored here.
 * 
 * 
 * REQUIREMENTS:
 * This sketch must be located in the MultiSens protocol directory within
 * a copy of the ArduFSM repository on the local computer's Arduino sketchbook
 * folder. In addition to this file, the MultiSens directory should contain
 * the following files:
 * 
 * 1. config.h
 * 2. config.cpp
 * 3. States.h
 * 4. MultiSens.ino
 * 
 * In addition, the local computer's Arduino sketchbook library must contain 
 * the following libraries:
 *  
 * 1. chat, available at https://github.com/cxrodgers/ArduFSM/tree/master/libraries/chat
 * 2. TimedState, available at https://github.com/cxrodgers/ArduFSM/tree/master/libraries/TimedState
 * 3. devices, available at https://github.com/danieldkato/devices
 * 
 * 
 * DESCRIPTION:
 * This file primarily defines a number of functions that are called from
 * the state-dependent `switch case` statement in the `loop` function of the 
 * main MultiSens.ino sketch. For any state that consist entirely of some short-
 * duration (1-100ms) action that can be completed on a single pass of loop,
 * States.cpp defines a single, non-class function. For states that must persist  
 * for an extended period of time over multiple passes of `loop`, States.cpp 
 * defines classes and class functions that inherit from TimedState. These
 * TimedState objects store the time the corresponding state began as well as
 * the current time, and execute appropriate class functions based on how
 * much time has elapsed since the beginning of the state.
 * 
 * In addition to defining the TimedState sub-classes, this file also actually
 * instantiates them and returns them to the main sketch in an array. 
 * 
 * This file also defines arrays that store trial and response parameters for 
 * the current trial.
 * 
 * This code is agnostic with respect to what actual hardware devices (steppers,
 * speakers, etc.) are controlled by the Arduino on the current experiment. 
 * Rather, the code for the STIM_PERIOD object simply iterates through an 
 * array of device objects returned from config.cpp, and for each device object, 
 * calls some class function that selects and executes an appropriate action 
 * based on some current trial parameter and the current time. 
 */


/* Implementation file for declaring protocol-specific states.
This implements a two-alternative choice task with two lick ports.

Defines the following:
* param_abbrevs, which defines the shorthand for the trial parameters
* param_values, which define the defaults for those parameters
* results_abbrevs, results_values, default_results_values
* implements the state functions and state objects

*/

#include "devices.h"
#include "config.h"
#include "States.h"
#include "Arduino.h"
#include "Stepper.h"
// include this one just to get __TRIAL_SPEAK_YES
#include "chat.h"

int lickThresh = 900;
int Device::deviceCounter = 0;
Device ** devPtrs = config_hw();
int devIndices[NUM_DEVICES] = { tpidx_STPRIDX, tpidx_SPKRIDX };

extern STATE_TYPE next_state;

// These should go into some kind of Protocol.h or something
char* param_abbrevs[N_TRIAL_PARAMS] = {
  "STPRIDX", "SPKRIDX", "STIMDUR", "REW", "REW_DUR", 
  "IRI", "TO", "ITI", "RWIN", "MRT", 
  "TOE" 
  };
long param_values[N_TRIAL_PARAMS] = {
  0, 0, 2000, 0, 50, 
  500, 6000, 3000, 45000, 1,
  1   
  };

// Whether to report on each trial  
// Currently, manually match this up with Python-side
// Later, maybe make this settable by Python, and default to all True
// Similarly, find out which are required on each trial, and error if they're
// not set. Currently all that are required_ET are also reported_ET.
bool param_report_ET[N_TRIAL_PARAMS] = {
  1, 1, 1, 1, 0, 
  0, 0, 0, 0, 0,
  0
};
  
char* results_abbrevs[N_TRIAL_RESULTS] = {"RESP", "OUTC"};
long results_values[N_TRIAL_RESULTS] = {0, 0};
long default_results_values[N_TRIAL_RESULTS] = {0, 0};

// Global, persistent variable to remember where the stepper is
long sticky_stepper_position = 0;

//define function for isntantiating states (to be called from main sketch)
TimedState ** getStates(){
  
  static StimPeriod stim_period(param_values[tpidx_STIM_DUR]);
  static StateResponseWindow srw(param_values[tpidx_RESP_WIN_DUR]);
  static StateFakeResponseWindow sfrw(param_values[tpidx_RESP_WIN_DUR]);
  static StateInterTrialInterval state_inter_trial_interval(
    param_values[tpidx_ITI]);
  static StateErrorTimeout state_error_timeout(
    param_values[tpidx_ERROR_TIMEOUT]);
  static StatePostRewardPause state_post_reward_pause(
        param_values[tpidx_INTER_REWARD_INTERVAL]);

  static TimedState * states[] = { &stim_period, &srw, &sfrw, &state_inter_trial_interval, &state_error_timeout, &state_post_reward_pause};
  return states;
}

////Utility functions
boolean checkLicks(){
  boolean licking;
  int aIn = analogRead(LICK_DETECTOR_PIN);
    if ( aIn > lickThresh ){
      licking = 1;
    }
    else {
      licking = 0;
    }
  return licking;
}

//// State definitions
extern Stepper* stimStepper;


////StimPeriod
void StimPeriod::s_setup(){

  duration = param_values[tpidx_REW_DUR];  
  licked = 0;
  
  for ( int i = 0; i < NUM_DEVICES; i++ ){
    devFcns[i] = param_values[devIndices[i]]; 
  }
}

void StimPeriod::loop(){

  unsigned long time = millis();
  
  for ( int i = 0; i < NUM_DEVICES; i++ ){
    devPtrs[i] -> loop(devFcns[i]);
  }

  //on rewarded trials, make reward coterminous with stimulus
  if ( param_values[tpidx_REW] == 1 && (timer - time) < param_values[tpidx_REW_DUR] ){
    digitalWrite(SOLENOID_PIN, HIGH);
  }
}

void StimPeriod::s_finish()
{
  for ( int i = 0; i < NUM_DEVICES; i++ ){
    devPtrs[i] -> s_finish();
  }

  digitalWrite(SOLENOID_PIN, LOW);

  //if the mouse licked during the stimulus period, transition to timeout
  if ( licked == 1 ){ 
    next_state = ERROR; 
  }
  //if not, transition to response window
  else {
    next_state = RESPONSE_WINDOW;
  }
}

//// StateResponseWindow
void StateResponseWindow::update()
{
 my_licking = checkLicks();
}

void StateResponseWindow::s_setup(){
  duration = param_values[tpidx_RESP_WIN_DUR];
}

void StateResponseWindow::loop()
{
  int current_response;
  bool licking;
  
  // get the licking state 
  // overridden in FakeResponseWindow
  set_licking_variables(licking);
  
  // transition if max rewards reached
  if (my_rewards_this_trial >= param_values[tpidx_MRT])
  {
    next_state = INTER_TRIAL_INTERVAL;
    flag_stop = 1;
    return;
  }

  // Do nothing if both or neither are being licked.
  // Otherwise, assign current_response.
  if (!licking)
    return;
  else if (licking)
    current_response = GO;
  else
    Serial.println("ERR this should never happen");

  // Only assign result if this is the first response
  if (results_values[tridx_RESPONSE] == 0)
    results_values[tridx_RESPONSE] = current_response;
  
  // Move to reward state, or error if TOE is set, or otherwise stay
  if ((current_response == GO) && (param_values[tpidx_REW] == GO))
  { // Hit
    next_state = REWARD;
    my_rewards_this_trial++;
    results_values[tridx_OUTCOME] = OUTCOME_HIT;
  }
  else if (param_values[tpidx_TERMINATE_ON_ERR] == __TRIAL_SPEAK_NO)
  { // Error made, TOE is false
    // Decide how to deal with this non-TOE case
  }
  else
  { // Error made, TOE is true
    results_values[tridx_OUTCOME] = OUTCOME_FA;
    next_state = ERROR;
  }
}

void StateResponseWindow::s_finish()
{
  // If response is still not set, mark as a nogo response
  if (results_values[tridx_RESPONSE] == 0)
  {
    // The response was nogo
    results_values[tridx_RESPONSE] = NOGO;
    
    // Outcome depends on what he was supposed to do
    if (param_values[tpidx_REW] == NOGO) {
      // Correctly did nothing on a NOGO trial
      results_values[tridx_OUTCOME] = OUTCOME_CR;
    } else {
      results_values[tridx_OUTCOME] = OUTCOME_MISS;
    }

  // In any case the trial is over
  next_state = INTER_TRIAL_INTERVAL;
  }
}

void StateResponseWindow::set_licking_variables(bool &licking)
{ /* Gets the current licking status from the touched variable for each port */
  
  int aIn = analogRead(LICK_DETECTOR_PIN);
  if ( aIn > lickThresh ){
    licking = 1;
  }
  else {
    licking = 0;
  }
}


//// StateFakeResponsewindow
// Differs only in that it randomly fakes a response
void StateFakeResponseWindow::set_licking_variables(bool &licking)
{ /* Fakes a response by randomly choosing lick status for each */
  licking = (random(0, 10000) < 3);       
}


//// Inter-trial interval
void StateInterTrialInterval::s_setup()
{
  duration = param_values[tpidx_ITI];
  // First-time code: Report results
  for(int i=0; i < N_TRIAL_RESULTS; i++)
  {
    Serial.print(time_of_last_call);
    Serial.print(" TRLR ");
    Serial.print(results_abbrevs[i]);
    Serial.print(" ");
    Serial.println(results_values[i]);
  }
}

void StateInterTrialInterval::s_finish()
{
  next_state = WAIT_TO_START_TRIAL;   
}

//// Post-reward state
void StatePostRewardPause::s_finish()
{
  next_state = RESPONSE_WINDOW;
}

//// StateErrorTimeout
void StateErrorTimeout::s_finish()
{
  next_state = INTER_TRIAL_INTERVAL;
}

void StateErrorTimeout::s_setup(){
  duration = param_values[tpidx_ERROR_TIMEOUT];  
}


//// Non-class states
// The reward states use delay because they need to be millisecond-precise
int state_reward(STATE_TYPE& next_state)
{
  digitalWrite(SOLENOID_PIN, HIGH);
  delay(param_values[tpidx_REW_DUR]);
  digitalWrite(SOLENOID_PIN, LOW); 
  next_state = POST_REWARD_PAUSE;
  return 0;  
}
