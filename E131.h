#ifndef E131_H_
#define E131_H_

#define E131_DEFAULT_PORT 5568

//Copied from https://github.com/Ulto/K64F-E131/blob/1d7de16acdc27aa035950e8220a2c8b20fe76398/sources/E131.h
/* E1.31 Packet Structure */
typedef union {
	struct {
		/* Root Layer */
		uint16_t preamble_size;
		uint16_t postamble_size;
		uint8_t  acn_id[12];
		uint16_t root_flength;
		uint32_t root_vector;
		uint8_t  cid[16];

		/* Frame Layer */
		uint16_t frame_flength;
		uint32_t frame_vector;
		uint8_t  source_name[64];
		uint8_t  priority;
		uint16_t reserved;
		uint8_t  sequence_number;
		uint8_t  options;
		uint16_t universe;

		/* DMP Layer */
		uint16_t dmp_flength;
		uint8_t  dmp_vector;
		uint8_t  type;
		uint16_t first_address;
		uint16_t address_increment;
		uint16_t property_value_count;
		uint8_t  property_values[513];
	} __attribute__((packed));

	uint8_t raw[638];
} e131_packet_t;

#endif /* E131_H_ */

