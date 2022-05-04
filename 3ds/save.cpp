
#include "save.hpp"

#include <iostream>


Result save::get_save(u64 tid, FS_Archive *arc)
{
	Result res;

	u32 tidlow = (u32)tid;
	u32 extdataId = tidlow >> 8;

	std::cout << "0x" << std::uppercase << std::hex << tidlow << std::endl;
	std::cout << "0x" << std::uppercase << std::hex << extdataId << std::endl;

	const u32 path[3] = { MEDIATYPE_SD, extdataId, 0 };

	res = FSUSER_OpenArchive(arc, ARCHIVE_EXTDATA, { PATH_BINARY, 12, path });
	return res;
}

Result save::open_file(FS_Archive arc, Handle *out, std::u16string fname)
{
	Result res;

	res = FSUSER_OpenFile(out, arc, fsMakePath(PATH_UTF16, fname.data()), FS_OPEN_WRITE, 0);

	return res;
}

Result save::backup_save(Handle in, u32 size, Handle out)
{
	Result res;
	u32 read;
	u8 *buf = new u8[size];

	res = FSFILE_Read(in, &read, 0, buf, size);
	if (R_FAILED(res))
	{
		return res;
	}

	res = FSFILE_Write(out, &read, 0, buf, size, 0);
	if (R_FAILED(res))
	{
		return res;
	}

	FSFILE_Close(out); //because we don't need this to be open after baccup
	delete [] buf;
	return res;
}

