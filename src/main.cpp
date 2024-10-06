#include "psarcnx.hpp"

#include <iostream>
#include <filesystem>

void PrintHelpMessage();

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		PrintHelpMessage();
		return 0;
	}

	for (int i = 1; i < argc; i++)
	{
		std::filesystem::path path(argv[i]);

		if (!std::filesystem::exists(path))
		{
			continue;
		}
		
		PSARC_NX archive;

		if (std::filesystem::is_regular_file(path))
		{
			if (path.extension() != ".psarc")
			{
				continue;
			}

			if (!archive.LoadArchive(path))
			{
				std::cout << "Failed to load PSARC archive at " << path << "." << std::endl;
				continue;
			}

			if (!archive.DumpArchive())
			{
				std::cout << "Failed to dump contents of PSARC archive at " << path << "." << std::endl;
				continue;
			}
		}
		else if (std::filesystem::is_directory(path))
		{
			if (!archive.FillArchive(path))
			{
				std::cout << "Failed to get files from directory at " << path << "." << std::endl;
				continue;
			}

			if (!archive.SaveArchive(path))
			{
				std::cout << "Failed to save PSARC archive to " << path << "." << std::endl;
				continue;
			}
		}
	}
}

void PrintHelpMessage()
{
	std::cout << "PSARC-NX tool by Gamma/Sage of Mirrors." << std::endl;
	std::cout << "Provides packing and unpacking utilities for GUST's Nintendo Switch resource archives." << std::endl;
	std::cout << "(They have the .psarc extension, but they're not actually PSARC archives.)" << std::endl << std::endl;

	std::cout << "Usage:" << std::endl;
	std::cout << "Unpack a PSARC file into a directory: psarc-nx.exe path/to/psarc/file" << std::endl;
	std::cout << "Pack a directory into a PSARC file:   psarc-nx.exe path/to/directory" << std::endl;
}
