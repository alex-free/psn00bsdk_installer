#include "global.h"
#include "iso.h"

char rootname[] = { "<root>" };

static bool icompare_func(unsigned char a, unsigned char b)
{
	return std::tolower( a ) == std::tolower( b );
}

static bool icompare(const std::string& a, const std::string& b)
{
	if ( a.length() == b.length() )
	{
		return std::equal( b.begin(), b.end(), a.begin(), icompare_func );
	}
	else
	{
		return false;
	}
}

int iso::DirTreeClass::GetWavSize(const char* wavFile)
{
	FILE *fp;

	if ( !( fp = fopen( wavFile, "rb" ) ) )
	{
		printf("ERROR: File not found.\n");
		return false;
	}

	// Get header chunk
	struct
	{
		char	id[4];
		int		size;
		char	format[4];
	} HeaderChunk;

	fread( &HeaderChunk, 1, sizeof(HeaderChunk), fp );

	if ( memcmp( &HeaderChunk.id, "RIFF", 4 ) ||
		memcmp( &HeaderChunk.format, "WAVE", 4 ) )
	{
		// It must be a raw
		fseek(fp, 0, SEEK_END);
		int wavlen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		fclose( fp );
		return wavlen;
	}

	// Get header chunk
	struct
	{
		char	id[4];
		int		size;
		short	format;
		short	chan;
		int		freq;
		int		brate;
		short	balign;
		short	bps;
	} WAV_Subchunk1;

	fread( &WAV_Subchunk1, 1, sizeof(WAV_Subchunk1), fp );

	// Check if its a valid WAVE file
	if ( memcmp( &WAV_Subchunk1.id, "fmt ", 4 ) )
	{
		fclose( fp );
		return 0;
	}

	// Search for the data chunk
	struct
	{
		char	id[4];
		int		len;
	} WAV_Subchunk2;

	while ( 1 )
	{
		fread( &WAV_Subchunk2, 1, sizeof(WAV_Subchunk2), fp );

		if ( memcmp( &WAV_Subchunk2.id, "data", 4 ) )
		{
			fseek( fp, WAV_Subchunk2.len, SEEK_CUR );
		}
		else
		{
			break;
		}
	}

	fclose( fp );

	return 2352*((WAV_Subchunk2.len+2351)/2352);
}

int iso::DirTreeClass::PackWaveFile(cd::IsoWriter* writer, const char* wavFile, int pregap)
{
	FILE *fp;
	int waveLen;
	unsigned char buff[CD_SECTOR_SIZE];

	if ( !( fp = fopen( wavFile, "rb" ) ) )
	{
		printf("ERROR: File not found.\n");
		return false;
	}

	// Get header chunk
	struct
	{
		char	id[4];
		int		size;
		char	format[4];
	} HeaderChunk;

	fread( &HeaderChunk, 1, sizeof(HeaderChunk), fp );

	if ( memcmp( &HeaderChunk.id, "RIFF", 4 ) ||
		memcmp( &HeaderChunk.format, "WAVE", 4 ) )
	{

		// File must be a raw, pack it anyway
		memset(buff, 0x00, CD_SECTOR_SIZE);

		if ( pregap ) {

			// Write pregap region
			for ( int i=0; i<150; i++ ) {
				writer->WriteBytesRaw(buff, CD_SECTOR_SIZE);
			}

		}

		// Write data
		fseek(fp, 0, SEEK_END);
		waveLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		while ( waveLen > 0 )
		{
			memset(buff, 0x00, CD_SECTOR_SIZE);

			int readLen = waveLen;

			if (readLen > CD_SECTOR_SIZE)
			{
				readLen = CD_SECTOR_SIZE;
			}

			fread( buff, 1, readLen, fp );
			writer->WriteBytesRaw( buff, CD_SECTOR_SIZE );

			waveLen -= readLen;
		}

		printf("Packed as raw... ");

		fclose( fp );
		return true;
	}

	// Get header chunk
	struct
	{
		char	id[4];
		int		size;
		short	format;
		short	chan;
		int		freq;
		int		brate;
		short	balign;
		short	bps;
	} WAV_Subchunk1;

	fread( &WAV_Subchunk1, 1, sizeof(WAV_Subchunk1), fp );

	// Check if its a valid WAVE file
	if ( memcmp( &WAV_Subchunk1.id, "fmt ", 4 ) )
	{
		if ( !global::QuietMode )
		{
			printf( "\n    " );
		}

		printf( "ERROR: Unsupported WAV format.\n" );

		fclose( fp );
		return false;;
	}


    if ( (WAV_Subchunk1.chan != 2) || (WAV_Subchunk1.freq != 44100) ||
		(WAV_Subchunk1.bps != 16) )
	{
		if ( !global::QuietMode )
		{
			printf( "\n    " );
		}

		printf( "ERROR: Only 44.1KHz, 16-bit Stereo WAV files are supported.\n" );

		fclose( fp );
		return false;
    }

	// Search for the data chunk
	struct
	{
		char	id[4];
		int		len;
	} WAV_Subchunk2;

	while ( 1 )
	{
		fread( &WAV_Subchunk2, 1, sizeof(WAV_Subchunk2), fp );

		if ( memcmp( &WAV_Subchunk2.id, "data", 4 ) )
		{
			fseek( fp, WAV_Subchunk2.len, SEEK_CUR );
		}
		else
		{
			break;
		}
	}

	waveLen = WAV_Subchunk2.len;

	// Write pregap region
	memset(buff, 0x00, CD_SECTOR_SIZE);
	if ( pregap )
	{
		for ( int i=0; i<150; i++ )
		{
			writer->WriteBytesRaw(buff, CD_SECTOR_SIZE);
		}
	}

	// Write data
	while ( waveLen > 0 )
	{
		memset(buff, 0x00, CD_SECTOR_SIZE);

        int readLen = waveLen;

        if (readLen > CD_SECTOR_SIZE)
		{
			readLen = CD_SECTOR_SIZE;
		}

		fread( buff, 1, readLen, fp );
        writer->WriteBytesRaw( buff, CD_SECTOR_SIZE );

        waveLen -= readLen;
	}

	fclose( fp );
	return true;
}

iso::DirTreeClass::DirTreeClass()
{
	recordLBA = 0;
	passedSector = false;
	parent = nullptr;
	first_track = false;

	name = rootname;
}

