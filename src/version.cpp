#include <Arduino.h>

#include "version.h"
#include "version_cfg.h"

#include "messages_tx.h"


// Name for the major version
#define VERSION_NAME_MAJOR  ("major")

// Name for the minor version
#define VERSION_NAME_MINOR  ("minor")

// Name for the bugfix version
#define VERSION_NAME_BUGFIX ("bugfix")

// Name for the build date
#define VERSION_NAME_DATE   ("date")

// Name for the build time
#define VERSION_NAME_TIME   ("time")


// Version data for this software
static const versionData versionDataSoftware[] = {{VERSION_NAME_MAJOR,  VERSION_CONTENTS_MAJOR},
                                                  {VERSION_NAME_MINOR,  VERSION_CONTENTS_MINOR},
                                                  {VERSION_NAME_BUGFIX, VERSION_CONTENTS_BUGFIX},
                                                  {VERSION_NAME_DATE,   __DATE__},
                                                  {VERSION_NAME_TIME,   __TIME__}
};                                                                                                                                                    

// Size of the versionDataSoftware structure
static const unsigned int versionDataStructureSize = (sizeof(versionDataSoftware) / sizeof(versionDataSoftware[0]));


/**
    Transmit a version message.
    No processing of the message here.
*/
void versionTransmitVersionMessage(void) {   
    messsagesTxVersionMessage(versionDataSoftware, &versionDataStructureSize);
}
