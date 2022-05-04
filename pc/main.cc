
#include <minecraft.hpp>
#include <iostream>
#include <cstring>
#include <cstdio>


void lowercase(std::string& str)
{
	for(size_t i = 0; i < str.size(); ++i)
		str[i] = std::tolower(str[i]);
}

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		std::cout
			<< "Invalid usage!\n" << argv[0]
			<< " <eur/jpn/usa> <input file> <output file>" << std::endl;
		return 1;
	}

	mc::Region region;

	// Shut
	std::string regstr = argv[1];
	lowercase(regstr);

	if(regstr == "eur")
		region = mc::Region::eur;
	else if(regstr == "usa")
		region = mc::Region::usa;
	else if(regstr == "jpn")
		region = mc::Region::jpn;
	else
	{
		std::cout << "Invalid region: " << regstr << std::endl;
		return 1;
	}


	u8 *chars = new u8[SAVE_SIZE];

	FILE *in = std::fopen(argv[2], "rb");
	if(in == NULL)
	{
		std::cout << "Error opening file \"" << argv[2] << "\" [" << errno << "]" << ": "
			<< std::strerror(errno) << std::endl;
		delete [] chars;
		return 2;
	}

	std::fread(chars, SAVE_SIZE, 1, in);
	std::fclose(in);

	u8 *res = new u8[SAVE_SIZE];
	if(!mc::patch_in_mem(chars, res, region))
	{
		std::cout << "Invalid minecraft save. try redumping your save." << std::endl;
		delete [] chars;
		delete [] res;
		return 4;
	};

	FILE *out = std::fopen(argv[3], "wb");
	if(out == NULL)
	{
		std::cout << "Error opening file \"" << argv[3] << "\" [" << errno << "]" << ": "
			<< std::strerror(errno) << std::endl;
		delete [] chars;
		delete [] res;
		return 3;
	}

	std::fwrite(res, SAVE_SIZE, 1, out);
	std::fclose(out);

	std::cout << argv[1] << " -> " << argv[2] << std::endl;

	delete [] chars;
	delete [] res;
	return 0;
}