iso::DirTreeClass::~DirTreeClass()
{
	for ( int i=0; i<entries.size(); i++ )
	{
		if (entries[i].subdir != nullptr)
		{
			delete (DirTreeClass*)entries[i].subdir;
		}
	}
}

int	iso::DirTreeClass::AddFileEntry(const char* id, int type, const char* srcfile)
{
	struct stat fileAttrib;

    if ( stat( srcfile, &fileAttrib ) != 0 )
	{
		if ( !global::QuietMode )
		{
			printf("      ");
		}

		printf("ERROR: File not found: %s\n", srcfile);
		return false;
    }

	// Check if XA data is valid
	if ( type == EntryXA )
	{
		// Check header
		char buff[4];
		FILE* fp = fopen(srcfile, "rb");
		fread(buff, 1, 4, fp);
		fclose(fp);

		// Check if its a RIFF (WAV container)
		if ( strncmp(buff, "RIFF", 4) == 0 )
		{
			if (!global::QuietMode)
			{
				printf("      ");
			}

			printf("ERROR: %s is a WAV or is not properly ripped!\n", srcfile);

			return false;
		}

		// Check if size is a multiple of 2336 bytes
		if ( ( fileAttrib.st_size % 2336 ) != 0 )
		{
			if ( !global::QuietMode )
			{
				printf("      ");
			}

			printf("ERROR: %s is not a multiple of 2336 bytes.\n", srcfile);

			if ( !global::QuietMode )
			{
				printf("      ");
			}

			printf("Did you create your multichannel XA file with subheader data?\n");

			return false;
		}

		// Check if first sub header is valid usually by checking
		// if the first four bytes match the next four bytes
		else if ( ((int*)buff)[0] == ((int*)buff)[1] )
		{
			if ( !global::QuietMode )
			{
				printf("      ");
			}

			printf("WARNING: %s may not have a valid subheader. ", srcfile);
		}

	// Check STR data
	} else if ( type == EntrySTR ) {

		// Check header
		char buff[4];
		FILE* fp = fopen(srcfile, "rb");
		fread(buff, 1, 4, fp);
		fclose(fp);

		// Check if its a RIFF (WAV container)
		if ( strncmp(buff, "RIFF", 4) == 0 )
		{
			if (!global::QuietMode)
			{
				printf("      ");
			}

			printf("ERROR: %s is a WAV or is not properly ripped.\n", srcfile);

			return false;
		}

		// Check if size is a multiple of 2336 bytes
		if ( ( fileAttrib.st_size % 2336 ) != 0 )
		{
			if ( ( fileAttrib.st_size % 2048) == 0 )
			{
				type = EntrySTR_DO;
			}
			else
			{
				if ( !global::QuietMode )
				{
					printf("      ");
				}

				printf("ERROR: %s is not a multiple of 2336 or 2048 bytes.\n",
					srcfile);

				return false;
			}
		}

	}


	std::string temp_name = id;

	for ( int i=0; temp_name[i] != 0x00; i++ )
	{
		temp_name[i] = std::toupper( temp_name[i] );
	}

	temp_name += ";1";


	// Check if file entry already exists
    for ( int i=0; i<entries.size(); i++ )
	{
		if ( !entries[i].id.empty() )
		{
            if ( ( entries[i].type == EntryFile )
				&& ( icompare( entries[i].id, temp_name ) ) )
			{
				if (!global::QuietMode)
				{
					printf("      ");
				}

				printf("ERROR: Duplicate file entry: %s\n", id);

				return false;
            }

		}

    }

	DIRENTRY entry;
	memset( &entry, 0x00, sizeof(DIRENTRY) );

	entry.id.assign(temp_name.c_str());
	entry.type		= type;
	entry.subdir	= nullptr;

	if ( srcfile != nullptr )
	{
		entry.srcfile = srcfile;
	}

	if ( type == EntryDA )
	{
		entry.length = GetWavSize( srcfile );
	}
	else if ( type != EntryDir )
	{
		entry.length = fileAttrib.st_size;
	}

    tm* fileTime = gmtime( &fileAttrib.st_mtime );

    entry.date.hour		= fileTime->tm_hour;
    entry.date.minute	= fileTime->tm_min;
    entry.date.second	= fileTime->tm_sec;
    entry.date.day		= fileTime->tm_mday;
    entry.date.month	= fileTime->tm_mon+1;
    entry.date.year		= fileTime->tm_year;
    entry.date.GMToffs	= 0;

	entries.push_back( entry );

	return true;

}

void iso::DirTreeClass::AddDummyEntry(int sectors)
{
	DIRENTRY entry;

	entry.subdir	= nullptr;
	entry.type		= EntryFile;
	entry.length	= 2048*sectors;

	entries.push_back( entry );
}

iso::DirTreeClass* iso::DirTreeClass::AddSubDirEntry(const char* id)
{
    for ( int i=0; i<entries.size(); i++ )
	{
		if ( !entries[i].id.empty() )
		{
			if ( ( entries[i].type == iso::EntryDir ) &&
				( entries[i].id.compare( id ) == 0 ) )
			{
				if (!global::QuietMode)
				{
					printf("      ");
				}

				printf("ERROR: Duplicate directory entry: %s\n", id);

				return nullptr;
			}
		}

    }

	DIRENTRY entry;

	entry.id		= id;

	for ( int i=0; i<entry.id.size(); i++ )
	{
		entry.id[i] = std::toupper( entry.id[i] );
	}

	entry.type		= EntryDir;
	entry.subdir	= (void*) new DirTreeClass; // TODO: when should this be freed?

	entries.push_back( entry );

	entries.back().length	= ((iso::DirTreeClass*)entries.back().subdir)->CalculateDirEntryLen();

	((DirTreeClass*)entry.subdir)->parent = this;

	tm*	dirTime = gmtime( &global::BuildTime );

	entries.back().date.hour	= dirTime->tm_hour;
	entries.back().date.minute	= dirTime->tm_min;
	entries.back().date.second	= dirTime->tm_sec;
	entries.back().date.month	= dirTime->tm_mon+1;
	entries.back().date.day		= dirTime->tm_mday;
	entries.back().date.year	= dirTime->tm_year;
	entries.back().date.GMToffs	= 0;

	for ( int i=0; entry.id[i] != 0x00; i++ )
	{
		entry.id[i] = toupper( entry.id[i] );
	}

	return (DirTreeClass*)entry.subdir;
}

