
#pragma once

#include <minecraft.hpp>
#include <3ds.h>


namespace save
{
	Result open_file(FS_Archive arc, Handle *out, std::u16string fname);
	Result backup_save(Handle in, u32 size, Handle out);
	Result get_save(u64 tid, FS_Archive *arc);
}
