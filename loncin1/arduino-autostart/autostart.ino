/* Gen autostart

  (L) Arno Teigseth 2024+

  SERVO STUFF stolen from by BARRAGAN <http://barraganstudio.com>
  https://www.arduino.cc/en/Tutorial/LibraryExamples/Sweep

  Electrical power generator set autostarter for generators WITH manual choke AND electrical starter motor. For example Loncin 13000W

  Features:  
  - Auto choke with on-boot calibration
  - Generator autostart / autostop, and connect / disconnect outputs
  - Warning buzzer before start, in case someone has hands, hair etc caught up in the unit. YES, it is a good idea to mount a Mushroom Emergency stop switch cutting all power.
  - Couple LEDs indicating MAINS and GEN power.
  - Diagnostic output to serial port. Connect a display, computer, printer… :)
  - Tries up to 3 times to start the generator - if no generated voltage is detected, it gives up until reset. This is to save the starter motor from out of fuel situations etc.

  Caveats
  - Will not retry starting after out-of-fuel situation (or other generator failure)

  Inputs:
  - Mains OK input
  - Generator generates power (successful start)

  Outputs:
  - Ignition on
  - Starter motor run
  - Connect load

  Program flow is event driven but uses a fixed state machine after an event is trigged:
  - BOOT:  
    - Set choke to middle position (90°)
    - Set choke to CLOSED position and read the input for 3 seconds, allowing adjustment. The value read at the end of this period is used for the remainder of system uptime.
    - Set choke to OPEN position and read the input for 3 seconds, allowing adjustment. The value read at the end of this period is used for the remainder of system uptime.
    
  - MAINS FAIL:
    - Wait 10 seconds while measuring the mains input to make sure it failed, and isn't just digital noise on the input.
  A - Beep for a while
    - Connect ignition
    - Open choke    
    - Run starter motor
    - Close choke after a second while starter motor on
    - Stop starter motor
    - Wait a while to see if generator is generating power
    - If there is no generator power, turn off ignition and wait for a while for the motor to stop ===NOTE NOT TESTED, see TODO comment below===, then go to A up to 3 times, or go stuck until a power cycle.
    - If the generator started successfully:
    - Wait during a warm-up time, then
    - Connect generator to power

  - MAINS RESTORED (and generator is running):
    - Disconnect load
    - Wait 10 seconds while measuring mains. If mains is flaky during this time, reconnect generator and abort shutdown.
    - If 10 seconds with mains OK has gone by,
    - Turn off ignition and go back to wait mode.
    
*/

// includes
#include <Servo.h>
Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

// variables
int genrunning = 0;
int genpowerok = 0;
int mainpowerok = 0;
int currentstartattempt = 0;
int state = 0;
int laststate = 0;

// constants
const String copyright = " - Arno Teigseth 2024";
const String programversion = " Generator autostarter/transfer v0.1";
const int beeptime = 7000; // beep time before running starter
const int chokemovementtime = 1000; // wait time before running starter
const int chokeextratime = 1000; // after starter released
// const int startermotortime = 3000; // starter runtime
const int startermotortime = 800; // starter runtime WITH choke
const int postchokestartertime = 500; // after choke close, keep it 1 little
const int restartattempttime = 8000; // time between starter attempts. 3000 is definitely too short
const int runstabilizetime = 500; // time from run has engaged until we can turn starter
const int genstabilizetime = 5000; // time from start until checking for voltage

// TODO need to check these
const int warmuptime = 10000; // time from voltage stable until loading
// const int cooldowntime = 30000; // time from zero gen load until gen shutoff
// const int cooldowntime = 5000; // time from zero gen load until gen shutoff
const int cooldownsecs = 10; // time from zero gen load until gen shutoff, in seconds

const int startattempts = 3; // max number of attempts before giving up
int chokeon = 90; // degrees to close choke, corrected by setup step below
int chokeoff = 180; // degrees to open choke, corrected by setup step below
const int state_mainsok = 1; //
const int state_mainsnok = 2; //
const int state_mainsgennok = 3; //
const int state_genok = 4; //
const int state_stuck = 5; //

