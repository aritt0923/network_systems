#ifndef STATUS_CODES_H_
#define STATUS_CODES_H_

/*
 * Server return status codes
 */

#define OK 200          // OK: request is valid
#define BAD_REQ 400     // Bad Request: request could not be parsed or is malformed
#define FBDN 403        // Forbidden: permission issue with file
#define NOTFOUND 404          // Not Found: file not found
#define MNA 405         // Method Not Allowed: a method other than GET was requested
#define HTTP 505        // HTTP Version Not Supported: an http version other than 1.0 or 1.1 was requested
#define BAD_RESP 500    // Response from remote server was wonky 

#endif //STATUS_CODES_H_