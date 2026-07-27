// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: t -*-

/// @file	GCS_MAVLink.h
/// @brief	One size fits all header for MAVLink integration.

#ifndef GCS_MAVLink_h
#define GCS_MAVLink_h

#define HardwareSerial_h
/*
// AVR Includes
#if HARDWARE_TYPE == 0
 #include <SingleSerial.h> // MUST be first
#elif HARDWARE_TYPE == 1
 #include <FastSerial.h> 
#endif
*/
#include "compat.h"

#include "Arduino.h"

//extern SingleSerial Serial;

// we have separate helpers disabled to make it possible
// to select MAVLink 1.0 in the arduino GUI build
//#define MAVLINK_SEPARATE_HELPERS

#define MAVLINK_COMM_NUM_CHANNELS 1

// Compile out MAVLink2 packet signing (SHA-256) - this OSD never configures
// signing keys, so the code could only ever take its "no signing" path while
// costing several KB of flash on the ATmega328. See mavlink_helpers.h stubs.
#define MAVLINK_NO_SIGNING 1

// Trim the MAVLink2 message CRC/length table (190 entries, ~1.7KB PROGMEM in
// the full ardupilotmega dialect) down to the messages this firmware actually
// receives, plus PARAM_VALUE (22, slave builds) and REQUEST_DATA_STREAM (66).
// Entries are verbatim rows of the full table in ardupilotmega.h and MUST stay
// sorted by msgid (mavlink_get_msg_entry uses a bisection search). Messages
// not listed simply fail CRC lookup and are dropped - same net effect as the
// parser's default case ignoring them.
#define MAVLINK_MESSAGE_CRCS {{0, 50, 9, 0, 0, 0}, {1, 124, 31, 0, 0, 0}, {2, 137, 12, 0, 0, 0}, {22, 220, 25, 0, 0, 0}, {24, 24, 30, 0, 0, 0}, {27, 144, 26, 0, 0, 0}, {29, 115, 14, 0, 0, 0}, {30, 39, 28, 0, 0, 0}, {33, 104, 28, 0, 0, 0}, {35, 244, 22, 0, 0, 0}, {36, 222, 21, 0, 0, 0}, {42, 28, 2, 0, 0, 0}, {62, 183, 26, 0, 0, 0}, {65, 118, 42, 0, 0, 0}, {66, 148, 6, 3, 2, 3}, {74, 20, 20, 0, 0, 0}, {109, 185, 9, 0, 0, 0}, {131, 223, 255, 0, 0, 0}, {137, 195, 14, 0, 0, 0}, {162, 189, 8, 0, 0, 0}, {166, 21, 9, 0, 0, 0}, {168, 1, 12, 0, 0, 0}, {181, 174, 4, 0, 0, 0}, {241, 90, 32, 0, 0, 0}, {246, 184, 38, 0, 0, 0}, {253, 83, 51, 0, 0, 0}}

#include "include/mavlink/v2.0/ardupilotmega/version.h"

#define MAVLINK_COMM_NUM_BUFFERS 1
#include "include/mavlink/v2.0/mavlink_types.h"

/// MAVLink stream used for HIL interaction
extern BetterStream	*mavlink_comm_0_port;

/// MAVLink stream used for ground control communication
extern BetterStream	*mavlink_comm_1_port;

/// MAVLink system definition
extern mavlink_system_t mavlink_system;

/// Send a byte to the nominated MAVLink channel
///
/// @param chan		Channel to send to
/// @param ch		Byte to send
///
static inline void comm_send_ch(mavlink_channel_t chan, uint8_t ch)
{
#if MAVLINK_COMM_NUM_CHANNELS>1
    switch(chan) {
	case MAVLINK_COMM_0:
		mavlink_comm_0_port->write(ch);
		break;
	case MAVLINK_COMM_1:
		mavlink_comm_1_port->write(ch);
		break;
	default:
		break;
	}
#else
	//mavlink_comm_0_port->write(ch);
	Serial.write_S(ch);
#endif
}

/// Read a byte from the nominated MAVLink channel
///
/// @param chan		Channel to receive on
/// @returns		Byte read
///
static inline uint8_t comm_receive_ch(mavlink_channel_t chan)
{
#if MAVLINK_COMM_NUM_CHANNELS>1
    uint8_t data = 0;

    switch(chan) {
	case MAVLINK_COMM_0:
		data = mavlink_comm_0_port->read();
		break;
	case MAVLINK_COMM_1:
		data = mavlink_comm_1_port->read();
		break;
	default:
		break;
	}
    return data;
#else
    //return mavlink_comm_0_port->read();
    return Serial.read_S();
#endif
}

/// Check for available data on the nominated MAVLink channel
///
/// @param chan		Channel to check
/// @returns		Number of bytes available
static inline uint16_t comm_get_available(mavlink_channel_t chan)
{
#if MAVLINK_COMM_NUM_CHANNELS>1
    uint16_t bytes = 0;
    switch(chan) {
	case MAVLINK_COMM_0:
		bytes = mavlink_comm_0_port->available();
		break;
	case MAVLINK_COMM_1:
		bytes = mavlink_comm_1_port->available();
		break;
	default:
		break;
	}
    return bytes;
#else
	//return mavlink_comm_0_port->available();
	return Serial.available_S();
#endif
}


/// Check for available transmit space on the nominated MAVLink channel
///
/// @param chan		Channel to check
/// @returns		Number of bytes available, -1 for error
static inline int comm_get_txspace(mavlink_channel_t chan)
{
#if MAVLINK_COMM_NUM_CHANNELS>1
    switch(chan) {
	case MAVLINK_COMM_0:
		return mavlink_comm_0_port->txspace();
		break;
	case MAVLINK_COMM_1:
		return mavlink_comm_1_port->txspace();
		break;
	default:
		break;
	}
    return -1;
#else
    return 1;
#endif
}

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#include "include/mavlink/v2.0/ardupilotmega/mavlink.h"

uint8_t mavlink_check_target(uint8_t sysid, uint8_t compid);

// return a MAVLink variable type given a AP_Param type
uint8_t mav_var_type(enum ap_var_type t);

#endif // GCS_MAVLink_h
