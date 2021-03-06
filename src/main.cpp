
   
   #include <Arduino.h>
#include <MIDI.h>
#include "noteList.h"
#include "pitches.h"
#include "MCP492X.h"

MIDI_CREATE_DEFAULT_INSTANCE();

#ifdef ARDUINO_SAM_DUE // Due has no tone function (yet), overriden to prevent build errors.
#define tone(...)
#define noTone(...)
#endif

// This example shows how to make a simple synth out of an Arduino, using the
// tone() function. It also outputs a gate signal for controlling external
// analog synth components (like envelopes).

#define PIN_SPI_CHIP_SELECT_DAC 9

MCP492X myDac(PIN_SPI_CHIP_SELECT_DAC);

static const unsigned sAudioOutPin = 4;
static const unsigned sMaxNumNotes = 16;
MidiNoteList<sMaxNumNotes> midiNotes;

// -----------------------------------------------------------------------------

inline void handleGateChanged(bool inGateActive)
{
   // digitalWrite(sGatePin, inGateActive ? HIGH : LOW);
}

inline void pulseGate()
{
    handleGateChanged(false);
    delay(1);
    handleGateChanged(true);
}

// -----------------------------------------------------------------------------

void handleNotesChanged(bool isFirstNote = false)
{
    if (midiNotes.empty())
    {
        handleGateChanged(false);
        //noTone(sAudioOutPin); // Remove to keep oscillator running during envelope release.
    }
    else
    {
        // Possible playing modes:
        // Mono Low:  use midiNotes.getLow
        // Mono High: use midiNotes.getHigh
        // Mono Last: use midiNotes.getLast

        byte currentNote = 0;
        if (midiNotes.getLast(currentNote))
        {
            tone(sAudioOutPin, sNotePitches[currentNote]);

            if (isFirstNote)
            {
                handleGateChanged(true);
            }
            else
            {
               // pulseGate(); // Retrigger envelopes. Remove for legato effect.
            }
        }
    }
}

// -----------------------------------------------------------------------------

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
    const bool firstNote = midiNotes.empty();
    midiNotes.add(MidiNote(inNote, inVelocity));
    handleNotesChanged(firstNote);
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    midiNotes.remove(inNote);
    handleNotesChanged();
}

// -----------------------------------------------------------------------------

int pwmTab[] = {255, 255, 255, 0, 255,127,192,255,255};
int i = 0;
unsigned long refTime = 0;
int currentCV;

void setCV(int value)
{
    currentCV = value;
  myDac.analogWrite(false , false, true, true, value);
}


void handlePWM()
{
    unsigned long tmpTime = millis();

    long diff = tmpTime - refTime;

    if ((diff) > 0 )
    {
        setCV(currentCV + (diff));

    if (currentCV >= 4095)
        currentCV = 0;

    refTime = tmpTime;
    }   
}

///
void setup()
{
    pinMode(sAudioOutPin, OUTPUT);
    //MIDI.setHandleNoteOn(handleNoteOn);
    //MIDI.setHandleNoteOff(handleNoteOff);
    //MIDI.begin(3);
    myDac.begin();
    //setCV(2500);
    tone(sAudioOutPin, 440);
}

void loop()
{
   //MIDI.read();
   handlePWM();
}