
#include <minecraft.hpp>
#include "save.hpp"

#include <iostream>
#include <cstring>

// no header go brrr :chaos:

#define PRINT_ERROR(msg,res) std::cout << msg << " (0x" << std::uppercase << std::hex << res << ")" << std::endl


int search_tids_count(u64 *tids, u64 *query, u32 titlecount);
bool search_tids(u64 *tids, u64 *query, u32 titleCount);
void add_region_string(char *str, mc::Region region);
u64 get_base_tid(mc::Region region);
void patch_region(mc::Region region);
u32 get_regions();

#define EUR_BASE 0x000400000017CA00
#define USA_BASE 0x00040000001B8700
#define JPN_BASE 0x000400000017FD00

//								base game		   update data		  "dlc" title
static u64 EUR_TIDS[3] = { EUR_BASE, 0x0004000E0017CA00, 0x0004008C0017CA00 };
static u64 USA_TIDS[3] = { USA_BASE, 0x0004000E001B8700, 0x0004008C001B8700 };
static u64 JPN_TIDS[3] = { JPN_BASE, 0x0004000E0017FD00, 0x0004008C0017FD00 };
#define REGION_TID_AMOUNT 3


std::u16string to_u16_string(const char *src)
{
	char16_t tmp[256] = { 0 };
	utf8_to_utf16((u16 *) tmp, (u8 *) src, 256);
	return std::u16string(tmp);
}

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, nullptr);
	fsInit();
	amInit();

	u32 regionFlag = get_regions();

	if (regionFlag == 0)
	{
		std::cout << "The base game, update or dlc is not installed. Please install the the required titles and try again." << std::endl;
	}
	else
	{
		if (regionFlag & (u32)mc::Region::eur)
		{
			std::cout << "Patching EUR..." << std::endl;
			patch_region(mc::Region::eur);
		}

		if (regionFlag & (u32)mc::Region::usa)
		{
			std::cout << "Patching USA..." << std::endl;
			patch_region(mc::Region::usa);
		}

		if (regionFlag & (u32)mc::Region::jpn)
		{
			std::cout << "Patching JPN..." << std::endl;
			patch_region(mc::Region::jpn);
		}
	}

	std::cout << "Press START to exit." << std::endl;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	fsExit();
	gfxExit();
	amExit();
	return 0;
}

void patch_region(mc::Region region)
{
	Result res;
	FS_Archive arc;
	FS_Archive sdmc;
	Handle handle;
	Handle backup_file;

	res = save::get_save(get_base_tid(region), &arc);
	if (R_FAILED(res))
	{
		PRINT_ERROR("could not open extdata save archive.", res);
		return;
	}

	res = save::open_file(arc, &handle, to_u16_string("/options.txt"));
	if (R_FAILED(res))
	{
		PRINT_ERROR("Failed to open save file!", res);
		return;
	}
	std::cout << "Open file success!" << std::endl;


	// backup save file in case of failure.
	res = FSUSER_OpenArchive(&sdmc, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (R_FAILED(res))
	{
		PRINT_ERROR("Could not open SD Card for writing!", res);

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);

		return;
	}


	res = FSUSER_CreateDirectory(sdmc, fsMakePath(PATH_ASCII, "/friimc_savebackup"), 0);
	if (R_FAILED(res) && R_DESCRIPTION(res) != 190) // already exists
	{
		PRINT_ERROR("Could not create save file backup directory!", res);

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);
		FSUSER_CloseArchive(sdmc);

		return;
	}

	FSUSER_CloseArchive(sdmc);
	// backup dir created (if it didn't exist already) and openend, now reading the source save file into memory to use it

	u32 read;

	u8 *inbuf = new u8[SAVE_SIZE];
	u8 *outbuf = new u8[SAVE_SIZE];

	res = FSFILE_Read(handle, &read, 0, inbuf, SAVE_SIZE);

	// for(size_t i = 0; i < 100; ++i)
	// {
	// 	printf("%02X ", inbuf[i]);
	// }

	// wtf?
	// Tl;dl usa sucks come to europa
	// Long: for some reason usa copies of the
	// game ALWAYS fail with error code
	// 0xD900458B = RomFS or Savedata hash check failed
	// why? i have 0 fucking ideas because it still
	// reads although it gives the error
	if (R_FAILED(res) && res != (Result) 0xD900458B)
	{
		PRINT_ERROR("Failed to read from file!", res);

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);

		delete [] inbuf;
		delete [] outbuf;

		return;
	}

	std::cout << "Read file success." << std::endl;
	std::cout << "Read 0x" << std::hex << read << " bytes from file." << std::endl;

	// create save data backup from the copy that's loaded in memory

	char backupPath[26 /* prefix */ + 3 /* region */ + 4 /* .bin */ + 1 /* NULL term */] =  "/friimc_savebackup/backup_";
	add_region_string(backupPath, region);
	std::strcat(backupPath, ".bin\0");

	res = FSUSER_OpenFileDirectly(&backup_file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, backupPath), FS_OPEN_WRITE | FS_OPEN_CREATE, 0);

	if (R_FAILED(res))
	{
		PRINT_ERROR("Could not create backup save data file!", res);

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);

		delete [] inbuf;
		delete [] outbuf;

		return;
	}

	res = FSFILE_Write(backup_file, &read, 0, inbuf, SAVE_SIZE, 0);

	if (R_FAILED(res))
	{
		PRINT_ERROR("Could not write to backup file!", res);

		FSFILE_Close(handle);
		FSFILE_Close(backup_file);
		FSUSER_CloseArchive(arc);

		delete [] inbuf;
		delete [] outbuf;

		return;
	}

	FSFILE_Close(backup_file);
	std::cout << "Wrote backup file to " << backupPath << std::endl;
	std::cout << "Patching save file in memory." << std::endl;

	if(!mc::patch_in_mem(inbuf, outbuf, region))
	{
		std::cout << "Failed patching file in memory, aborting region..." << std::endl;

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);

		delete [] inbuf;
		delete [] outbuf;

		return;
	}

	std::cout << "Patched save file in memory, writing to save file on disk..." << std::endl;

	res = FSFILE_Write(handle, &read, 0, outbuf, SAVE_SIZE, 0);

	if (R_FAILED(res))
	{
		PRINT_ERROR("failed to write patched save data!", res);

		FSFILE_Close(handle);
		FSUSER_CloseArchive(arc);

		delete [] inbuf;
		delete [] outbuf;

		return;
	}

	std::cout << "patched save was successfully written!" << std::endl;

	FSFILE_Close(handle);
	FSUSER_CloseArchive(arc);

	delete [] inbuf;
	delete [] outbuf;
}

