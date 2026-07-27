/** @file
 *  @brief MAVLink comm protocol - uAvionix dialect (minimal).
 *
 *  The upstream v2.0 header snapshot bundled in this repository referenced this
 *  sub-dialect from ardupilotmega.h (#include "../uAvionix/uAvionix.h") but the
 *  file itself was never included in the snapshot, which broke the v2.0 build.
 *
 *  This OSD firmware only *receives* standard common/ardupilotmega telemetry and
 *  never uses the uAvionix ADS-B-out messages. The message CRC/length table in
 *  ardupilotmega.h (MAVLINK_MESSAGE_CRCS) is self-contained, and MAVLINK_MESSAGE_INFO
 *  is only expanded when MAVLINK_USE_MESSAGE_INFO is defined (it is not here). So the
 *  only requirement is that this header exist and be self-consistent.
 *
 *  We therefore provide the message ids, lengths, CRC-extras and payload structs
 *  (matching the entries already hard-coded in ardupilotmega.h) but intentionally
 *  omit the pack/encode/decode/send helper functions - those are unused and require
 *  newer mavlink_helpers APIs than this vintage of the library provides.
 */
#ifndef MAVLINK_UAVIONIX_H
#define MAVLINK_UAVIONIX_H

#ifndef MAVLINK_H
    #error Wrong include order: MAVLINK_UAVIONIX.H MUST NOT BE DIRECTLY USED. Include mavlink.h from the same directory instead.
#endif

// MESSAGE UAVIONIX_ADSB_OUT_CFG
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_CFG 10001
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_CFG_LEN 20
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_CFG_MIN_LEN 20
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_CFG_CRC 209

typedef struct __mavlink_uavionix_adsb_out_cfg_t {
 uint32_t ICAO; /*<  Vehicle address (24 bit)*/
 uint16_t stallSpeed; /*< [cm/s] Aircraft stall speed in cm/s*/
 char callsign[9]; /*<  Vehicle identifier (8 characters, null terminated)*/
 uint8_t emitterType; /*<  Transmitting vehicle type. See ADSB_EMITTER_TYPE enum*/
 uint8_t aircraftSize; /*<  Aircraft length and width encoding (table 2-35 of DO-282B)*/
 uint8_t gpsOffsetLat; /*<  GPS antenna lateral offset (table 2-36 of DO-282B)*/
 uint8_t gpsOffsetLon; /*<  GPS antenna longitudinal offset from nose (table 2-37 DO-282B)*/
 uint8_t rfSelect; /*<  ADS-B transponder receiver and transmit enable flags*/
} mavlink_uavionix_adsb_out_cfg_t;

// MESSAGE UAVIONIX_ADSB_OUT_DYNAMIC
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_DYNAMIC 10002
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_DYNAMIC_LEN 41
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_DYNAMIC_MIN_LEN 41
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_OUT_DYNAMIC_CRC 186

typedef struct __mavlink_uavionix_adsb_out_dynamic_t {
 uint32_t utcTime; /*< [s] UTC time in seconds since GPS epoch. If unknown set to UINT32_MAX*/
 int32_t gpsLat; /*< [degE7] Latitude WGS84 (deg * 1E7). If unknown set to INT32_MAX*/
 int32_t gpsLon; /*< [degE7] Longitude WGS84 (deg * 1E7). If unknown set to INT32_MAX*/
 int32_t gpsAlt; /*< [mm] Altitude (WGS84). UP +ve. If unknown set to INT32_MAX*/
 int32_t baroAltMSL; /*< [mbar] Barometric pressure altitude (MSL). If unknown set to INT32_MAX*/
 uint32_t accuracyHor; /*< [mm] Horizontal accuracy in mm. If unknown set to UINT32_MAX*/
 uint16_t accuracyVert; /*< [cm] Vertical accuracy in cm. If unknown set to UINT16_MAX*/
 uint16_t accuracyVel; /*< [mm/s] Velocity accuracy in mm/s. If unknown set to UINT16_MAX*/
 int16_t velVert; /*< [cm/s] GPS vertical speed in cm/s. If unknown set to INT16_MAX*/
 int16_t velNS; /*< [cm/s] North-South velocity over ground in cm/s North +ve. If unknown set to INT16_MAX*/
 int16_t VelEW; /*< [cm/s] East-West velocity over ground in cm/s East +ve. If unknown set to INT16_MAX*/
 uint16_t state; /*<  ADS-B transponder dynamic input state flags*/
 uint16_t squawk; /*<  Mode A code (typically 1200 [0x04B0] for VFR)*/
 uint8_t gpsFix; /*<  0-1: no fix, 2: 2D fix, 3: 3D fix, 4: DGPS, 5: RTK*/
 uint8_t numSats; /*<  Number of satellites visible. If unknown set to UINT8_MAX*/
 uint8_t emergencyStatus; /*<  Emergency status*/
} mavlink_uavionix_adsb_out_dynamic_t;

// MESSAGE UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT 10003
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT_LEN 1
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT_MIN_LEN 1
#define MAVLINK_MSG_ID_UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT_CRC 4

typedef struct __mavlink_uavionix_adsb_transceiver_health_report_t {
 uint8_t rfHealth; /*<  ADS-B transponder messages*/
} mavlink_uavionix_adsb_transceiver_health_report_t;

#endif // MAVLINK_UAVIONIX_H