void iso::DirTreeClass::PrintRecordPath()
{
	DirTreeClass *p = (DirTreeClass*)parent;

	if ( p == nullptr )
	{
		return;
	}

	p->PrintRecordPath();
	printf( "/%s", name.c_str() );
}

int iso::DirTreeClass::CalculateFileSystemSize(int lba) {

	// Set LBA of directory record of this class
	recordLBA = lba;

	lba += CalculateDirEntryLen()/2048;

	for ( int i=0; i<entries.size(); i++ )
	{
		// If it is a subdir
		if (entries[i].subdir != nullptr)
		{
			// Recursively calculate the LBA of subdirectories
			lba = ((DirTreeClass*)entries[i].subdir)->CalculateFileSystemSize(lba);
		}
		else
		{
			// Increment LBA by the size of files
			if ( ( entries[i].type == EntryFile ) ||
				( entries[i].type == EntrySTR_DO ) )
			{
				lba += (entries[i].length+2047)/2048;
			}
			else if ( ( entries[i].type == EntryXA ) ||
				( entries[i].type == EntrySTR ) )
			{
				lba += (entries[i].length+2335)/2336;
			}
		}
	}

	return lba;

}

int iso::DirTreeClass::CalculateTreeLBA(int lba)
{
	// Set LBA of directory record of this class
	recordLBA = lba;

	lba += CalculateDirEntryLen()/2048;

	if ( ( global::NoLimit == false) && (passedSector) )
	{
		if (!global::QuietMode)
			printf("      ");

		printf("WARNING: Directory record ");
		PrintRecordPath();
		printf(" exceeds 2048 bytes.\n");
	}

	for ( int i=0; i<entries.size(); i++ )
	{
		// Set current LBA to directory record entry
		if ( ( entries[i].type == EntryDA ) && ( !first_track ) )
		{
			entries[i].lba = lba;
		}
		else
		{
			entries[i].lba = lba;
		}

		// If it is a subdir
		if (entries[i].subdir != nullptr)
		{
			((DirTreeClass*)entries[i].subdir)->name = entries[i].id;

			// Recursively calculate the LBA of subdirectories
			lba = ((DirTreeClass*)entries[i].subdir)->CalculateTreeLBA(lba);

			entries[i].length = ((DirTreeClass*)entries[i].subdir)->CalculateDirEntryLen();
		}
		else
		{
			// Increment LBA by the size of file
			if ( ( entries[i].type == EntryFile ) ||
				( entries[i].type == EntrySTR_DO ) )
			{
				lba += (entries[i].length+2047)/2048;
			}
			else if ( ( entries[i].type == EntryXA ) ||
				(entries[i].type == EntrySTR ) )
			{
				lba += (entries[i].length+2335)/2336;
			}
			else if ( entries[i].type == EntryDA )
			{
				if ( !first_track )
				{
					lba += ((entries[i].length+2351)/2352)+150;
					first_track = true;
				}
				else
				{
					lba += ((entries[i].length+2351)/2352);
				}
			}
		}
	}

	return lba;
}

int iso::DirTreeClass::CalculateDirEntryLen()
{

	int dirEntryLen = 68;

	if ( !global::noXA )
	{
		dirEntryLen += 28;
	}

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i].id.empty() )
		{
			continue;
		}

		int dataLen = 33;

		dataLen += 2*((entries[i].id.size()+1)/2);

		if ( ( entries[i].id.size()%2 ) == 0 )
		{
			dataLen++;
		}

		if ( !global::noXA )
		{
			dataLen += sizeof( cd::ISO_XA_ATTRIB );
		}

		if ( ((dirEntryLen%2048)+dataLen) > 2048 )
		{
			dirEntryLen = (2048*(dirEntryLen/2048))+(dirEntryLen%2048);
		}

		dirEntryLen += dataLen;
	}

	if (dirEntryLen > 2048)
	{
		passedSector = true;
	}

	return 2048*((dirEntryLen+2047)/2048);
}

void iso::DirTreeClass::SortDirEntries()
{
	iso::DIRENTRY temp;
	int numdummies = 0;

	if ( entries.size() < 2 )
	{
		return;
	}

	// Search for directories
	for(int i=0; i<entries.size(); i++)
	{
		if ( entries[i].type == EntryDir )
		{
			// Perform recursive call
            if ( entries[i].subdir != nullptr )
			{
				((iso::DirTreeClass*)entries[i].subdir)->SortDirEntries();
			}
		}
		else
		{
			if ( entries[i].id.empty() )
			{
				numdummies++;
			}
		}
	}

	// Sort dummies to end of list
	for ( int i=1; i<entries.size(); i++ )
	{
		for ( int j=1; j<entries.size(); j++ )
		{
			if ( (entries[j-1].id.empty()) && (!entries[j].id.empty()) )
			{
				temp = entries[j];
				entries[j] = entries[j-1];
				entries[j-1] = temp;
			}
		}
	}

	// Now sort the entries
	for ( int i=1; i<entries.size()-numdummies; i++ )
	{
		for ( int j=1; j<entries.size()-numdummies; j++ )
		{
			if ( entries[j-1].id.compare( entries[j].id ) > 0 )
			{
				temp = entries[j-1];
				entries[j-1] = entries[j];
				entries[j] = temp;
			}
		}
	}

}

