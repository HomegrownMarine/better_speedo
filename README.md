better_speedo
=============

For Arduino: Turn pulses from airmar speedo paddles into NMEA speed sentences.

As I’ve learned to sail, I’ve constantly wanted better data out of my instruments so that I could better analyze the performance of different maneuvers.  Moving to a 5Hz GPS last year really illustrated this point.

Here, you can see a tack, where the speedo never even registered within 3/4 of a knot of the lowest speed, and was well over a second behind the GPS in response (this isn’t just a calibration issue, we were in a little bit of current).

![](https://raw.githubusercontent.com/HomegrownMarine/better_speedo/master/README/before_speed_vs_sog.png)

After reading Merlin’s excellent blog here: http://sailboatinstruments.blogspot.com/2011/03/measuring-boat-speed-part-1.html, I decided I could just hook the signal wires from the speedo into the Arduino in my Boat Computer (link).  I realized that if I really wanted a fast response, I’d need to filter the raw pulses.  I figured that I could output 5 or 10Hz, filtered from the raw pulses, and that other parts of the system shouldn’t need to do additional filtering or smoothing.

I researched a bit about signal smoothing and after a little trial and error, decided to go with a simple weighted average using approximately the last 1s worth of pulses.  I went with a scheme that weighed the most recent point twice as heavily as the oldest point.  In the end, calculating speed at 5Hz, this system responds faster than my GPS.  I hoped to do a better test last weekend, but had some power issues on the boat.  I’ll post more results in March when the season picks up.

![](https://raw.githubusercontent.com/HomegrownMarine/better_speedo/master/README/after_speed_vs_sog.png)
