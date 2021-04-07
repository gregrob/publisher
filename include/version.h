#ifndef VERSION_H
#define VERSION_H

// Structure for version data
typedef struct {
  const char*   versionName;
  const char*   versionContents;
} versionData;


/**
    Transmit a version message.
    No processing of the message here.
*/
void versionTransmitVersionMessage(void);

#endif
