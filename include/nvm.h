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

// NVM base address
#define NVM_BASE_ADDRESS            (0)


// CRC datatype (32bit)
typedef uint32_t crc_t;

// NVM structure footer
typedef struct {
     uint32_t               buffer;              // Buffer for ensuring structure is > 32bits and not going to contain DATA 0xFFFFFFFF / CRC 0xFFFFFFFF
     crc_t                  crc;                 // CRC
} nvmFooterCrc;

// NVM structure configuration
typedef struct {
    uint8_t * const         addressRamMirror;    // Pointer to base address of the RAM mirror (pointer RO, data RW)
    const uint8_t * const   addressRomDefault;   // Pointer to base address of the ROM defaults (pointer RO, data RO)
    uint8_t * const         addressCrc;          // Pointer to crc address of the RAM mirror (pointer RO, data RW)

    const uint32_t          length;              // Length (in bytes) of the structure excluding the CRC

    const bool              rewriteWhenCorrupt;  // Allow defaults to be restored when NVM structure is corrupt
} nvmStructureConfig;


/**
    Initialise the NVM module.
    Include initialisation of the RAM mirrors and recovery to defaults where allowed.
*/
void nvmInit(void);


#endif
