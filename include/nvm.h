#ifndef NVM_H
#define NVM_H

// CRC size in number of bytes
#define NVM_CRC_SIZE_BYTES          (sizeof(crc_t) / sizeof(uint8_t))

// CRC size in number of characters (including end of string character)
#define NVM_CRC_SIZE_CHARS          ((NVM_CRC_SIZE_BYTES * 2) + 1)

// CRC value for use in ROM defaults 
#define NVM_CRC_DEFAULT             (crc_t(0))

// Default buffer value for use in ROM defaults
#define NVM_BUFFER_DEFAULT          (uint32_t(0xDEADBEEF))

// Maximum error number (do not use brackets on this number)
#define NVM_MAX_ERROR               9999

// Maximum version number (do not use brackets on this number)
#define NVM_MAX_VERSION             9999

// NVM base address
#define NVM_BASE_ADDRESS            (0)

// Default for NVM version
#define NVM_DATA_VERSION_DEFAULT    (1)

// Default for NVM errors
#define NVM_DATA_ERRORS_DEFAULT     (0)


// CRC datatype (32bit)
typedef uint32_t crc_t;

// NVM structure footer
typedef struct {
     uint32_t               buffer;              // Buffer for ensuring structure is > 32bits and not going to contain DATA 0xFFFFFFFF / CRC 0xFFFFFFFF
     crc_t                  crc;                 // CRC
} nvmFooterCrc;

// NVM structure core data
typedef struct {
     uint16_t                errorCounter;       // Number of recovered NVM errors
     uint16_t                version;            // Version of NVM data
} nvmCoreData;

// NVM structure configuration
typedef struct {
    uint8_t * const         addressRamMirror;    // Pointer to base address of the RAM mirror (pointer RO, data RW)
    const uint8_t * const   addressRomDefault;   // Pointer to base address of the ROM defaults (pointer RO, data RO)
    uint8_t * const         addressCrc;          // Pointer to crc address of the RAM mirror (pointer RO, data RW)

    const uint32_t          length;              // Length (in bytes) of the structure excluding the CRC

    const bool              rewriteWhenCorrupt;  // Allow defaults to be restored when NVM structure is corrupt
} nvmStructureConfig;

// NVM data message
typedef struct {
    nvmCoreData             core;

    uint32_t                bytesConsumed;
    uint32_t                structures;
} nvmData;


/**
    Clear the NVM contents.
    This call is BLOCKING.
*/
void nvmClear(void);

/**
    Update indexed RAM mirror structure CRC based on the current structure contents.
  
    @param[in]     index index of the NVM structure the will have the CRC update.
*/
void nvmUpdateRamMirrorCrcByIndex(uint32_t index);

/**
    Comitt the RAM mirror directly to NVM.
    Before calling, make sure the appropiate structure CRC's have been updated.
    This call is BLOCKING (writing the RAM mirrors to EEPROM).
*/
void nvmComittRamMirror(void);

/**
    Initialise the NVM module.
    Include initialisation of the RAM mirrors and recovery to defaults where allowed.
    This call is BLOCKING (reading / populating the RAM mirrors from EEPROM).
*/
void nvmInit(void);

/**
    Transmit a NVM status message.
    No processing of the message here.
*/
void nvmTransmitStatusMessage(void);

#endif
