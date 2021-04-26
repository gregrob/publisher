#include <Arduino.h>

#include "version.h"
#include "version_cfg.h"

#include "utils.h"
#include "messages_tx.h"


// Maximum size of a version string
#define VERSION_VERSION_STRING_MAX      (32)

// Name for the app version
#define VERSION_NAME_APP                "app-ver"

// Name for the compiled time
#define VERSION_NAME_APP_COMPILED       "app-compiled"

// Name for the build time
#define VERSION_ESP_CORE                "esp-core"

// Name for the build time
#define VERSION_ESP_SDK                 "esp-sdk"

// Name for the build time
#define VERSION_ESP_FLASHID             "esp-flashid"


// App version data
static const char * versionApp = VERSION_CONTENTS_MAJOR "." VERSION_CONTENTS_MINOR "." VERSION_CONTENTS_BUGFIX;
static const char * versionAppCompiled = __DATE__ " " __TIME__;

// Version data for ESP
static char versionEspCore[VERSION_VERSION_STRING_MAX];
static char versionEspSdk[VERSION_VERSION_STRING_MAX];
static char versionEspFlashId[STRNLEN_INT(MAX_VALUE_32BIT_UNSIGNED_HEX) + 1];

// Version data for this software
static const versionData versionDataSoftware[] = {{VERSION_NAME_APP,            (char *)versionApp},
                                                  {VERSION_NAME_APP_COMPILED,   (char *)versionAppCompiled},
                                                  {VERSION_ESP_CORE,            versionEspCore},
                                                  {VERSION_ESP_SDK,             versionEspSdk},
                                                  {VERSION_ESP_FLASHID,         versionEspFlashId}
};                                                                                                                                                    

// Size of the versionDataSoftware structure
static const unsigned int versionDataStructureSize = (sizeof(versionDataSoftware) / sizeof(versionDataSoftware[0]));


/**
    Initialise the version module.
*/
void versionInit(void) {
    snprintf(versionEspFlashId, sizeof(versionEspFlashId), "0x%.8X", ESP.getFlashChipId());
    strncpy(versionEspSdk, ESP.getSdkVersion(), sizeof(versionEspSdk));
    strncpy(versionEspCore, ESP.getCoreVersion().c_str(), sizeof(versionEspCore));
}


/**
    Transmit a version message.
    No processing of the message here.
*/
void versionTransmitVersionMessage(void) {
    messsagesTxVersionMessage(versionDataSoftware, &versionDataStructureSize);
}


/**
    Set-up read pointer to the version data.
        
    @param[in]     activeVersionData pointer for the version data.
    @return        number of version elements.
*/
const uint32_t versionGetData(const versionData ** activeVersionData) {
    *activeVersionData = &versionDataSoftware[0];
    return(versionDataStructureSize);
}
