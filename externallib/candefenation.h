#ifndef CAN_DEFS_H
#define CAN_DEFS_H

#include <stdint.h>

// CAN Frame Header
struct can_frame {
    uint32_t can_id;    // 32-bit CAN ID
    uint8_t can_dlc;    // Data Length Code (DLC)
    uint8_t data[8];    // Data payload (up to 8 bytes)
};

// CAN ID Masks (from socketcan standards)
#define CAN_SFF_MASK     0x000007FFU  // Standard Frame Format mask
#define CAN_EFF_MASK     0x1FFFFFFFU  // Extended Frame Format mask

// CAN Frame Types
#define CAN_EFF_FLAG     0x80000000U  // Extended Frame Format flag
#define CAN_RTR_FLAG     0x40000000U  // Remote Transmission Request flag

// Macros to check CAN ID formats
#define CAN_IS_SFF(id)   (((id) & CAN_SFF_MASK) == (id))    // Check if the ID is standard format
#define CAN_IS_EFF(id)   (((id) & CAN_EFF_MASK) == (id))    // Check if the ID is extended format

// CAN ID Operations
#define CAN_EFF_ID(id)   ((id) & CAN_EFF_MASK)               // Extract the extended frame ID
#define CAN_SFF_ID(id)   ((id) & CAN_SFF_MASK)               // Extract the standard frame ID

// Error Handling
#define CAN_ERR_CRTL     0x20000000U  // Control Error (can be used to detect errors)
#define CAN_ERR_PROT     0x10000000U  // Protocol Error (for protocol-related issues)

// CAN Bus Baud Rates (example for setting up)
#define CAN_BAUD_125K    125000      // 125 kbps
#define CAN_BAUD_250K    250000      // 250 kbps
#define CAN_BAUD_500K    500000      // 500 kbps
#define CAN_BAUD_1M      1000000     // 1 Mbps

// CAN Error Types (example)
#define CAN_ERR_OK       0x00        // No error
#define CAN_ERR_TX_FAIL  0x01        // Transmission failed
#define CAN_ERR_BUSOFF   0x02        // Bus off condition

#endif // CAN_DEFS_H
