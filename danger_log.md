Danger log

1. If http request contains Connection: keep-alive and the response length happens to be multiple of 65536, the recv() loop will not exit. // fixed using package

2. Caching with max-age field does not correctly log the expire date. // fixed

3. Caching is not thread-safe. // fixed using r/w lock

4. An unlock with no lock pairing with it, effectively blocking the thread. // fixed

5. Double freeing when extracted method from main transmit() method. // fixed

6. Can only tell if a tunnel has closed when either side tries to write to a closed socket, instead of getting that the connection should have been closed in time.

7. Exception handling is too general. No request/response specific handling.

8. If cache is validated by server, the new 304 response is returned to the client instead of the cached 200 response. // fixed

9. Gap between time is retrieved and time is used, causing minor time difference when calculating expire time.
10. Requesting twice if revalidation responds with 200 OK, due to the method conducting the validation has no effective way to send the new response back to the client and ending the thread execution. 
11. If max-age is 0, the proxy should re-validate the cache on every access, but there is chance that if 2 requests are made extremely close, one of the requests will have a change to directly use the cached respond without revalidation.
12. if daemon() returns -1 the program will continue to execute, but as a normal process.

Cache Policy:
- store up to 1000 responses (or more if you change the value). Replacement with LIFO, so the oldest record is removed if there is no more space. Remove expired files. 

Guarantee:

thread method is covered in try-catch blocks, guarantees to not throw outside of the thread method. Uses error_code to ensure error handling write() will not throw in catch block.

main while loop guarantees if accept() throws the id remains the same.
