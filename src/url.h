#ifndef NETDATA_URL_H
#define NETDATA_URL_H 1

// ----------------------------------------------------------------------------
// URL encode / decode
// code from: http://www.geekhideout.com/urlcode.shtml

/* Converts a hex character to its integer value */
extern char from_hex(char ch);

/* Converts an integer value to its hex character*/
extern char to_hex(char code);

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
extern char *url_encode(char *str);

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
extern char *url_decode(char *str);

#endif /* NETDATA_URL_H */