int iso::DirTreeClass::WriteDirEntries(cd::IsoWriter* writer, int lastLBA)
{
	char	dataBuff[2048];
	char*	dataBuffPtr=dataBuff;
	char	entryBuff[128];
	int		dirlen;

	cd::ISO_DIR_ENTRY*	entry;
	cd::ISO_XA_ATTRIB*	xa;

	memset(dataBuff, 0x00, 2048);

	writer->SeekToSector( recordLBA );

	for( int i=0; i<2; i++ )
	{
		entry = (cd::ISO_DIR_ENTRY*)dataBuffPtr;

		SetPair16( &entry->volSeqNum, 1 );
		entry->identifierLen = 1;

		int dataLen = 32;

		if (i == 0)
		{
			// Current
			dirlen = 2048*((CalculateDirEntryLen()+2047)/2048);

			SetPair32( &entry->entrySize, dirlen );
			SetPair32( &entry->entryOffs, recordLBA );
		}
		else
		{
			// Parent
			if ( parent ) {
				dirlen = ((DirTreeClass*)parent)->CalculateDirEntryLen();
			} else {
				dirlen = CalculateDirEntryLen();
			}
			dirlen = 2048*((dirlen+2047)/2048);

			SetPair32( &entry->entrySize, dirlen );
			SetPair32( &entry->entryOffs, lastLBA );
			dataBuffPtr[dataLen+1] = 0x01;
		}

		dataLen += 2;

		if ( !global::noXA )
		{
			xa = (cd::ISO_XA_ATTRIB*)(dataBuffPtr+dataLen);
			memset( xa, 0x00, sizeof(cd::ISO_XA_ATTRIB) );

			xa->id[0] = 'X';
			xa->id[1] = 'A';
			xa->attributes  = 0x558d;

			dataLen += sizeof(cd::ISO_XA_ATTRIB);
		}

		entry->flags = 0x02;
		entry->entryLength = dataLen;

		tm* dirTime = gmtime( &global::BuildTime );

		entry->entryDate.year	= dirTime->tm_year;
		entry->entryDate.month	= dirTime->tm_mon+1;
		entry->entryDate.day	= dirTime->tm_mday;
		entry->entryDate.hour	= dirTime->tm_hour;
		entry->entryDate.minute	= dirTime->tm_min;
		entry->entryDate.second	= dirTime->tm_sec;

		dataBuffPtr += dataLen;
	}

	for ( int i=0; i<entries.size(); i++)
	{
		if ( entries[i].id.empty() )
		{
			continue;
		}

		memset( entryBuff, 0x00, 128 );
		entry = (cd::ISO_DIR_ENTRY*)entryBuff;

		if ( entries[i].type == EntryDir )
		{
			entry->flags = 0x02;
		}
		else
		{
			entry->flags = 0x00;
		}

		// File length correction for certain file types
		int lba = entries[i].lba;
		int length = 0;

		if ( ( entries[i].type == EntryXA ) || ( entries[i].type == EntrySTR ) )
		{
			length = 2048*((entries[i].length+2335)/2336);
		}
		else if ( entries[i].type == EntrySTR_DO )
		{
			length = 2048*((entries[i].length+2047)/2048);
		}
		else if ( entries[i].type == EntryDA )
		{
			length = 2048*((entries[i].length+2351)/2352);
			lba += 150;
		}
		else
		{
			length = entries[i].length;
		}

		SetPair32( &entry->entryOffs, lba );
		SetPair32( &entry->entrySize, length );
		SetPair16( &entry->volSeqNum, 1 );

		entry->identifierLen = entries[i].id.length();
		entry->entryDate = entries[i].date;

		int dataLen = 33;

		strncpy( &entryBuff[dataLen], entries[i].id.c_str(), entry->identifierLen );
		dataLen += entry->identifierLen;

		if ( (entry->identifierLen%2) == 0 )
		{
			dataLen++;
		}

		if ( !global::noXA )
		{
			xa = (cd::ISO_XA_ATTRIB*)(entryBuff+dataLen);
			memset(xa, 0x00, sizeof(cd::ISO_XA_ATTRIB));

			xa->id[0] = 'X';
			xa->id[1] = 'A';

			if ( (entries[i].type == EntryFile) ||
				(entries[i].type == EntrySTR_DO) )
			{
				xa->attributes	= 0x550d;
			}
			else if (entries[i].type == EntryDA)
			{
				xa->attributes	= 0x5545;
			}
			else if ( (entries[i].type == EntrySTR) ||
				(entries[i].type == EntryXA) )
			{
				xa->attributes	= 0x553d;
			}
			else if (entries[i].type == EntryDir)
			{
				xa->attributes	= 0x558d;
			}

			dataLen += sizeof(cd::ISO_XA_ATTRIB);
		}

		entry->entryLength = dataLen;

		if ( (dataBuffPtr+dataLen) > (dataBuff+2047) )
		{
			writer->SetSubheader( cd::IsoWriter::SubData );
			writer->WriteBytes( dataBuff, 2048, cd::IsoWriter::EdcEccForm1 );

			memset( dataBuff, 0x00, 2048 );
			dataBuffPtr = dataBuff;
		}

		memcpy( dataBuffPtr, entryBuff, dataLen );
		dataBuffPtr += dataLen;
	}

	writer->SetSubheader( cd::IsoWriter::SubEOF );
	writer->WriteBytes( dataBuff, 2048, cd::IsoWriter::EdcEccForm1 );

	return 1;
}

int iso::DirTreeClass::WriteDirectoryRecords(cd::IsoWriter* writer, int lastDirLBA)
{
	if (lastDirLBA == 0)
	{
		lastDirLBA = recordLBA;
	}

	WriteDirEntries( writer, lastDirLBA );

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i].type == EntryDir )
		{
			if ( !((DirTreeClass*)entries[i].subdir)->WriteDirectoryRecords(
				writer, recordLBA ) )
			{
				return 0;
			}
		}
	}

	return 1;
}

