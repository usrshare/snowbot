#ifndef CONFIG_H
#define CONFIG_H

// This configuration file allows to define, during compilation, whether certain features are enabled or not.

#define IRC_MAX_NICK_LEN 9
// specifies how many bytes an IRC nickname can store.
// this value should be increased, if the bot is to run on servers with extensions.

#define ENABLE_URL_NOTIFY 0
// checks if URLs typed in chat are part of a large notify list.

#endif
