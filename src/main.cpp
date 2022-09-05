
   
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
#define SAMPLE_RATE 44100

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

uint16_t attack = 512;
uint16_t decay = 512;
uint16_t sustain = 512;
uint16_t release = 512;

uint64_t samplePeriod = 1000000/SAMPLE_RATE;

void setCV(int value)
{
    myDac.analogWrite(false , false, true, true, value);
}


void handlePWM()
{

}

///
void setup()
{
    myDac.begin();
}

void loop()
{
    
}