int iso::DirTreeClass::WriteFiles(cd::IsoWriter* writer)
{
	first_track = false;

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( ( entries[i].type == EntryDA ) && ( first_track ) )
		{
			writer->SeekToSector( entries[i].lba-150 );
		}
		else
		{
			writer->SeekToSector( entries[i].lba );
		}

		// Write files and dummies as regular data sectors
		if ( entries[i].type == EntryFile )
		{
			char buff[2048];

			if ( !entries[i].srcfile.empty() )
			{
				if ( !global::QuietMode )
				{
					printf( "      Packing %s... ", entries[i].srcfile.c_str() );
				}

				FILE *fp = fopen( entries[i].srcfile.c_str(), "rb" );

				writer->SetSubheader( cd::IsoWriter::SubData );

				while( !feof( fp ) )
				{
					memset( buff, 0x00, 2048 );
					fread( buff, 1, 2048, fp );

					if ( feof( fp ) )
					{
						writer->SetSubheader( cd::IsoWriter::SubEOF );
					}

					writer->WriteBytes( buff, 2048, cd::IsoWriter::EdcEccForm1 );
				}

				fclose( fp );

				if ( !global::QuietMode )
				{
					printf("Done.\n");
				}

			}
			else
			{
				memset( buff, 0x00, 2048 );

				writer->SetSubheader( cd::IsoWriter::SubData );
				for ( int c=0; c<(entries[i].length/2048); c++ )
				{
					if ( c == ((entries[i].length/2048)-1) )
					{
						writer->SetSubheader( cd::IsoWriter::SubEOF );
					}
					writer->WriteBytes( buff, 2048, cd::IsoWriter::EdcEccForm1 );
				}
			}

		// Write XA audio streams as Mode 2 Form 2 sectors without ECC
		}
		else if (entries[i].type == EntryXA)
		{
			char buff[2336];

			if (!global::QuietMode)
			{
				printf( "      Packing XA %s... ", entries[i].srcfile.c_str() );
			}

			FILE *fp = fopen( entries[i].srcfile.c_str(), "rb" );

			while( !feof( fp ) )
			{
				fread( buff, 1, 2336, fp );
				writer->WriteBytesXA(buff, 2336, cd::IsoWriter::EdcEccForm2);
			}

			fclose( fp );

			if ( !global::QuietMode )
			{
				printf( "Done.\n" );
			}

		// Write STR video streams as Mode 2 Form 1 (video sectors) and Mode 2 Form 2 (XA audio sectors)
		// Video sectors have EDC/ECC while XA does not
		}
		else if ( entries[i].type == EntrySTR )
		{
			char buff[2336];

			if ( !global::QuietMode )
			{
				printf( "      Packing STR %s... ", entries[i].srcfile.c_str() );
			}

			FILE *fp = fopen( entries[i].srcfile.c_str(), "rb" );

			while ( !feof( fp ) )
			{
				memset( buff, 0x00, 2336 );
				fread( buff, 1, 2336, fp );

				// Check submode if sector is mode 2 form 2
				if ( buff[2]&0x20 )
				{
				    // If so, write it as an XA sector
					writer->WriteBytesXA( buff, 2336, cd::IsoWriter::EdcEccForm2 );

				}
				else
				{
					// Otherwise, write it as Mode 2 Form 1
					writer->WriteBytesXA( buff, 2336, cd::IsoWriter::EdcEccForm1 );
				}

			}

			fclose( fp );

			if (!global::QuietMode)
			{
				printf( "Done.\n" );
			}

		// Write data only STR streams as Mode 2 Form 1
		}
		else if ( entries[i].type == EntrySTR_DO )
		{
			char buff[2048];

			if ( !entries[i].srcfile.empty() )
			{
				if ( !global::QuietMode )
				{
					printf( "      Packing STR-DO %s... ", entries[i].srcfile.c_str() );
				}

				FILE *fp = fopen( entries[i].srcfile.c_str(), "rb" );

				writer->SetSubheader( cd::IsoWriter::SubSTR );

				while( !feof( fp ) )
				{
					memset( buff, 0x00, 2048 );
					fread( buff, 1, 2048, fp );

					writer->WriteBytes( buff, 2048, cd::IsoWriter::EdcEccForm1 );
				}

				fclose( fp );

				writer->SetSubheader(0x00080000);

				if ( !global::QuietMode )
				{
					printf("Done.\n");
				}

			}
			else
			{
				memset( buff, 0x00, 2048 );

				writer->SetSubheader( cd::IsoWriter::SubData );
				for ( int c=0; c<(entries[i].length/2048); c++ )
				{
					if ( c == ((entries[i].length/2048)-1) )
					{
						writer->SetSubheader( cd::IsoWriter::SubEOF );
					}
					writer->WriteBytes( buff, 2048, cd::IsoWriter::EdcEccForm1 );
				}
			}

		// Write DA files as audio tracks
		}
		else if ( entries[i].type == EntryDA )
		{
			if ( !global::QuietMode )
			{
				printf( "      Packing DA %s... ", entries[i].srcfile.c_str() );
			}

			if ( PackWaveFile( writer, entries[i].srcfile.c_str(), first_track ) )
			{
				if (!global::QuietMode)
				{
					printf( "Done.\n" );
				}
			}

			first_track = true;
		}
		else if ( entries[i].type == EntryDir )
		{
			((DirTreeClass*)entries[i].subdir)->WriteFiles( writer );
		}
	}

	return 1;
}

void iso::DirTreeClass::OutputHeaderListing(FILE* fp, int level)
{
	if ( level == 0 )
	{
		fprintf( fp, "#ifndef _ISO_FILES\n" );
		fprintf( fp, "#define _ISO_FILES\n\n" );
	}

	fprintf( fp, "/* %s */\n", name.c_str() );

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( ( !entries[i].id.empty() ) && ( entries[i].type != EntryDir ) )
		{
			std::string temp_name;

			temp_name = "LBA_" + entries[i].id;

			for ( int c=0; c<temp_name.length(); c++ )
			{
				temp_name[c] = std::toupper( temp_name[c] );

				if ( temp_name[c] == '.' )
				{
					temp_name[c] = '_';
				}

				if ( temp_name[c] == ';' )
				{
					temp_name[c] = 0x00;
				}
			}

			fprintf( fp, "#define %s", temp_name.c_str() );

			for( int s=0; s<17-(int)entries[i].id.size(); s++ )
			{
				fprintf( fp, " " );
			}

			fprintf( fp, "%d\n", entries[i].lba );

		}
	}

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i].type == EntryDir )
		{
			fprintf( fp, "\n" );
			((DirTreeClass*)entries[i].subdir)->OutputHeaderListing( fp, level+1 );
		}
	}

	if ( level == 0 )
	{
		fprintf( fp, "\n#endif\n" );
	}
}

int iso::DirTreeClass::WriteCueEntries(FILE* fp, int* trackNum)
{

	for(int i=0; i<entries.size(); i++)
	{
		if ( entries[i].type == EntryDA )
		{
			*trackNum += 1;
			fprintf( fp, "  TRACK %02d AUDIO\n", *trackNum );

			int trackLBA = entries[i].lba-150;

			if ( *trackNum == 2 )
			{
				fprintf( fp, "    PREGAP 00:02:00\n" );
			}
			else
			{
				fprintf( fp, "    INDEX 00 %02d:%02d:%02d\n",
					(trackLBA/75)/60, (trackLBA/75)%60,
					trackLBA%75 );
			}

			trackLBA += 150;

			fprintf( fp, "    INDEX 01 %02d:%02d:%02d\n",
				(trackLBA/75)/60, (trackLBA/75)%60, trackLBA%75 );

		}
		else if ( entries[i].type == EntryDir )
		{
			((DirTreeClass*)entries[i].subdir)->WriteCueEntries( fp, trackNum );
		}

	}

	return *trackNum;
}