// I/O
const int pin_closeadj = A1; // Analog1
const int pin_openadj = A2; // Analog2
const int pin_buzzer = 4; // DIGITAL4
const int pin_chokeservo = 2; // DIGITAL2
const int pin_runRelay = 11; // DIGITAL11
const int pin_starterRelay = 12; // DIGITAL12
const int pin_loadRelay = 8; // DIGITAL8
const int pin_mainpwr = 5; // DIGITAL5
const int pin_genpwr = 6; // DIGITAL6
const int pin_ledmain = 9; // DIGITAL9
const int pin_ledgen = 7; // DIGITAL7



// boot time
void testServo() {
  analogReference(DEFAULT);
  int del = 100;


  // cycle middle/close
  myservo.write(90); // middle pos
  int minread = 0;

  delay(1000);
  for (int x = 0; x < 30; x++) {
    minread = analogRead(pin_closeadj);
    chokeon = minread / 6; // 5.688 steps per degree
    choke(1);
    Serial.print("Choke CLOSE setting ");
    Serial.print(minread);
    Serial.print(" ");
    Serial.print(chokeon);
    Serial.println();

    delay(del);
  }

  // cycle middle/open
  for (int x = 0; x < 30; x++) {
    int maxread = analogRead(pin_openadj);
    chokeoff = maxread / 6; // 5.688 steps per degree
    choke(0);
    Serial.print("Choke OPEN setting ");
    Serial.print(maxread);
    Serial.print(" ");
    Serial.print(chokeoff);
    Serial.println();

    delay(del);
  }

}

// for boot time testing of stuff. pass relay pin#
void testRelay(int rel, int endstate = 1) {
  int del = 100;

  for (int x = 0; x < 3; x++) {
    digitalWrite(rel, 0);
    delay(del);
    digitalWrite(rel, 1);
    delay(del);
  }

  digitalWrite(rel, endstate);
}

void setup() {
  // myservo.attach(2);  // attaches the servo on pin DIGITAL2 to the servo object
  myservo.attach(pin_chokeservo);  // attaches the servo on pin DIGITAL2 to the servo object
  pinMode(pin_mainpwr, INPUT_PULLUP);
  pinMode(pin_genpwr, INPUT_PULLUP);
  // pinMode(pin_closeadj, INPUT);
  // pinMode(pin_openadj, INPUT);

  pinMode(pin_runRelay, OUTPUT);
  digitalWrite(pin_runRelay, 1);

  pinMode(pin_loadRelay, OUTPUT);
  digitalWrite(pin_loadRelay, 1);

  pinMode(pin_starterRelay, OUTPUT);
  digitalWrite(pin_starterRelay, 1);

  pinMode(pin_buzzer, OUTPUT);
  digitalWrite(pin_buzzer, 0); // active high

  pinMode(pin_ledmain, OUTPUT);
  digitalWrite(pin_ledmain, 0);

  pinMode(pin_ledgen, OUTPUT);
  digitalWrite(pin_ledgen, 0);

  // debug output
  Serial.begin(9600); // opens serial port, sets data rate to 9600 bps

  // should dump setup values
  Serial.print("BOOT ");
  Serial.print(programversion);
  Serial.println(copyright);

  // test leds
  Serial.println("Test MAINS led");
  testRelay(pin_ledmain, 0);
  Serial.println("Test GEN led");
  testRelay(pin_ledgen, 0);

  // setup servo, will read pots from A1/A2 for values
  testServo();

}


// helpers
void beep(int setting) {
  if (setting == 1) {
    digitalWrite(pin_buzzer, 1);
    // Serial.println("Beep");
  } else {
    digitalWrite(pin_buzzer, 0);
    // Serial.println("NoBeep");
  }
}

void choke(int setting) {
  // standard servo wires
  // red + center
  // brown -
  // orange signal

  // 12-19cm from

  if (setting == 1) {
    myservo.write(chokeon);
  } else {
    myservo.write(chokeoff);
  }
}

