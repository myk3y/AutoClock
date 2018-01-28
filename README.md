# AutoClock


![WeMos](https://github.com/myk3y/AutoClock/blob/master/Screen%20Shot%202018-01-27%20at%207.47.28%20PM.png)

A simple internet-enabled clock driven by NTP and GeoIP to give an automatic offset from UTP for your location.

My wife bought a clock that seems to lose about ten minutes a day and is constantly adjusting it, and it drove me bonkers. The clock maker, who will remain nameless (but Bush Electronics seems so appropriate for a clock maker that's clocks don't keep even close to the correct time... https://en.wikipedia.org/wiki/Bush_(brand)) has made my days a constant quest for the correct time - I think I have developed Chronophobia.

To resolve this:

I used a WeMos D1 Mini, but you could use any ESP8266 module or any ethernet shield or wireless shield, with suitable modifications.

I am building a much bigger clock, but used this code and the tiny < 1 inch OLED display to sort out the automatic NTP time and setting the offset for the location.

It has been tested in Welligton NZ and Brunei Darussalam as of this writing.

It uses JSON to parse the API data from http://ip-api.com and https://timezonedb.com, but in theory you could use any
GEO-IP database and timezone reconciliation service. These are both free for hobby use, don't sell your details to 
the Mob and seem to not have sent me one single piece of spam in the last year. 

You will need to sign up for an API with https://timezonedb.com but it's more to track you don't abuse the system than 
to assist any Nigerian princes looking to get a partner or to help them repatriate the millions of dollars they have stuck
in the bank.
