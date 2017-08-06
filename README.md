# redis-uniqueid

Unique ID generator as a redis module.

It registers `uniqueid.get` command to redis, which returns a unique ID as a 64-bit integer with a [snowflake](https://blog.twitter.com/engineering/en_us/a/2010/announcing-snowflake.html)-like fashion.

The 64 bit consists of 41 bits of timestamp (in milliseconds), 10 bits of worker id, 12 bits of sequential number, and the sign which is actually always positive.