void LBAtoTimecode(int lba, char* timecode)
{
	sprintf( timecode, "%02d:%02d:%02d", (lba/75)/60, (lba/75)%60, (lba%75) );
}

void iso::DirTreeClass::OutputLBAlisting(FILE* fp, int level)
{
	char textbuff[10];

	for ( int i=0; i<entries.size(); i++ )
	{
		fprintf( fp, "    " );

		if ( !entries[i].id.empty() )
		{
			if ( entries[i].type == EntryFile )
			{
				fprintf( fp, "File  " );
			}
			else if ( entries[i].type == EntryDir )
			{
				fprintf( fp, "Dir   " );
			}
			else if ( ( entries[i].type == EntrySTR ) ||
				( entries[i].type == EntrySTR_DO ) )
			{
				fprintf( fp, "STR   " );
			}
			else if ( entries[i].type == EntryXA )
			{
				fprintf( fp, "XA    " );
			}
			else if ( entries[i].type == EntryDA )
			{
				fprintf( fp, "CDDA  " );
			}

			fprintf( fp, "%s", entries[i].id.c_str() );
			for ( int s=0; s<17-(int)strlen(entries[i].id.c_str()); s++ )
			{
				fprintf( fp, " " );
			}
		} else {

			fprintf( fp, "Dummy <DUMMY>          " );

		}

		// Write size in sector units
		sprintf( textbuff, "%d", ((entries[i].length+2047)/2048) );
		fprintf( fp, "%s", textbuff );
		for ( int s=0; s<10-(int)strlen(textbuff); s++ )
		{
			fprintf( fp, " " );
		}

		// Write LBA offset
		sprintf( textbuff, "%d", entries[i].lba );
		fprintf( fp, "%s", textbuff );
		for ( int s=0; s<10-(int)strlen(textbuff); s++ )
		{
			fprintf( fp, " " );
		}

		// Write Timecode
		LBAtoTimecode( 150+entries[i].lba, textbuff );
		fprintf( fp, "%s    ", textbuff );

		// Write size in byte units
		sprintf( textbuff, "%d", entries[i].length );
		fprintf( fp, "%s", textbuff );
		for ( int s=0; s<10-(int)strlen(textbuff); s++ )
		{
			fprintf( fp, " " );
		}

		// Write source file path
		if ( (!entries[i].id.empty()) && (entries[i].type != EntryDir) )
		{
			fprintf( fp, "%s\n", entries[i].srcfile.c_str() );
		}
		else
		{
			fprintf( fp, " \n" );
		}

		if ( entries[i].type == EntryDir )
		{
			((DirTreeClass*)entries[i].subdir)->OutputLBAlisting( fp, level+1 );
		}
	}

	if ( level > 0 )
	{
		fprintf( fp, "    End   %s\n", name.c_str() );
	}
}


int iso::DirTreeClass::CalculatePathTableLenSub(DIRENTRY* dirEntry)
{
	int len = 8;

	// Put identifier (nullptr if first entry)
	len += 2*((dirEntry->id.size()+1)/2);

	for ( int i=0; i<((DirTreeClass*)dirEntry->subdir)->entries.size(); i++ )
	{
		if ( ((DirTreeClass*)dirEntry->subdir)->entries[i].type == EntryDir )
		{
			len += CalculatePathTableLenSub(
				&((DirTreeClass*)dirEntry->subdir)->entries[i] );
		}
	}

	return len;
}

int iso::DirTreeClass::CalculatePathTableLen()
{
	int len = 10;

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i].type == EntryDir )
		{
			len += CalculatePathTableLenSub( &entries[i] );
		}
	}

	return len;
}

void iso::DirTreeClass::GenPathTableSub(PathTableClass* table,
	DirTreeClass* dir, int parentIndex, int msb)
{
	for ( int i=0; i<dir->entries.size(); i++ )
	{
		if ( dir->entries[i].type == EntryDir )
		{
			PathEntryClass* entry = new PathEntryClass;

			dirIndex++;
			entry->dir_id		= &dir->entries[i].id;
			entry->dir_level	= parentIndex;
			entry->dir_lba		= ((DirTreeClass*)dir->entries[i].subdir)->recordLBA;
			entry->dir			= dir->entries[i].subdir;
			entry->next_parent	= dirIndex;

			table->entries.push_back( entry );
		}
	}

	for ( int i=0; i<table->entries.size(); i++ ) {

		DirTreeClass* subdir = (DirTreeClass*)table->entries[i]->dir;
		PathTableClass* sub = new PathTableClass;

		GenPathTableSub( sub, subdir, table->entries[i]->next_parent, msb );
		table->entries[i]->sub = sub;

	}
}

int iso::DirTreeClass::GeneratePathTable(unsigned char* buff, int msb)
{
	PathTableClass pathTable;

	dirIndex = 1;

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i].type == EntryDir )
		{
			PathEntryClass* entry = new PathEntryClass;

			dirIndex++;
			entry->dir_id		= &entries[i].id;
			entry->dir_level	= 1;
			entry->dir_lba		= ((DirTreeClass*)entries[i].subdir)->recordLBA;
			entry->dir			= entries[i].subdir;
			entry->next_parent	= dirIndex;

			pathTable.entries.push_back(entry);
		}
	}

	for ( int i=0; i<pathTable.entries.size(); i++ ) {

		DirTreeClass* subdir = (DirTreeClass*)pathTable.entries[i]->dir;
		PathTableClass* sub = new PathTableClass;

		GenPathTableSub( sub, subdir, pathTable.entries[i]->next_parent, msb );
		pathTable.entries[i]->sub = sub;
	}

	*buff = 1;	// Directory identifier length
	buff++;
	*buff = 0;	// Extended attribute record length (unused)
	buff++;

	int lba = recordLBA;

	// Write LBA and directory number index
	memcpy( buff, &lba, 4 );
	*((short*)(buff+4)) = 1;

	if ( msb )
	{
		cd::SwapBytes( buff, 4 );
		cd::SwapBytes( buff+4, 2 );
	}

	buff += 6;

	// Put identifier (nullptr if first entry)
	memset( buff, 0x00, 2 );
	buff += 2;

	unsigned char* newbuff = pathTable.GenTableData( buff, msb );

	return (int)(newbuff-buff);

}

