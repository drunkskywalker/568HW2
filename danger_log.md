If http request contains Connection: keep-alive and the response length happens to be multiple of 65536, the recv() loop will not exit. // fixed using package

Caching with max-age field does not correctly log the expire date. // fixed

Double freeing when extracted method from main transmit() method. // fixed

Can only tell if a tunnel has closed when either side tries to write to a closed socket, instead of getting that the connection should have been closed in time.

Exception handling is too general.

