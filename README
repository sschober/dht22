Read Humidity and Temperature with DHT22 Sensor
===============================================

`dht22` is a small daemon, if you like, that continuously aquires
humidity and temperature readings at configurable intervals and
prints these to `stdout` and to a file called `log.csv` in the
current working directory. Each line therein is formatted as
follows:

    <YYYY-MM-DD>;<HH:MM:SS>;kk.k;ll.l

With `k` being a digit of a humidity reading and `l` corespondingly
being a digit of a temperature reading. A sample line might look
like the following:

    2017-12-10;15:17:31;51.1;20.7

Here, a humidity of 50.1% and a temperature of 20.7C was read on 10.
decembre 2017 at 15:17:31.


Invocation
----------

`dht22` can be invoked as following:

    dht22 [<read delay in ms> [<number of retries> [<retry delay in ms]]]

A sample invocation might look like this:

    dht22

Which would trigger default values for read and retry delay and
number of retries. Another sample invocation might be:

    dht22 60000 10 600

which would try to aquire a reading every minute, with 3 retries and
a retry delay of 600ms.


Build
-----

Just the usual

    make

should suffice.


Author
------

The code is heavily inspired by various sources found on the net and
tweaked a bit by me.

Sven Schober <sv3sch@gmail>
