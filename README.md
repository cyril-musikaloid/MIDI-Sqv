# MIDI-Sqv

This branch is a test of a sort of "custom" Karplus-Strong algorithme

In fact, the digitalPin sPitchClockOut will play the tone and work like a clock

sPitchClockIn will work with interrupts plugged in the sPitchClockOut to command the DAC :
- LOW -> 0V
- HIGH -> The amplitude of signal in function of the ADSR evolution

