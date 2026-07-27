#if defined(USE_MAVLINK)  || defined(USE_MAVLINKPX4)

#ifdef MAVLINK_CONFIG


void mavlink_return_packet(uint8_t id, uint8_t len, uint8_t crc);

#pragma pack(push,1)
typedef struct MAV_conf { // needs to fit in 253 bytes
    uint16_t gap; // frame number
    uint8_t magick[4]; // = 0xee 'O' 'S' 'D'
    uint8_t cmd;		// command
    uint8_t id;		// number of 128-bytes block
    uint8_t len;		// real length
    uint8_t data[128];
    uint8_t crc; 		// may be
} Mav_conf;
#pragma pack(pop)

bool parse_osd_packet(uint8_t *p){
    Mav_conf *c = (Mav_conf *)p;

    if(c->magick[0] == 0xEE && c->magick[1] == 'O' && c->magick[2] == 'S' && c->magick[3] == 'D') {
	switch(c->cmd){
	case 'w':
	    eeprom_write_len((uint8_t *)&c->data, (uint16_t)(c->id) * 128,  c->len );
	    lflags.was_mav_config=1;
            // confirm on ALL hardware (was HARDWARE_TYPE>0 only): without the
            // '!' ack the configurator writes blind and cannot verify/retry
            c->cmd='!'; // confirm
            mavlink_return_packet(MAVLINK_MSG_ID_ENCAPSULATED_DATA, MAVLINK_MSG_ID_ENCAPSULATED_DATA_LEN, MAVLINK_MSG_ID_ENCAPSULATED_DATA_CRC); // send packet back
	    return true;
	
	
	case 'b':
	    if(c->len==0 && lflags.was_mav_config) {
#if defined(SLAVE_BUILD)
// just reload new values
                readSettings();

#else 
	        __asm__ __volatile__ (    // Jump to RST vector
	            "clr r30\n"
	            "clr r31\n"
	            "ijmp\n"
	        );
#endif
	    }
	    return true;
#ifdef MAVLINK_READ_EEPROM
        case 'r':
            eeprom_read_len((uint8_t *)&c->data, (uint16_t)(c->id) * 128,  c->len );

            mavlink_return_packet(MAVLINK_MSG_ID_ENCAPSULATED_DATA, MAVLINK_MSG_ID_ENCAPSULATED_DATA_LEN, MAVLINK_MSG_ID_ENCAPSULATED_DATA_CRC); // send packet back
            return true;
            
#endif

 #ifdef MAVLINK_FONT_UPLOAD
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align" // yes I know

        case 'f': { // font via MAVlink (now on ALL hardware - was HARDWARE_TYPE>0,
                    // which made the configurator's charset update time out on the 328)
            uint8_t s = SREG;
            cli(); // the VSYNC ISR also drives SPI - a collision mid-NVM-write corrupts the char.
                   // ~12ms with interrupts off is safe here: the CT waits for this ack before sending more.
            osd.write_NVM(*((uint16_t *)(&c->data)), (uint8_t *)(&c->data)+2); // first 2 byte is number, all another is bitmap
            SREG = s;
            c->cmd='!'; // confirm
            mavlink_return_packet(MAVLINK_MSG_ID_ENCAPSULATED_DATA, MAVLINK_MSG_ID_ENCAPSULATED_DATA_LEN, MAVLINK_MSG_ID_ENCAPSULATED_DATA_CRC); // send packet back
            } return true;

#pragma GCC diagnostic pop

 #endif

	default:
	    break;
	}
    }

    return false;
}

// sending in manual mode - calculate CRC and send to UART

/*
    uint16_t checksum; /// sent at end of packet
    uint8_t magic;   ///< protocol magic marker
    uint8_t len;     ///< Length of payload
    uint8_t seq;     ///< Sequence of packet
    uint8_t sysid;   ///< ID of message sender system/aircraft
    uint8_t compid;  ///< ID of the message sender component
    uint8_t msgid;   ///< ID of message in payload
    uint64_t payload64[(MAVLINK_MAX_PAYLOAD_LEN+MAVLINK_NUM_CHECKSUM_BYTES+7)/8];
*/
void mavlink_return_packet(uint8_t id, uint8_t len, uint8_t crc) {
    uint16_t checksum;
    
    // Reply in MAVLink1 framing: the configurator's parser is v1-only, and this
    // reply exists solely for it (config/EEPROM reads). The firmware's own RX
    // path stays MAVLink2-capable - v1 is a legal frame on the same link.
    uint8_t hdr[6];
    hdr[0] = MAVLINK_STX_MAVLINK1; // 0xFE
    hdr[1] = len;
    hdr[2] = msgbuf.m.seq;         // echo the request's sequence
    hdr[3] = mavlink_system.sysid;
    hdr[4] = MAV_COMP_ID_CAMERA;   // stole this id
    hdr[5] = id;

    checksum = crc_calculate(&hdr[1], 5);
    crc_accumulate_buffer(&checksum, (const char *)&msgbuf.m.payload64, len);
#if MAVLINK_CRC_EXTRA
    crc_accumulate(crc, &checksum);
#endif

    _mavlink_send_uart((mavlink_channel_t)0, (const char *)hdr, 6);
    _mavlink_send_uart((mavlink_channel_t)0, (const char *)&msgbuf.m.payload64, len );
    _mavlink_send_uart((mavlink_channel_t)0, (const char *)&checksum, 2);
}


#endif

#endif

