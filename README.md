# Generator autostart
I currently have a Chinese Loncin 13000W 230V/60Hz generator powering my house during blackouts.

It had an ignition key and self starter, but also manual choke (carburetor, no ignition). When in an important meeting, it quickly became pretty stressful to run out and start (or stop) the generator manually. Was also hard to explain start/stop handling to my wife - and the apparatus was already scary in the first place.

This collection holds my builds and code to get my generator to:
* autostart when power is lost
  - retry 3 times and give up if it didn't start generating power. This is to save the starter motor.
* transfer power to the generator (after a warm up period)
* when mains power is back, transfer power to mains (pretty easy, just crosswire the contactors)
* autostop (after a cool down period)

Later I thought it would be cool to add a Wi-Fi module to the (independent) autostarter, with these features:
* Statistics - not only for fun and accounting, but also to calculate fuel consumption per hour, and thereby
* Predict fuel remaining on tank
* Allow Inhibiting starts manually or by schedule (might not want to run the generator while we sleep)
* Allow force starting manually or by schedule (for test runs, to maintain the battery charge, etc)

## Future
all the Loncin-buddy related code is found in the loncin1/ folder; with some luck (or lack of, depending on your point of view) there will be improved versions.
