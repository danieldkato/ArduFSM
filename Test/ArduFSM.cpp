/* Standard definitions of setup() and (TODO) loop() for all ArduFSM protocols.

Protocol-specific code goes into certain specified functions:
user_setup1()
user_setup2()
(TODO) a dispatch table for state machine
*/

#include "ArduFSM.h"
#include "Arduino.h"
#include "chat.h"
#include "TimedState.h"
#include "States.h"

#include "Params.h"
#include "TrialSpeak.h"


// These functions are defined in the protocol-specific *.ino file
extern void user_setup1();
extern void user_setup2();
extern void user_trial_start();
extern bool flag_start_trial;

// Params
// params and results
extern char* param_abbrevs[N_TRIAL_PARAMS];
extern long param_values[N_TRIAL_PARAMS];
extern bool param_report_ET[N_TRIAL_PARAMS];
extern char* results_abbrevs[N_TRIAL_RESULTS];
extern long results_values[N_TRIAL_RESULTS];
extern long default_results_values[N_TRIAL_RESULTS];

//// State variables
// We use this pointer to monitor the current state
State* current_state;

// These are standard states which will be externed into the ino
StateTrialStart state_trial_start;
StateWaitToStartTrial state_wait_to_start_trial;

//// Setup function
void setup()
{ /* Standard setup function to initialize the arduino.
  
  1. Initializes the serial port and announces time and (TODO) version info
  2. Runs a protocol-specific user_setup1() function that sets things like
     inputs/outpus before receiving any serial communcation.
  3. Runs serial communication until enough info has been received to run
     the first trial.
  4. Runs a protocol-specific user_setup2() function that finalizes anything
     that depens on receiving input, like 2-pin vs 4-pin mode.
  */
  unsigned long time = millis();
  int status = 1;
  
  // Initalize serial port communication and announce time
  Serial.begin(115200);
  Serial.print(time);
  Serial.println(" DBG begin setup");

  // Protocol-specific setup1, to be run before receiving any serial data
  user_setup1();

  // Run communications until we've received enough startup info to
  // start the first trial.
  while (!flag_start_trial)
  {
    status = communications(time);
    if (status != 0)
    {
      Serial.println("comm error in setup");
      delay(1000);
    }
  }
  
  // Now finalize the setup using the received initial parameters
  user_setup2();
}

// Standard function for announcing state changes
void announce_state_change(unsigned long time, State *current_state,
  State *next_state)
{
  Serial.print(time);
  Serial.print(" ST_CHG ");
  Serial.print(current_state->id);
  Serial.print(" ");
  Serial.println(next_state->id);

  Serial.print(millis());
  Serial.print(" ST_CHG2 ");
  Serial.print(current_state->id);
  Serial.print(" ");
  Serial.println(next_state->id);
}


//// Standard state definitions
// Define the wait to start trial state
State* StateWaitToStartTrial::run(unsigned long time)
{
  // Wait until we receive permission to continue  
  if (flag_start_trial)
  {
    // Announce that we have ended the trial and reset the flag
    Serial.print(time);
    Serial.println(" TRL_RELEASED");
    flag_start_trial = 0;
    
    // Proceed to next trial
    return &state_trial_start;
  }
}

// Define the trial start state
State* StateTrialStart::run(unsigned long time) {
  // Set up the trial based on received trial parameters
  Serial.print(time);
  Serial.println(" TRL_START");
  for(int i=0; i < N_TRIAL_PARAMS; i++)
  {
    if (param_report_ET[i]) 
    {
      // Buffered write would be nice here
      Serial.print(time);
      Serial.print(" TRLP ");
      Serial.print(param_abbrevs[i]);
      Serial.print(" ");
      Serial.println(param_values[i]);
    }
  }

  // Set up trial_results to defaults
  for(int i=0; i < N_TRIAL_RESULTS; i++)
  {
    results_values[i] = default_results_values[i];
  }      
  
  // Run user-specific trial start code, including setting next state
  user_trial_start();
}