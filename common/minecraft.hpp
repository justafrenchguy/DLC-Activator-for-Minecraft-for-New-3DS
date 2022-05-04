
#pragma once

#include <iostream>
#include <cstdint>
#include <sstream>
#include <string>


typedef std::uint32_t u32;
typedef std::uint16_t u16;
typedef std::uint8_t u8;


#define REVERSE_MERGE(x) (x[3] << 24 | x[2] << 16 | x[1] << 8 | x[0] << 0)
#define NORMAL_MERGE(x) (x[0] << 24 | x[1] << 16 | x[2] << 8 | x[3] << 0)

#define PATCH_FIELD "purchased_items"
#define SAVE_SIZE (1024 * 128)
#define REG_EUR 'P'
#define REG_JPN 'J'
#define REG_USA 'E'

#define BUILD_PART(id,reg) (std::string("CTRMBD3") + (reg == mc::Region::eur ? \
	REG_EUR : reg == mc::Region::jpn ? REG_JPN : REG_USA) + mc::_get_hex(id))


namespace mc
{
	enum class Region
	{
		eur = 0x1, 
		usa = 0x2, 
		jpn = 0x4,
	};

	bool patch_in_mem(u8 *chars, u8 *ret, mc::Region reg);
	std::string _get_hex(u32 n);
}
