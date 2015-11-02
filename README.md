# snowbot - a small IRC bot written in C

snowbot is a simple IRC bot that responds to queries in public and private messages and allows custom settings for different users.

It doesn't depend on any IRC library (anymore), but certain functions require extra libraries:

- libcurl for retrieving weather, exchange rates and working with pastebin,

- libjson-c for parsing weather and exchange rate data,

- libcrypto for (todo) password hashing and user authentication.

