
   
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
#define SAMPLE_RATE 25000

MCP492X myDac(PIN_SPI_CHIP_SELECT_DAC);

static const unsigned sPitchClockOutPin = 4;
static const unsigned sPitchClockInPin = 2;
static const unsigned sMaxNumNotes = 16;
MidiNoteList<sMaxNumNotes> midiNotes;

// -----------------------------------------------------------------------------

bool isGateActive = false;

inline void handleGateChanged(bool inGateActive)
{
   // digitalWrite(sGatePin, inGateActive ? HIGH : LOW);
   isGateActive = inGateActive;
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
        noTone(sPitchClockOutPin);
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
            tone(sPitchClockOutPin, sNotePitches[currentNote]);

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

void setCV(int value)
{
    myDac.analogWrite(false , false, true, true, value);
}


void handlePWM()
{

}

bool attackDone = false;
bool decayDone = false;

const uint8_t sAttackPin = A0;
const uint8_t sDecayPin = A1;
const uint8_t sSustainPin = A2;
const uint8_t sReleasePin = A3;

uint16_t voltage = 0;

void doAttack(uint16_t attack, uint16_t voltage, uint64_t time)
{
    
}

void audioClockHigh()
{
    if (isGateActive)
    {
        if (!attackDone)
        {
            //attack = analogRead(sAttackPin);
            doAttack(attack, voltage, micros());
        }
        else if (!decayDone)
        {
            //sustain = analogRead(sSustainPin);
            //decay = analogRead(sDecayPin);
            doDecay(sustain, decay, voltage, micros());
        }
        else
        {
            //sustain = analogRead(sSustainPin);
            doSustain(sustain, voltage);
        }
    }
    else
    {
        //sustain = analogRead(sSustainPin);
        //release  = analogRead(sReleasePin);
        doRelease(sustain, release, voltage, micros());
    }
}

void audioClockLow()
{

}

///
void setup()
{
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.begin(3);
    myDac.begin();
    pinMode(sPitchClockOutPin, OUTPUT);
    pinMode(sPitchClockInPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sPitchClockInPin), audioClockHigh, RISING);
    attachInterrupt(digitalPinToInterrupt(sPitchClockInPin), audioClockLow, FALLING);
}

void loop()
{
    MIDI.read();

}