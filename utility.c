#include "utility.h"

bool32
AK_readEntireFileIntoMemory(file_info* Info)
{
  bool32 Success = false;
  int FileDescriptor = open(Info->FileName, O_RDONLY);
  if(FileDescriptor != -1)
    {
      size_t FileSizeBytes = lseek(FileDescriptor, 0, SEEK_END);
      if(FileSizeBytes != -1)
	{
	  Info->FileSize = FileSizeBytes;
	  void* FilePointer = mmap(0, FileSizeBytes, PROT_READ, MAP_PRIVATE,
				   FileDescriptor, 0);
	  if(FilePointer != MAP_FAILED)
	    {
	      Info->BasePointer = FilePointer;
	      Success = true;
	    }
	}
      close(FileDescriptor);
    }
  return(Success);
}

allocation_table_info
AK_ReadAllocationTable(char* Address)
{
  allocation_table_info TableInfo = {};
  TableInfo.BasePointer = Address;
  TableInfo.AllocationTableOffset = 0;
  TableInfo.BitTableOffset    = TableInfo.AllocationTableOffset + ALLOCATION_TABLE_SIZE;
  //NOTE At the end of the struct there could be padding in the file because the struct
  //     is not tightly packed. Now if we imagine this utility is actually inside akdb this could be fixed
  //     by forcing the pack, although there are other ways if we would like to keep the alignment for whatever
  //     reason. 
  TableInfo.BlocksStartOffset = (TableInfo.BasePointer + sizeof(AK_allocation_table)) - TableInfo.BasePointer;

  return(TableInfo);
}

int
AK_ReadInHeader(char* Buffer, AK_header* Headers)
{
  char* BufferBegin = Buffer;
  int bytesWritten;
  for(int HeaderIndex = 0;
      HeaderIndex < MAX_ATTRIBUTES;
      ++HeaderIndex)
    {
      AK_header* CurrentHeader = Headers + HeaderIndex;
      int AttributeTypeCode = CurrentHeader->type;
      if(AttributeTypeCode!=FREE_INT)
	{
	  const char* AttributeType = attribute_type_string[AttributeTypeCode];
	  bytesWritten= snprintf(Buffer, 128,
				 "\r\n\t-----ATTRIBUTE %d-----\r\n\r\n" \
				 "\tType: %s\r\n"				\
				 "\tName: %s\r\n",
				 HeaderIndex+1,
				 AttributeType,
				 CurrentHeader->att_name);
	  Buffer += bytesWritten;
	  for(int ConstraintIndex = 0;
	      ConstraintIndex < MAX_CONSTRAINTS;
	      ++ConstraintIndex)
	    {
	      bytesWritten= snprintf(Buffer, 512,
				     "\tConstraint:  % d\r\n "		\
				     "\t\tName: %s\r\n"			\
				     "\t\tCode: %s\r\n",
				     CurrentHeader->integrity[ConstraintIndex],
				     CurrentHeader->constr_name[ConstraintIndex],
				     CurrentHeader->constr_code[ConstraintIndex]);
	      Buffer += bytesWritten;
	    }
	}
      else
	{
	  bytesWritten = snprintf(Buffer, 128,
				  "\r\n\t-----ATTRIBUTE %d-----\r\n\r\n" \
				  "\tType: UNDEFINED\r\n",
				  HeaderIndex+1);
	  Buffer += bytesWritten;
	}      
    }  
  int TotalBytesWritten = Buffer - BufferBegin;
  return(TotalBytesWritten);
}

