#ifndef VERSION_H
#define VERSION_H

// Structure for version data
typedef struct {
  const char*   versionName;
  char*         versionContents;
} versionData;


/**
    Initialise the version module.
*/
void versionInit(void);

/**
    Transmit a version message.
    No processing of the message here.
*/
void versionTransmitVersionMessage(void);

/**
    Set-up read pointer to the version data.
        
    @param[in]     activeVersionData pointer for the version data.
    @return        number of version elements.
*/
const uint32_t versionGetData(const versionData ** activeVersionData);

#endif
