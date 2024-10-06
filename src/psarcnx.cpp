#include "psarcnx.hpp"
#include "bstream.h"

#include <vector>
#include <sstream>
#include <iostream>

constexpr uint32_t PSARC_NX_MAGIC     = 0x4B4C524D;
constexpr uint32_t CONST_HEADER_SIZE  = 0x00000018;
constexpr uint32_t FILE_ENTRY_SIZE    = 0x00000008;
constexpr char     STRING_DELIMITER[] = { '\r', '\n' };
constexpr char     PSARC_EXT[]        = { ".psarc" };

PSARC_NX::~PSARC_NX()
{
	ClearArchive();
} // PSARC_NX::~PSARC_NX

bool PSARC_NX::AddFile(const std::string& name, uint8_t* data, size_t size)
{
	if (mFiles.find(name) == mFiles.end())
	{
		mFiles[name] = FileSizePair(data, size);
		return true;
	}

	return false;
} // PSARC_NX::AddFile

bool PSARC_NX::RemoveFile(const std::string& name)
{
	if (mFiles.find(name) != mFiles.end())
	{
		delete[] mFiles[name].first;
		mFiles[name].second = 0;

		mFiles.erase(name);
		return true;
	}

	return false;
} // PSARC_NX::RemoveFile

void PSARC_NX::GetFile(const std::string& name, FileSizePair& fileData)
{
	if (mFiles.find(name) != mFiles.end())
	{
		fileData = mFiles[name];
	}

	fileData = FileSizePair(nullptr, 0);
} // PSARC_NX::GetStream

bool PSARC_NX::LoadArchive(std::filesystem::path filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		return false;
	}

	mSourcePath = filePath;
	bStream::CFileStream stream(filePath.generic_string(), bStream::Little, bStream::In);

	return LoadArchive(stream);
} // PSARC_NX::LoadArchive

bool PSARC_NX::SaveArchive(std::filesystem::path filePath)
{
	if (!filePath.has_extension() || filePath.extension() != PSARC_EXT)
	{
		filePath.replace_extension(PSARC_EXT);
	}

	bStream::CFileStream stream(filePath.generic_string(), bStream::Little, bStream::Out);
	return SaveArchive(stream);
} // PSARC_NX::SaveArchive

bool PSARC_NX::LoadArchive(bStream::CStream& stream)
{
	if (stream.readUInt32() != PSARC_NX_MAGIC)        // 0x0000
	{
		return false;
	}

	uint32_t unkValue = stream.readUInt32();
	assert(unkValue == 0);                 // 0x0004

	std::cout << stream.tell() << std::endl;

	uint32_t headerSize = stream.readUInt32();        // 0x0008
	uint32_t fileCount = stream.readUInt32();         // 0x000C
	uint32_t stringTableOffset = stream.readUInt32(); // 0x0010
	uint32_t stringTableSize = stream.readUInt32();   // 0x0014

	uint32_t* fileOffsets = new uint32_t[fileCount];
	uint32_t* fileSizes   = new uint32_t[fileCount];

	for (uint32_t i = 0; i < fileCount; i++)
	{
		fileOffsets[i] = stream.readUInt32();
		fileSizes[i]   = stream.readUInt32();
	}

	stream.seek(stringTableOffset);

	for (uint32_t i = 0; i < fileCount; i++)
	{
		std::vector<char> nameVec;

		char nameChar = (char)stream.readInt8();
		while (nameChar != 0x0D)
		{
			nameVec.push_back(nameChar);
			nameChar = (char)stream.readInt8();
		}
		nameVec.push_back('\0');

		std::string fileName = std::string(nameVec.data());
		uint32_t fileSize = fileSizes[i];
		uint8_t* fileData = new uint8_t[fileSize];

		stream.skip(1);
		size_t nextNameOffset = stream.tell();

		stream.seek(fileOffsets[i]);
		stream.readBytesTo(fileData, fileSizes[i]);
		stream.seek(nextNameOffset);

		mFiles[fileName] = FileSizePair(fileData, fileSize);
	}

	delete[] fileOffsets;
	delete[] fileSizes;

	return true;
} // PSARC_NX::LoadArchive