void
AK_PrintBlockInfo(allocation_table_info* TableInfo, int32 blockNumber)
{
  //TODO This obviously won't work for long term
  char Buffer[4096] = {};
  void* Pointer = TableInfo->BasePointer + TableInfo->BlocksStartOffset + sizeof(AK_block)*blockNumber;
  
  int address            = *((int*)Pointer);
  Pointer += sizeof(int);
  int BlockType          = *((int*)Pointer);
  Pointer += sizeof(int);
  int chained_with       = *((int*)Pointer);
  Pointer += sizeof(int);
  int AK_free_space      = *((int*)Pointer);
  Pointer += sizeof(int);
  int last_tuple_dict_id = *((int*)Pointer);
  Pointer += sizeof(int);
  if(BlockType == BLOCK_TYPE_FREE)
    {
      int BytesWritten =
	snprintf(Buffer, 256,
		 "-----BLOCK INFO-----\r\n\r\n"			  \
		 "Address:       % d\r\n"			  \
		 "Type:           %s\r\n"			  \
		 "Free Space:    % d\r\n",
		 address, block_type_string[BlockType - (BLOCK_TYPE_START+1)],
		 AK_free_space);
      Pointer += sizeof(AK_header) * MAX_ATTRIBUTES;
      Pointer += sizeof(AK_tuple_dict) * DATA_BLOCK_SIZE;
    }
  else if(BlockType == BLOCK_TYPE_NORMAL)
    {
      //NOTE We presume that BLOCK_TYPE_NORMAL always has chained_with -1,
      //     this might not be the case, but investigate why even this is a
      //     block type.
      int BytesWritten =
	snprintf(Buffer, 256,
		 "-----BLOCK INFO-----\r\n\r\n"			  \
		 "Address:       % d\r\n"			  \
		 "Type:           %s\r\n"			  \
		 "Free Space:    % d\r\n"			  \
		 "Last Tuple ID: % d\r\n",
		 address, block_type_string[BlockType - (BLOCK_TYPE_ENUM_START+1)],
		 AK_free_space, last_tuple_dict_id);

      AK_header* headers = (AK_header*)Pointer;
      Pointer += sizeof(AK_header) * MAX_ATTRIBUTES;
      BytesWritten = AK_ReadInHeader(Buffer + BytesWritten, headers);
    }
  else if(BlockType == BLOCK_TYPE_CHAINED)
    {
      int BytesWritten =
	snprintf(Buffer, 256,
		 "-----BLOCK INFO-----\r\n\r\n"			  \
		 "Address:       % d\r\n"			  \
		 "Type:           %s\r\n"			  \
		 "Chained with:  % d\r\n"			  \
		 "Free Space:    % d\r\n"			  \
		 "Last Tuple ID: % d\r\n",
		 address, block_type_string[BlockType - (BLOCK_TYPE_START+1)],
		 chained_with,
		 AK_free_space, last_tuple_dict_id);
      
      AK_header* headers = (AK_header*)Pointer;
      Pointer += sizeof(AK_header) * MAX_ATTRIBUTES;
      BytesWritten = AK_ReadInHeader(Buffer + BytesWritten, headers);
    }
  else
    {
      InvalidCodePath;
    }
  
  AK_tuple_dict* tuple_dict = (AK_tuple_dict*)Pointer;
  Pointer += sizeof(AK_tuple_dict) * DATA_BLOCK_SIZE;
  
  void* data = Pointer;
  puts(Buffer);
}

int
main(int ArgCount, char** Args)
{
  if(ArgCount == 3)
    {
      file_info FileInfo = {}; 
      FileInfo.FileName   = Args[1];
      //TODO Check if valid number and if in range
      int BlockNumber     = atoi(Args[2]);

      //NOTE We will not want to read the entire file into memory for the actual function forobvious
      //     reasons, but for now it's viable and we reduce the number of points of failure if we had
      //     streaming, every read is a potential failure case. The case could also be made that since
      //     this is a DBMS of some kind we can presume that the "file" is always available and lots or
      //     reads can be done without much worries. All this would be moot if we switch to live reading
      //     instead of this kind of static analysis but that is a different beast.
      if(AK_readEntireFileIntoMemory(&FileInfo))
	{
	  //NOTE We presume the 'Allocation Table' is at the start of the file. Note that we are
	  //     missing an actual spec for our db file(s), that would be nice to have exist.
	  allocation_table_info TableInfo = AK_ReadAllocationTable(FileInfo.BasePointer);
	  AK_PrintBlockInfo(&TableInfo, BlockNumber);
	}     
    }
  else
    {
      puts("Usage: utility <file> <block-number>");
    }
  
  return(0);
}
