
#include "./minecraft.hpp"


// There's probably a better way to do this...
std::string mc::_get_hex(u32 n)
{
	std::stringstream hex, ret;
	hex << std::uppercase << std::hex << n;

	if(hex.str().size() == 1)
	{
		ret << "0000000" << hex.str();
	}

	else
	{
		ret << "000000" << hex.str();
	}

	return ret.str();
}


/**
 * The DRM goes as follows: 
 * 1) Ensure the options.txt file is 128KiB
 * 2) Ensure the header = the amount of data in the file
 * 	excluding the header and trailing 0x00 (4 bytes)
 * 3) Ensure all purchased content is stored in the
 * 	purchased_items key in this format:
 * 	"${GAME_PROD_ID}${CONTENT_ID}"
 * 	All purchased content must be seperated with comma's
 * 4) The latest update must be installed, this contains
 * 	All the actual dlc content
 * 5) The dlc must actually be installed. Athough it doesn't contain data,
 * 	the game checks if it is installed
 * 
 * What we're doing here is just bruteforcing every content ID
 * from 0x00 - 0xFF because the game doesn't check for extra IDs
 **/
bool mc::patch_in_mem(u8 *chars, u8 *ret, mc::Region reg)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u32 size = REVERSE_MERGE(chars);
#else
	u32 size = NORMAL_MERGE(chars);
#endif

	bool atValue = false;
	bool valid = false;
	std::string kbuf;

	u32 ns = size;


	// Skip the header
	for(u32 i = 4, wl = 4; i < size + 4; ++i, ++wl)
	{
		// :
		if(chars[i] == 0x3a)
		{
			atValue = true;
			if(kbuf == PATCH_FIELD)
			{
				std::string newk = BUILD_PART(0x0, reg);
				for(u32 i = 0x1; i < 0xFF; ++i)
				{
					newk += "," + BUILD_PART(i, reg);
				}

				// layout in ret is now
				// :<..ids...>\n
				// Layout in chars is now
				// :<...old user ids... (0+)>\n
				// Cut every old user id.
				for(u32 j = 0; i < size + 3; ++i, ++j)
				{
					// 0x0a = \n
					if(chars[i + 1] == 0x0a)
					{
						ns -= j;
						break;
					}
				}

				// 0x3a = :
				ret[wl] = 0x3a;
				for(size_t j = 0; j < newk.size(); ++j, ++wl)
					ret[wl + 1] = newk[j];
				ns += newk.size();

				u8 *wsize = (u8 *) &ns;


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
				ret[0] = wsize[0];
				ret[1] = wsize[1];
				ret[2] = wsize[2];
				ret[3] = wsize[3];
#else
				ret[3] = wsize[0];
				ret[2] = wsize[1];
				ret[1] = wsize[2];
				ret[0] = wsize[3];
#endif


				valid = true;
				continue;
			}

			kbuf.clear();
		}

		else if(chars[i] == 0x0a)
		{
			atValue = false;
		}

		else if(!atValue)
		{
			kbuf.push_back(chars[i]);
		}

		ret[wl] = chars[i];
	}

	return valid;
}