bool PSARC_NX::SaveArchive(bStream::CStream& stream)
{
	if (mFiles.empty())
	{
		return false;
	}

	uint32_t fileCount = (uint32_t)mFiles.size();
	uint32_t fullHeaderSize = CONST_HEADER_SIZE + FILE_ENTRY_SIZE * fileCount;

	stream.writeUInt32(PSARC_NX_MAGIC); // 0x0000
	stream.writeUInt32(0);              // 0x0004
	stream.writeUInt32(fullHeaderSize); // 0x0008
	stream.writeUInt32(fileCount);      // 0x000C

	stream.writeUInt32(fullHeaderSize); // 0x0010
	stream.writeUInt32(0);              // 0x0014

	std::stringstream stringTableBuilder;

	// Build string table and write file entries (data offset + file size)
	for (auto p : mFiles)
	{
		stringTableBuilder << p.first;
		stringTableBuilder << STRING_DELIMITER;

		stream.writeUInt32(0);
		stream.writeUInt32((uint32_t)p.second.second);
	}

	std::string stringTable = stringTableBuilder.str();

	size_t curStreamPos = stream.tell();
	stream.seek(0x0014);

	// Write string table size
	stream.writeUInt32((uint32_t)stringTable.length());
	stream.seek(curStreamPos);

	// Write string table
	stream.writeString(stringTable);

	uint32_t fileIndex = 0;

	// Write file data and fill in the offsets in the file entries
	for (auto p : mFiles)
	{
		curStreamPos = stream.tell();

		stream.seek(CONST_HEADER_SIZE + FILE_ENTRY_SIZE * fileIndex);
		fileIndex++;

		stream.writeUInt32((uint32_t)curStreamPos);
		stream.seek(curStreamPos);

		stream.writeBytes(p.second.first, p.second.second);
	}

	return true;
} // PSARC_NX::SaveArchive

bool PSARC_NX::DumpArchive(std::filesystem::path dirPath)
{
	if (mFiles.size() == 0 || (dirPath.empty() && mSourcePath.empty()))
	{
		return false;
	}

	if (dirPath.empty())
	{
		dirPath = mSourcePath.parent_path() / mSourcePath.stem();
		if (!std::filesystem::exists(dirPath))
		{
			std::filesystem::create_directory(dirPath);
		}
	}

	for (auto p : mFiles)
	{
		std::string fileName = p.first.substr(0, p.first.find('.'));
		std::string fileExt = p.first.substr(p.first.find('.'), 4);

		std::filesystem::path filePath = dirPath / fileName;
		filePath.replace_extension(fileExt);

		bStream::CFileStream stream(filePath.generic_string(), bStream::Little, bStream::Out);
		stream.writeBytes(p.second.first, p.second.second);
	}

	return true;
} // PSARC_NX::DumpArchive

bool PSARC_NX::FillArchive(std::filesystem::path dirPath)
{
	if (!std::filesystem::exists(dirPath))
	{
		return false;
	}

	for (const auto& d : std::filesystem::directory_iterator(dirPath))
	{
		if (!d.is_regular_file())
		{
			continue;
		}

		std::filesystem::path filePath = d.path();

		uint32_t fileSize = (uint32_t)d.file_size();
		uint8_t* fileData = new uint8_t[d.file_size()];

		bStream::CFileStream fileStream(filePath.generic_string(), bStream::Little, bStream::In);
		fileStream.readBytesTo(fileData, fileSize);

		std::string fileName = filePath.filename().generic_string();
		mFiles[fileName] = FileSizePair(fileData, fileSize);
	}

	mSourcePath = dirPath;
	return true;
} // PSARC_NX::FillArchive

void PSARC_NX::ClearArchive()
{
	for (auto p : mFiles)
	{
		if (p.second.first != nullptr)
		{
			delete[] p.second.first;
			p.second.second = 0;
		}
	}

	mFiles.clear();
} // PSARC_NX::ClearArchive
