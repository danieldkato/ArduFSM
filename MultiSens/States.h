/* Last updated DDK 6/7/16
 * 
 * OVERVIEW: 
 * This is a header file containing declarations for the functions defined in the 
 * States.cpp file of the ArduFSM protocol MultiSens. These functions constitute 
 * most of the state-dependent operations for the ArduFSM protocol MultiSens. This 
 * includes much of the protocol's state-transition logic.
 * 
 * This is also where the `STATE_TYPE` variable and its values (i.e. the protocol's
 * states) are declared. 
 * 
 * This file also defines a number of macros for indexing into the trial parameter
 * and trial results arrays defined in States.cpp, thereby precluding the need
 * to memorize numeric indices for each trial and results parameter.
 * 
 * 
 * REQUIREMENTS:
 * This sketch must be located in the MultiSens protocol directory within
 * a copy of the ArduFSM repository on the local computer's Arduino sketchbook
 * folder. In addition to this file, the MultiSens directory should contain
 * the following files:
 * 
 * 1. States.cpp
 * 2. MultiSens.ino
 * 
 * In addition, the local computer's Arduino sketchbook library must contain 
 * the following libraries:
 *  
 * 1. chat, available at https://github.com/cxrodgers/ArduFSM/tree/master/libraries/chat
 * 2. TimedState, available at https://github.com/cxrodgers/ArduFSM/tree/master/libraries/TimedState
 * 3. devices, available at https://github.com/danieldkato/devices
 */


/* Header file for declaring protocol-specific states.
This implements a two-alternative choice task with two lick ports.

Defines the following
* tpidx_ macros ... these allow you to index into trial_params
* tridx_ macros ... same, but for trial results
* the STATE_TYPE enum, which defines all possible states
* declarations for state functions and state objects, especially those derived
  from TimedState.

To define a new state, you should declare it here and put its implementation
into States.cpp.
*/


#ifndef __STATES_H_INCLUDED__
#define __STATES_H_INCLUDED__

#include "TimedState.h"
#include "devices.h"

//// Global trial parameters structure. This holds the current value of
// all parameters. Should probably also make a copy to hold the latched
// values on each trial.
// Characteristics of each trial. These can all be modified by the user.
// However, there is no guarantee that the newest value will be used until
// the current trial is released.
// Various types:
// * Should be latched, must be set at beginning ("RD_L")
// * Should be latched, can use default here ("SRVFAR")
// * Need to be set on every trial, else error ("STPPOS")
// * Some, like STPSPD, are not being updated
//
// * Init-only: those, like STPSPD, that are only used at the very beginning
// * Init-usually: those, like SRV_FAR, that could be varied later, but
//   the use case is infrequent (or never) and possibly untested.
// * Trial-required: those, like rewside, that must be specified on each trial 
//   or error.
// * Latched: those, like TO, that will often vary within a session
//
// Reported on each trial: trial-required, maybe latched
// Reported initially only: init-only, probably init-usually
// Reported on change only: latched, maybe init-usually
//
// Decision 1: Report latched on each trial or only on change?
// Decision 2: Should init-usually be treated more like latched (see above),
//             or more like init-only?
//
// The main things to add to this code are:
// * Report TRLP only those that are marked "report-on-each-trial"
// * Do not release trial until all "required-on-each-trial" are set
// And we can argue about how these two categories map onto the categories above.
//
// Attempt to have 0 be the "error value" since it cannot intentially be set to 0.
#define N_TRIAL_PARAMS 13
#define tpidx_STPRIDX 0 // reqd
#define tpidx_SPKRIDX 1 // latch
#define tpidx_STIM_DUR 2 // reqd
#define tpidx_REW 3 //reqd
#define tpidx_REW_DUR 4 // init-usually
#define tpidx_INTER_REWARD_INTERVAL 5
#define tpidx_ERROR_TIMEOUT 6
#define tpidx_ITI 7
#define tpidx_RESP_WIN_DUR 8
#define tpidx_MRT 9
#define tpidx_TERMINATE_ON_ERR 10 
#define tpidx_INTERSTIM_LATENCY 11
#define tpidx_VOLUME 12

//state indices for an array of state pointers
#define N_CLASS_STATES 6
#define stidx_STIM_PERIOD 0
#define stidx_RESPONSE_WINDOW 1 
#define stidx_FAKE_RESPONSE_WINDOW 2
#define stidx_INTER_TRIAL_INTERVAL 3
#define stidx_ERROR 4
#define stidx_POST_REWARD_PAUSE 5 