int mainsSuppliesPower() {
  int val = digitalRead(pin_mainpwr);
  if (val == 0) {
    // Serial.println("MainsPWR LO");

    digitalWrite(pin_ledmain, 1);
    return 1;
  } else {
    // Serial.println("MainsPWR HI");
    digitalWrite(pin_ledmain, 0);
    return 0;
  }
}

int genSuppliesPower() {
  int val = digitalRead(pin_genpwr);
  if (val == 0) {
    // Serial.println("GenPWR LO");
    digitalWrite(pin_ledgen, 1);
    return 1;
  } else {
    // Serial.println("GenPWR HI");
    digitalWrite(pin_ledgen, 0);
    return 0;
  }
}

void genRun(int setting) {
  // generator RUN position 0/1
  if (setting == 1) {
    digitalWrite(pin_runRelay, 0); // start
  } else {
    digitalWrite(pin_runRelay, 1); // stop
  }
}

void starter(int setting) {
  // starter motor 0/1
  if (setting == 1) {
    digitalWrite(pin_starterRelay, 0); // start
  } else {
    digitalWrite(pin_starterRelay, 1); // stop
  }
}

void connectgenload(int setting) {
  // starter motor 0/1
  if (setting == 1) {
    digitalWrite(pin_loadRelay, 0); // connect GEN to load
  } else {
    digitalWrite(pin_loadRelay, 1); // disconnect GEN
  }
}


int generatorStartAttempt() {

  beep(1);
  delay(beeptime); // warn people

  choke(1); // close choke
  Serial.println("Choke Close");
  delay(chokemovementtime); // wait until closed
  beep(0);


  // connect ignition
  genRun(1);
  Serial.println("Connect ignition");

  delay(runstabilizetime); // wait until relays switched
  Serial.println("Run starter motor");
  starter(1);

  // run start motor for x time
  //delay(startermotortime);
  //starter(0);
  //Serial.println("Stop starter motor");

  delay(chokeextratime); // could be 0
  choke(0); // full open
  Serial.println("Choke Open");

  // run start motor a little more after choke close
  delay(postchokestartertime);
  starter(0);
  Serial.println("Stop starter motor");

  // CHECK if generating power or not
  Serial.print("Waiting for power ");
  Serial.print(genstabilizetime, DEC);
  Serial.println("ms");
  delay(genstabilizetime);

  //if (random(0, 10) > 8) {
  if (genSuppliesPower()) {
    Serial.println("Generating power, cool!");
    return 1;
  } else {
    Serial.println("Uh oh, no generator power detected :/");
    // TODO we SHOULD turn off ignition AND wait here, because:
    // - if the generator voltage detection fails,
    //   the machine could still be running on the next start attempt,
    //   even if no power is detected.
    //   By turning ignition OFF and waiting 10 secs or so for shutdown,
    //   we avoid engaging the starter motor on a running engine (this can be different levels of bad).

    // TODO add and test this:
    // genRun(0);
    // wait
    
    return 0;
  }
}

int startGenerator() {
  // try x (startattempts) times

  while (!genrunning) {
    currentstartattempt++;
    genrunning = generatorStartAttempt();

    // TODO check if power meanwhile came back here,
    if ( mainsSuppliesPower()) {
      Serial.println("Mains came back while we started. Stop.");
      stopGenerator();
      return 1;
    }

    if (genrunning) {
      // connect genset after a certain warmup time
      Serial.print("Successful generator start, connect load in ");
      Serial.print(warmuptime, DEC);
      Serial.println("ms");
      delay(warmuptime); // time from stable to load connection
      connectgenload(1);
      Serial.println("Connected Load to Gen");
      return 1; // Successful start
    } else {
      if (currentstartattempt < startattempts) {
        Serial.print("Failed generator start ");
        Serial.print(currentstartattempt, DEC);
        Serial.print("/");
        Serial.print(startattempts, DEC);
        Serial.print(", retrying in ");
        Serial.print(restartattempttime, DEC);
        Serial.println("ms");
        delay(restartattempttime); // time between attempts
      } else {
        Serial.print("GAVE UP starting generator");
        genRun(0);
        return 0; // GAVE UP
      }
    }
  }
}


