#include <Arduino.h>
#include <MIDI.h>
#include "noteList.h"
#include "pitches.h"
#include "MCP492X.h"

#define PIN_SPI_CHIP_SELECT_DAC 9

MCP492X myDac(PIN_SPI_CHIP_SELECT_DAC);

static const unsigned sPitchClockOutPin = 4;
static const unsigned sPitchClockInPin = 2;
static const unsigned sMaxNumNotes = 16;
MidiNoteList<sMaxNumNotes> midiNotes;

// -----------------------------------------------------------------------------

bool isGateActive = false;
bool attackDone = false;
bool decayDone = false;

#define NONE        0
#define CLOCK_HIGH  1
#define CLOCK_LOW   2

#define ATTACK_CC   20
#define DECAY_CC    21
#define SUSTAIN_CC  22
#define RELEASE_CC  23

volatile byte functionToCall = NONE;

MIDI_CREATE_DEFAULT_INSTANCE();

inline void handleGateChanged(bool inGateActive)
{
   isGateActive = inGateActive;
   attackDone = decayDone = 0;
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
            tone(sPitchClockOutPin, sNotePitches[currentNote+1-24]);

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

uint16_t attack = 1;
uint16_t decay = 512;
uint16_t sustain = 512;
uint16_t release = 150;

void setCV(uint16_t value)
{
    myDac.analogWrite(false , false, true, true, value);
}

const uint8_t sAttackPin = A0;
const uint8_t sDecayPin = A1;
const uint8_t sSustainPin = A2;
const uint8_t sReleasePin = A3;

double voltage = 1.0;

uint32_t tmpTime = 0;
uint32_t newTime = 0;

#define MAX_VOLTAGE 4095.0

void doAttack(uint16_t attack, double* voltage, uint64_t time)
{
    attack += 1;
    double factor = ( MAX_VOLTAGE / ((double)attack * 4.0)) * ( (double)time / 1E6 );
    *voltage += MAX_VOLTAGE * factor;
    if (*voltage >= MAX_VOLTAGE)
    {
        *voltage = MAX_VOLTAGE;
        attackDone = true;
    }
}

void doDecay(uint16_t sustain, uint16_t decay, double* voltage, uint64_t time)
{
    double sustainComputation = (double)sustain * 4 - 4096;
    double decayComputation = 1024.0 - (double)decay;
    double timeFactor = (double)time/1E6;

    double factor = (sustainComputation / decayComputation) * timeFactor;
    
    *voltage += MAX_VOLTAGE * factor;
    if (*voltage <= (double)sustain * 4.0)
    {
        *voltage = (double)sustain * 4.0;
        decayDone = true;
    }
}

void doSustain(uint16_t sustain, double* voltage)
{
    *voltage = (double)sustain * 4.0;
}

void doRelease(uint16_t release, double* voltage, uint64_t time)
{
    double timeFactor = (double)time / 1E6;
    double releaseComputation = (1024.0 - (double)release) * 4.0;
    double factor = -(4096.0 / releaseComputation) * timeFactor;

    *voltage += MAX_VOLTAGE * factor;
    if (*voltage <= 0.0)
        *voltage = 0.0;
}

void audioClockHigh()
{
    newTime = micros();
    if (isGateActive == true)
    {
        if (!attackDone)
            doAttack(attack, &voltage, newTime - tmpTime);
        else if (!decayDone)
            //decay = analogRead(sDecayPin);
            doDecay(sustain, decay, &voltage, newTime - tmpTime);
        else
            doSustain(sustain, &voltage);
    }
    else if (isGateActive == false)
    {
        attackDone = false;
        decayDone = false;
        doRelease(release, &voltage, newTime - tmpTime);
    }
    setCV((uint16_t)voltage);
    tmpTime = newTime;
    functionToCall = NONE;
}

void audioClockLow()
{
    setCV(0);
    functionToCall = NONE;
}

void setAudioFunction()
{
    if (digitalRead(sPitchClockInPin) == HIGH)
        functionToCall = CLOCK_HIGH;
    else
        functionToCall = CLOCK_LOW;
}

void controlChange(byte channel, byte controlNumber, byte value)
{
    switch (controlNumber)
    {
        case ATTACK_CC:
            attack = value * 8;
            break;
        case DECAY_CC:
            decay = value * 8;
            break;
        case SUSTAIN_CC:
            sustain = value * 8;
            break;
        case RELEASE_CC:
            release = value * 8;
            break;
    }
}

void removeAllNote()
{
    MidiNoteList<sMaxNumNotes> newMidiNotes;
    midiNotes = newMidiNotes;
}


///
void setup()
{
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandleControlChange(controlChange);
    MIDI.begin(3);
    myDac.begin();
    pinMode(sPitchClockOutPin, OUTPUT);
    pinMode(sPitchClockInPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sPitchClockInPin), setAudioFunction, CHANGE);
}

int theNote = 15;
int nt = 0;
int nt2 = 0;
bool truc = false;
void loop()
{
    MIDI.read();
    if (functionToCall == CLOCK_HIGH)
        audioClockHigh();
    else if (functionToCall == CLOCK_LOW)
        audioClockLow();
}