int iso::DirTreeClass::GetFileCountTotal()
{
    int numfiles = 0;

    for ( int i=0; i<entries.size(); i++ )
	{
        if ( (entries[i].type != EntryDir) )
		{
			if ( !entries[i].id.empty() )
			{
				numfiles++;
			}
		}
		else
		{
            numfiles += ((DirTreeClass*)entries[i].subdir)->GetFileCountTotal();
		}
    }

    return numfiles;
}

int iso::DirTreeClass::GetDirCountTotal()
{
	int numdirs = 0;

    for ( int i=0; i<entries.size(); i++ )
	{
        if ( entries[i].type == EntryDir )
		{
			numdirs++;
            numdirs += ((DirTreeClass*)entries[i].subdir)->GetDirCountTotal();
		}
    }

    return numdirs;
}

void iso::WriteLicenseData(cd::IsoWriter* writer, void* data)
{
	writer->SeekToSector( 0 );
	writer->WriteBytesXA( data, 2336*12, cd::IsoWriter::EdcEccForm1 );

	char blank[2048];
	memset(blank, 0, 2048);

	writer->SetSubheader(0x00200000);

	for( int i=0; i<4; i++ ) {

		writer->WriteBytes( data, 2048, cd::IsoWriter::EdcEccForm1 );

	}

	writer->SetSubheader(0x00080000);
}

void iso::WriteDescriptor(cd::IsoWriter* writer, iso::IDENTIFIERS id,
	iso::DirTreeClass* dirTree, int imageLen)
{
	cd::ISO_DESCRIPTOR	isoDescriptor;

	memset( &isoDescriptor, 0x00, sizeof(cd::ISO_DESCRIPTOR) );

	isoDescriptor.header.type = 1;
	isoDescriptor.header.version = 1;
	strncpy( (char*)isoDescriptor.header.id, "CD001", 5 );

	// Set System identifier
	memset( isoDescriptor.systemID, 0x20, 32 );

	if ( id.SystemID != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.SystemID); i++ )
		{
			isoDescriptor.systemID[i] = std::toupper( id.SystemID[i] );
		}
	}

	// Set Volume identifier
	memset( isoDescriptor.volumeID, 0x20, 32 );

	if ( id.VolumeID != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.VolumeID); i++ )
		{
			isoDescriptor.volumeID[i] = std::toupper( id.VolumeID[i] );
		}
	}

	// Set Application identifier
	memset( isoDescriptor.applicationIdentifier, 0x20, 128 );

	if ( id.Application != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.Application); i++ )
		{
			isoDescriptor.applicationIdentifier[i] =
				std::toupper( id.Application[i] );
		}
	}

	// Volume Set identifier
	memset( isoDescriptor.volumeSetIdentifier, 0x20, 128 );
	if ( id.VolumeSet != nullptr )
	{
		for(int i=0; i<(int)strlen(id.VolumeSet); i++)
		{
			isoDescriptor.volumeSetIdentifier[i] =
				std::toupper(id.VolumeSet[i]);
		}
	}

	// Publisher identifier
	memset( isoDescriptor.publisherIdentifier, 0x20, 128 );
	if ( id.Publisher != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.Publisher); i++ )
		{
			isoDescriptor.publisherIdentifier[i] =
				std::toupper(id.Publisher[i]);
		}
	}

	// Data preparer identifier
	memset( isoDescriptor.dataPreparerIdentifier, 0x20, 128 );
	strcpy( isoDescriptor.dataPreparerIdentifier, "DISC IMAGE CREATED "
		"WITH MKPSXISO BY LAMEGUY64 OF MEIDO-TEK PRODUCTIONS "
		"HTTPS://GITHUB.COM/LAMEGUY64/MKPSXISO" );
	*strchr( isoDescriptor.dataPreparerIdentifier, 0x00 ) = 0x20;
	if ( id.DataPreparer != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.DataPreparer); i++ )
		{
			isoDescriptor.dataPreparerIdentifier[i] =
				std::toupper( id.DataPreparer[i] );
		}
	}

	// Copyright (file) identifier
	memset( isoDescriptor.copyrightFileIdentifier, 0x20, 128 );
	if ( id.Copyright != nullptr )
	{
		for ( int i=0; i<(int)strlen(id.Copyright); i++ )
		{
			isoDescriptor.copyrightFileIdentifier[i] =
				std::toupper( id.Copyright[i] );
		}
	}

	// Unneeded identifiers
	memset( isoDescriptor.abstractFileIdentifier, 0x20, 37 );
	memset( isoDescriptor.bibliographicFilelIdentifier, 0x20, 37 );

	tm* imageTime = localtime( &global::BuildTime );

	sprintf( isoDescriptor.volumeCreateDate, "%04d%02d%02d%02d%02d%02d00",
		imageTime->tm_year+1900, imageTime->tm_mon, imageTime->tm_mday,
		imageTime->tm_hour, imageTime->tm_min, imageTime->tm_sec );

	sprintf( isoDescriptor.volumeModifyDate, "%04d%02d%02d%02d%02d%02d00",
		imageTime->tm_year+1900, imageTime->tm_mon, imageTime->tm_mday,
		imageTime->tm_hour, imageTime->tm_min, imageTime->tm_sec );

	strcpy( isoDescriptor.volumeEffeciveDate, "0000000000000000" );
	strcpy( isoDescriptor.volumeExpiryDate, "0000000000000000" );

	isoDescriptor.fileStructVersion = 1;

	if ( !global::noXA )
	{
		strncpy( (char*)&isoDescriptor.appData[141], "CD-XA001", 8 );
	}

	int pathTableLen = dirTree->CalculatePathTableLen();
	int pathTableSectors = (pathTableLen+2047)/2048;

	cd::SetPair16( &isoDescriptor.volumeSetSize, 1 );
	cd::SetPair16( &isoDescriptor.volumeSeqNumber, 1 );
	cd::SetPair16( &isoDescriptor.sectorSize, 2048 );
	cd::SetPair32( &isoDescriptor.pathTableSize, pathTableLen );

	// Setup the root directory record
	isoDescriptor.rootDirRecord.entryLength = 34;
	isoDescriptor.rootDirRecord.extLength	= 0;
	cd::SetPair32( &isoDescriptor.rootDirRecord.entryOffs,
		18+(pathTableSectors*4) );
	cd::SetPair32( &isoDescriptor.rootDirRecord.entrySize,
		2048*((dirTree->CalculateDirEntryLen()+2047)/2048) );
	isoDescriptor.rootDirRecord.flags = 0x02;
	cd::SetPair16( &isoDescriptor.rootDirRecord.volSeqNum, 1);
	isoDescriptor.rootDirRecord.identifierLen = 1;
	isoDescriptor.rootDirRecord.identifier = 0x0;

	isoDescriptor.rootDirRecord.entryDate.year		= imageTime->tm_year;
	isoDescriptor.rootDirRecord.entryDate.month		= imageTime->tm_mon+1;
	isoDescriptor.rootDirRecord.entryDate.day		= imageTime->tm_mday;
	isoDescriptor.rootDirRecord.entryDate.hour		= imageTime->tm_hour;
	isoDescriptor.rootDirRecord.entryDate.minute	= imageTime->tm_min;
	isoDescriptor.rootDirRecord.entryDate.second	= imageTime->tm_sec;

	isoDescriptor.pathTable1Offs = 18;
	isoDescriptor.pathTable2Offs = isoDescriptor.pathTable1Offs+
		pathTableSectors;
	isoDescriptor.pathTable1MSBoffs = isoDescriptor.pathTable2Offs+1;
	isoDescriptor.pathTable2MSBoffs =
		isoDescriptor.pathTable1MSBoffs+pathTableSectors;
	cd::SwapBytes( &isoDescriptor.pathTable1MSBoffs, 4 );
	cd::SwapBytes( &isoDescriptor.pathTable2MSBoffs, 4 );

	SetPair32( &isoDescriptor.volumeSize, imageLen );

	// Write the descriptor
	writer->SeekToSector( 16 );
	writer->SetSubheader( cd::IsoWriter::SubEOL );
	writer->WriteBytes( &isoDescriptor, sizeof(cd::ISO_DESCRIPTOR),
		cd::IsoWriter::EdcEccForm1 );

	// Write descriptor terminator;
	memset( &isoDescriptor, 0x00, sizeof(cd::ISO_DESCRIPTOR) );
	isoDescriptor.header.type = 255;
	isoDescriptor.header.version = 1;
	strncpy( isoDescriptor.header.id, "CD001", 5 );

	writer->SetSubheader( cd::IsoWriter::SubEOF );
	writer->WriteBytes( &isoDescriptor, sizeof(cd::ISO_DESCRIPTOR),
		cd::IsoWriter::EdcEccForm1 );

	// Generate and write L-path table
	//unsigned char sectorBuff[2048*pathTableSectors];
	unsigned char *sectorBuff = new unsigned char[2048*pathTableSectors];
	memset( sectorBuff, 0x00, 2048*pathTableSectors );

	dirTree->GeneratePathTable( sectorBuff, false );
	writer->SetSubheader( cd::IsoWriter::SubData );

	for ( int i=0; i<pathTableSectors; i++ )
	{
		if ( i == pathTableSectors-1 )
		{
			writer->SetSubheader( cd::IsoWriter::SubEOF );
		}
		writer->WriteBytes( &sectorBuff[2048*i], 2048,
			cd::IsoWriter::EdcEccForm1 );
	}

	writer->SetSubheader( cd::IsoWriter::SubData );

	for( int i=0; i<pathTableSectors; i++ )
	{
		if ( i == pathTableSectors-1 )
		{
			writer->SetSubheader( cd::IsoWriter::SubEOF );
		}
		writer->WriteBytes( &sectorBuff[2048*i], 2048,
			cd::IsoWriter::EdcEccForm1 );
	}


	// Generate and write M-path table
	dirTree->GeneratePathTable( sectorBuff, true );
	writer->SetSubheader( cd::IsoWriter::SubData );

	for ( int i=0; i<pathTableSectors; i++ )
	{
		if ( i == pathTableSectors-1 )
		{
			writer->SetSubheader( cd::IsoWriter::SubEOF );
		}
		writer->WriteBytes( &sectorBuff[2048*i], 2048,
			cd::IsoWriter::EdcEccForm1 );
	}

	writer->SetSubheader( cd::IsoWriter::SubData );

	for ( int i=0; i<pathTableSectors; i++ )
	{
		if ( i == pathTableSectors-1 )
		{
			writer->SetSubheader( cd::IsoWriter::SubEOF );
		}
		writer->WriteBytes( &sectorBuff[2048*i], 2048,
			cd::IsoWriter::EdcEccForm1 );
	}

	writer->SetSubheader( cd::IsoWriter::SubData );
	delete[] sectorBuff;
}

