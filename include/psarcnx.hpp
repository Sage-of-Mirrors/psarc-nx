#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <filesystem>

namespace bStream
{
	class CStream;
}

using FileSizePair = std::pair<uint8_t*, size_t>;

class PSARC_NX
{
	std::filesystem::path mSourcePath;
	std::map<std::string, FileSizePair> mFiles;

public:
	PSARC_NX() : mSourcePath("") { }
	virtual ~PSARC_NX();

	// Gets the file data pointer and data size with the given name.
	void GetFile(const std::string& name, FileSizePair& fileData);

	// Adds a file to the archive with the given name, data, and size.
	bool AddFile(const std::string& name, uint8_t* data, size_t size);
	// Removes the file with the given name from the archive.
	bool RemoveFile(const std::string& name);

	// Loads the archive data from the file at the given path.
	bool LoadArchive(std::filesystem::path filePath);
	// Saves the archive data to a file at the given path.
	bool SaveArchive(std::filesystem::path filePath);

	// Loads the archive data from the given data stream.
	bool LoadArchive(bStream::CStream& stream);
	// Saves the archive data to the given data stream.
	bool SaveArchive(bStream::CStream& stream);

	// Saves the files in the archive to the given directory.
	bool DumpArchive(std::filesystem::path dirPath = "");
	// Loads the files from the given directory into the archive.
	bool FillArchive(std::filesystem::path dirPath);
	
	// Removes all files from the archive.
	void ClearArchive();
};