void stopGenerator() {
  // allow cooldown time. This should be rewritten to allow async = restart if mains was lost again during cooldown
  Serial.println("Disconnect generator load");
  connectgenload(0);
  Serial.print("Stop generator after cooldown time of ");
  Serial.print(cooldownsecs, DEC);
  Serial.println("s");

  int counter = cooldownsecs;
  while (counter > 0) {
    delay(1000); // 1 sec increments
    counter--;
    if (!mainsSuppliesPower()) {
      Serial.println("Mains failed! Reconnect generator load, abort gen shutdown");
      connectgenload(1); // reconnect generator
      return; // STOP generator shutdown
    } else {
      Serial.print("Shutdown in ");
      Serial.print(counter, DEC);
      Serial.println("s");
    }
  }
  // delay(cooldowntime); // need to watch mains here. If it fails again, return without stopping

  Serial.println("Stop generator");
  genRun(0);
  genrunning = 0;
  currentstartattempt = 0; // clean cycle, ready to restart in the future
}

void printonce(int state, int laststate, String message) {
  if (state != laststate) {
    Serial.println(message);
  }
}

// read 10 times, require 10 outs
int debouncedMainsSuppliesPower() {
  const int required = 10;
  int score = 0;
  int wait = 50; // start time of 50ms results 9867ms loop time
  for (int i = 0; i < required; i++) {
    int yes = mainsSuppliesPower();
    if (yes) {
      score++;
      beep(0);
    } else {
      if (!genrunning) {
        beep(1); // gen could be running, no need to beep 24/7
      }
    }
    delay(wait);
    Serial.println("DebounceMains " + String(yes) + " wait " + wait + " Score " + score );
    wait = wait * 1.618; // fibonacci to avoid noise in fixed frequencies
  }
  return score;
}

// main
void loop() {
  // testRelay(pin_loadRelay);

  // if ( mainsSuppliesPower()) {
  // 10 sec wait, 10 samples
  int mainsScore = debouncedMainsSuppliesPower();
  if ( mainsScore > 0 ) { // we have some indication of mains OK
    genSuppliesPower(); // just to pull status led
    state = state_mainsok;
    // printonce(state, laststate, "Mains OK");
    Serial.println("Mains OK");

    // reconnect mains (if wired with MAINS being PRI, it is already connected by now, and generator disconnected)

    // if generator is running and mains is very much up, stop it
    if (genrunning && mainsScore == 10) {
      stopGenerator();
      //      Serial.println("Debounce mains before stopping gen");
      //      delay(500);
      //      if ( mainsSuppliesPower() ) {
      //        Serial.println("Debounced, mains still in.");
      //        stopGenerator();
      //      } else {
      //        Serial.println("Debounced - must have been a fluke.");
      //      }
    }
  } else {
    state = state_mainsnok;
    if (genSuppliesPower()) {
      state = state_genok;
      printonce(state, laststate, "Gen OK");
    } else {
      if (laststate == state_stuck) {
        state = state_stuck; // keep stuck
        printonce(state, laststate, "Stuck! Tried starting generator but gave up");

        // need to tell someone. Could be out of fuel, etc

      } else {
        // no Mains, no Gen
        //        delay(500);
        //        if ( mainsSuppliesPower() ) {
        //          Serial.println("Debounced - must have been a fluke.");
        //        } else {
        Serial.println("Debounced, mains still out");
        state = state_mainsgennok;
        printonce(state, laststate, "Mains, Gen NOK!");

        // could need cooldown time between attempts maybe.

        // In any case run the generator 30secs after mains is back.
        // If mains is lost during cooldown period
        if (!currentstartattempt) {
          startGenerator(); // TODO currently a blocking attempt; mains could come back meanwhile, and the generator would then stop after starting, works okish for now

          // startgenerator should also (try to) connect GEN power. If main meanwhile came back, generator will just stop on next round.
        } else {
          // If generator started but later stopped by itself, it needs manual attention.
          state = state_stuck;
        }
        //        }
      }
    }
  }

  laststate = state;
}