iso::PathEntryClass::PathEntryClass() {

	dir_id = nullptr;
	dir_level = 0;
	dir_lba = 0;

	dir = nullptr;
	sub = nullptr;

}

iso::PathEntryClass::~PathEntryClass() {

	if ( sub != nullptr )
	{
		delete (PathTableClass*)sub;
	}

}

iso::PathTableClass::PathTableClass() {

}

iso::PathTableClass::~PathTableClass() {

	if ( entries.size() > 0 )
	{
		for ( int i=0; i<entries.size(); i++ )
		{
			delete entries[i];
		}
	}

}

unsigned char* iso::PathTableClass::GenTableData(unsigned char* buff, int msb) {

	if ( entries.size() == 0 )
	{
		return buff;
	}

	for ( int i=0; i<entries.size(); i++ )
	{
		*buff = entries[i]->dir_id->length();	// Directory identifier length
		buff++;
		*buff = 0;								// Extended attribute record length (unused)
		buff++;

		// Write LBA and directory number index
		*((int*)buff) = entries[i]->dir_lba;
		*((unsigned short*)(buff+4)) = entries[i]->dir_level;

		if ( msb )
		{
			cd::SwapBytes( buff, 4 );
			cd::SwapBytes( buff+4, 2 );
		}

		buff += 6;

		// Put identifier (nullptr if first entry)
		strncpy( (char*)buff, entries[i]->dir_id->c_str(),
			entries[i]->dir_id->length() );

		buff += 2*((entries[i]->dir_id->length()+1)/2);
	}

	for ( int i=0; i<entries.size(); i++ )
	{
		if ( entries[i]->sub )
		{
			buff = ((PathTableClass*)entries[i]->sub)->GenTableData( buff, msb );
		}

	}

	return buff;

}