u32 get_regions()
{
	u32 outres = 0;
	u32 tcount, read;
	Result res;

	res = AM_GetTitleCount(MEDIATYPE_SD, &tcount);

	if (R_FAILED(res))
	{
		PRINT_ERROR("Failed to query SD Titles.", res);
		return false;
	}
	else
	{
		std::cout << "Queried " << std::to_string(tcount) << " SD Titles." << std::endl;
	}

	u64 *tids = new u64[tcount];
	res = AM_GetTitleList(&read, MEDIATYPE_SD, tcount, tids);

	if (R_FAILED(res))
	{
		PRINT_ERROR("Failed to get list SD Titles' IDs.", res);
		return false;
	}
	else
	{
		std::cout << "Queried " << std::to_string(tcount) << " SD Title IDs." << std::endl;
	}

	if (search_tids(tids, EUR_TIDS, tcount))
	{
		outres |= (u32)mc::Region::eur;
		std::cout << "Found European copy of Minecraft!" << std::endl;
	}

	if (search_tids(tids, USA_TIDS, tcount))
	{
		outres |= (u32)mc::Region::usa;
		std::cout << "Found North American copy of Minecraft!" << std::endl;
	}

	if (search_tids(tids, JPN_TIDS, tcount))
	{
		outres |= (u32)mc::Region::jpn;
		std::cout << "Found Japanese copy of Minecraft!" << std::endl;
	}

	/* Check what tid we have on the cart */
	u32 cardtc;
	res = AM_GetTitleCount(MEDIATYPE_GAME_CARD, &cardtc);
	if(R_FAILED(res))
	{
		PRINT_ERROR("NONFATAL: Failed reading card slot, is it broken?", res);
	}

	/* There is a cart inserted */
	else if(cardtc == 1)
	{
		u64 cardtid;
		res = AM_GetTitleList(&read, MEDIATYPE_GAME_CARD, cardtc, &cardtid);
		if(R_FAILED(res))
		{
			PRINT_ERROR("Couldn't read title id from card?", res);
			goto failcard;
		}

		std::cout << "Found title id in card slot: " << std::hex << cardtid << std::endl;

		// We check for 2 here because only dlc and update need to be installed in this case
		switch(cardtid)
		{
		case USA_BASE:
			std::cout << "Found USA copy in card slot!" << std::endl;
			outres |= search_tids_count(tids, USA_TIDS, tcount) == 2 ? (u32) mc::Region::usa : 0;
			break;
		case JPN_BASE:
			std::cout << "Found JPN copy in card slot!" << std::endl;
			outres |= search_tids_count(tids, JPN_TIDS, tcount) == 2 ? (u32) mc::Region::jpn : 0;
			break;
		case EUR_BASE:
			std::cout << "Found EUR copy in card slot!" << std::endl;
			outres |= search_tids_count(tids, EUR_TIDS, tcount) == 2 ? (u32) mc::Region::eur : 0;
			break;
		default:
			std::cout << "Irrelevant card in slot, ignoring." << std::endl;
		}
	}

failcard:


	delete [] tids;
	return outres;
}

int search_tids_count(u64 *tids, u64 *query, u32 titleCount)
{
	u32 found = 0;

	for (u32 i = 0; i < REGION_TID_AMOUNT; i++)
	{
		for (u32 j = 0; j < titleCount; j++)
		{
			if (tids[j] == query[i])
			{
				found++;
			}
		}
	}

	return found;
}

bool search_tids(u64 *tids, u64 *query, u32 titleCount)
{
	return search_tids_count(tids, query, titleCount) == REGION_TID_AMOUNT;
}

u64 get_base_tid(mc::Region region)
{
	u64 out = 0;

	switch(region)
	{
		case mc::Region::eur:
			out = EUR_TIDS[0];
			break;
		case mc::Region::usa:
			out = USA_TIDS[0];
			break;
		case mc::Region::jpn:
			out = JPN_TIDS[0];
			break;
	}

	return out;
}

void add_region_string(char *str, mc::Region region)
{
	switch (region)
	{
		case mc::Region::eur:
			std::strcat(str, "eur");
			break;
		case mc::Region::usa:
			std::strcat(str, "usa");
			break;
		case mc::Region::jpn:
			std::strcat(str, "jpn");
			break;
	}
}
