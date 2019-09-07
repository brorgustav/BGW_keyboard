//BGW axiom32 keyboard, midi USB
//http://synthbror.com
//https://github.com/brorgustav/Axiom32_keyboard/
//free to use


#include "BGW_Keyboard.h"
#include "MIDIUSB.h"
#define debug SERIAL_PORT_MONITOR
#define ledPin LED_BUILTIN
#define num_keys 32
const byte numRows = 8; //four rows
const byte numCols = 8; //four columns
//    R = Rows     C = Columns      VC = Columns for velocity (same keys as in columns)
//                            AXIOM 32 flex cable pinout: R1,R2,R3,R4,R5,R6,R7,R8,C1,VC1,VC2,C2,C3,VC3,C4,VC4
// Mapping in order to matrix adress map((keyboardnote)): R1,R2,R3,R4,R5,R6,R7,R8,C1,C2,C3,C4,VC1,VC2,VC3,VC4
byte keyboardRows[numRows] = {5, 4, 3, 2, 1, 0, 21, 20}; //ROWS of the keyboard, i used mkrzero
byte keyboardCols[numCols] = {6, 9, 10, 12, 7, 8, 11, 13}; //columns
//Hence that with the keyboardNote array and this pin declaration
// i remapped the velocity inputs and placed them as last columns
uint8_t keyboardNote[numRows][numCols] = {
  {1, 9, 17, 25, 33, 41, 49, 57},
  {2, 10, 18, 26, 34, 42, 50, 58},
  {3, 11, 19, 27, 35, 43, 51, 59},
  {4, 12, 20, 28, 36, 44, 52, 60},
  {5, 13, 21, 29, 37, 45, 53, 61},
  {6, 14, 22, 30, 38, 46, 54, 62},
  {7, 15, 23, 31, 39, 47, 55, 63},
  {8, 16, 24, 32, 40, 48, 56, 64}
};

uint8_t keyOffset = 36 - keyboardNote[0][0]; //Defines what midi note key #1 will be
unsigned long velSensitivity = 800; //This variable defines how sensitive the velocity will be
uint8_t fixedChannel = 0; //The MIDI channel the keyboard maps its notes to
unsigned long keyTimer[num_keys]; //Variable used in measuring the time between velocity column and note column on key pressed

unsigned long loopCount;///variables for testing the speed of code
unsigned long startTime;//variables for testing the speed of code

Keyboard BGW_keyboard = Keyboard( makeKeymap(keyboardNote), keyboardRows, keyboardCols, numRows, numCols);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  loopCount = 0;
  startTime = millis();
  BGW_keyboard.setDebounceTime(20); //debounce filters out noise on keypresses, to make sure a key has been pressed
  BGW_keyboard.setHoldTime(7000);  //this defines how long you need to hold down a key before it executes the "hold" state in keypad library
  delay(1000);
  debug.print("maximum polyphony according to keypad.h (#define LIST_MAX) = ");
  debug.println(LIST_MAX);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
  debug.print("[MIDI] NOTE ON:");debug.print(channel);debug.print("|");debug.print(pitch);debug.print("|");debug.print(velocity);  debug.println("");
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
  debug.print("[MIDI] NOTE OFF:"); debug.print(channel); debug.print("|"); debug.print(pitch); debug.print("|"); debug.print(velocity); debug.println("");
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
void loop() {
  // put your main code here, to run repeatedly:
  //  loopCount++;
  //  if ( (millis() - startTime) > 5000 ) {
  //    Serial.print("Average loops per second = ");
  //    Serial.println(loopCount / 5);
  //    startTime = millis();
  //    loopCount = 0;
  //  }

  if (BGW_keyboard.getKeys())
  {
    for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
    {
      if ( BGW_keyboard.key[i].stateChanged )   // Only find keys that have changed state.
      {
        uint8_t tx = BGW_keyboard.key[i].kchar;
        switch (BGW_keyboard.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
            {
              if (tx > num_keys) //If the key is over the number of keys it is one of the velocity keys
              {
                keyTimer[tx - num_keys] = millis();
                debug.print("[");
                debug.print(tx - num_keys);
                debug.print("] = ");
                debug.println(keyTimer[tx - num_keys]);
              }
              else { //if not its a "note key"
                unsigned long elapsed = millis() - keyTimer[tx];
                debug.print("[");
                debug.print(tx);
                debug.print("] = ") ;
                debug.println(elapsed);
                uint8_t velocity = log(elapsed + 1) / log(velSensitivity) * 127; //map the velocity timing logarithmically
                velocity = map(velocity, 0, 127, 127, 0); //invert the velocity value
                velocity = constrain(velocity, 1, 127);
                noteOn(fixedChannel, tx + keyOffset, velocity); // Channel 0, middle C, normal velocity
                keyTimer[tx] = 0;
              }
              break;
            }
          case HOLD:
            {
            }
            break;
          case RELEASED:
            {
              if (tx <= num_keys) //Filter out velocity keys from sending MIDI on release
              {
                noteOff(fixedChannel, tx + keyOffset, 0);
              }
            }
            break;
          case IDLE:
            {
            }
            break;
        }
      }
    }
  }
}