//// Global trial results structure. Can be set by user-defined states. 
// Will be reported during mandatory INTER_TRIAL_INTERVAL state.
#define N_TRIAL_RESULTS 2
#define tridx_RESPONSE 0
#define tridx_OUTCOME 1

//// Defines for commonly used things
// Move this to TrialSpeak, and rename CHOICE_LEFT etc
#define GO 1
#define NOGO 2

#define OUTCOME_HIT 1
#define OUTCOME_FA 2
#define OUTCOME_MISS 3
#define OUTCOME_CR 4 

//MultiSens-specific macros:
#include "devices.h"                    
#define NUM_DEVICES 2

// Digital I/O:
// Pins 0 and 1 reserved for serial communication
#define SPKR_COND_PIN1 2
#define VOLUME_PIN 3
#define SOLENOID_PIN 4
#define LED_PIN 5 // reserved for H-bridge
#define STPR_PIN 6 // reserved for stepper motor driver STEP pin
#define ENBL_PIN 7 // reserved for stepper motor driver ENABLE pin
#define DIR_PIN 8 // reserved for stepper motor driver DIR pin
#define SLP_PIN 9 // reserved for stepper motor driver SLEEP pin
#define SPKR_COND_PIN2 10
//#define STPR1_PIN2 6      
#define TIMER_PIN 11
#define SPKR_PIN 13

// Analog inputs:
#define HALL_PIN A0 // reserved for hall effect sensor
#define HALL_PIN2 A1
#define LICK_DETECTOR_PIN A1 //

#define NUM_STEPS 200
#define HALL_THRESH 1000
#define STPR1_SPEED 100
#define STPR1_CW 50
#define STPR1_CCW -50
#define HALL_VAL 500  
#define STEP_HALFDELAY_US 1100
#define MICROSTEP 16
#define REVERSE_ROTATION_DEGREES 50

//// States
// Defines the finite state machine for this protocol
enum STATE_TYPE
{
  WAIT_TO_START_TRIAL,
  TRIAL_START,
  STIM_PERIOD,
  REWARD,
  RESPONSE_WINDOW,
  ERROR,
  INTER_TRIAL_INTERVAL,
  POST_REWARD_PAUSE,
};

//Declare function for handling state-dependent operations:
void stateDependentOperations(STATE_TYPE current_state, unsigned long time);

// Declare utility functions
boolean checkLicks();

// Declare non-class states
int state_reward(STATE_TYPE& next_state);
  

//// Declare states that are objects
// Presently these all derive from TimedState
// Ensure that they have a public constructor, and optionally, a public
// "update" function that allows resetting of their parameters.
//
// Under protected, declare their protected variables, and declare any of
// s_setup, loop(), and s_finish() that you are going to define.
class StimPeriod : public TimedState {
  protected:
    int _timerPin;
    void s_setup();
    void loop();
    void s_finish();
    boolean licked;

  public: 
    int trialNumber;
    int devFcns[NUM_DEVICES];
    StimPeriod(unsigned long d, int timerPin) : TimedState(d), _timerPin(timerPin), trialNumber(1){ pinMode(_timerPin, OUTPUT);}
};

class StateResponseWindow : public TimedState {
  protected:
    boolean my_licking = 0;
    unsigned int my_rewards_this_trial = 0;
    void s_setup();
    void loop();
    void s_finish();
    virtual void set_licking_variables(bool &);
  
  public:
    void update();
    StateResponseWindow(unsigned long d) : TimedState(d) { };
};

class StateFakeResponseWindow : public StateResponseWindow {
  /* A version of StateResponseWindow that randomly makes decisions */
  protected:
    void set_licking_variables(bool &);
  
  public:
    StateFakeResponseWindow(unsigned long d) : StateResponseWindow(d) { };
};

class StateErrorTimeout : public TimedState {
  protected:
  
    void s_setup();
    void s_finish();
  
  public:
    StateErrorTimeout(unsigned long d) : TimedState(d) { };
};

class StateInterTrialInterval : public TimedState {
  protected:
    void s_setup();
    void s_finish();
  
  public:
    StateInterTrialInterval(unsigned long d) : TimedState(d) { };
};

class StatePostRewardPause : public TimedState {
  protected:
    void s_finish();
  
  public:
    StatePostRewardPause(unsigned long d) : TimedState(d) { };
};

Device ** config_hw();

void rotate_to_sensor();
void rotate_one_step();
void rotate_back();
void trigger_stepper();
void trigger_audio();
void signal_trial_start();
void rotate_steps(int n);

#endif
