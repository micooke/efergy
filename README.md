# Arduino-based Efergy E2 Classic decoder
The receiver code was designed for an esp8266, but it should work with any arduino.
The decoder is for the bitstream, so for the baseband PWM signal - not the 433MHz RF.

# Acknowledgements
I started this project to help @mr-sneezy, who was looking at https://github.com/hardtechnology/efergy_v2
This is written by @steven-geo at @hardtechnology. Check it out as he has some great examples for logging and using slack.

I was going to debug this, but I ended up writing a receiver and decoder in about an hour using pin change interrupts which was simpler for me and **should** suit @mr-sneezy's application better (integration with Home Assistant) as it is non-blocking.

Check out my examples for the transmitter and receiver.

As I dont have an efergy, I needed to first create a simulator (```efergy_tx.h```) which shamelessly utilises my own PWM library.
Please dont look at this code :) Getting that first marker pulse is a mess and makes no sense. I think the issue is that my interrupts
take too long (it takes 5us to enter an ISR to begin with, and I had 20us between the compare ISR and the overflow ISR); so I dont have time to set the pulse width, period and pin inversion correctly. What we are left with a hack and when my OCD gets the better of me I will fix it, but hey it works (confirmed with saleae logic captures).

If I haven't scared you off, for the transmitter you need an ATmega328 device: Arduino uno, nano or similar. The output is on OCR1A pin == ```D9```.
You can specify your own bit stream as a 64 character string (I include 3 examples).

The receiver (```efergy_rx.h```) is setup to decode a 64b frame from an Efergy E2 classic, not the elite.
The input pin is setup for ```GPIO5``` in the example, but you can set it to any GPIO pin (except for ```GPIO16``` - the wake pin) using the init function e.g. ```efergy_rx::init(5);```.

# Historical
The original idea using an rtl-sdr and a rpi (EfergyRPI_001.c -> EfergyRPI_log.c).
This was written by Nathaniel Elijah, then updated by [Gough](me@goughlui.com).
There was also (slightly) more recent work on this by @magellannh, check it out [here](https://github.com/magellannh/RPI_Efergy)

If you are interested in decoding the 433MHz signal, check out @magellannh's work.
