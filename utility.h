#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define false 0
#define true  !false

#ifdef __clang__
#define Assert(Expresion) if(!(Expresion)){ __builtin_trap();}
#else
#define Assert(Expresion) if(!(Expresion)){(*((int*)0) = 0);}
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")

#define KILOBYTES(request) 1024*request
#define MEGABYTES(request) 1024*KILOBYTES(request)

//NOTE These defines are here now but should really be available externally, for example.
//     A lot of this depends if this code will be merged with akdb at some point
#define MAX_ATT_NAME 255
#define MAX_CONSTR_CODE 255
#define MAX_CONSTRAINTS 5
#define MAX_CONSTR_NAME 255
#define MAX_ATTRIBUTES 10
#define DATA_BLOCK_SIZE 500
#define DATA_ENTRY_SIZE 10

#define FREE_INT -10

#define BITNSLOTS(nb)   ((int)(nb + CHAR_BIT - 1) / CHAR_BIT)

#define GENERATE_BLOCK_TYPES(TYPE)		       \
  TYPE(BLOCK_TYPE_FREE),			       \
    TYPE(BLOCK_TYPE_NORMAL),			       \
    TYPE(BLOCK_TYPE_CHAINED),			       \

#define BLOCK_TYPE(TYPE) TYPE
#define BLOCK_TYPE_STRING(TYPE) #TYPE

enum block_types
  {
    BLOCK_TYPE_ENUM_START = -2,
    GENERATE_BLOCK_TYPES(BLOCK_TYPE)
  };

//NOTE Presumed that block types start from 0 (this is the case for now)
static const char* block_type_string[] =
  {
    GENERATE_BLOCK_TYPES(BLOCK_TYPE_STRING)
    BLOCK_TYPE_COUNT
  };


#define GENERATE_ATTRIBUTE_TYPES(TYPE)	       \
  TYPE(TYPE_INTERNAL),			       \
    TYPE(TYPE_INT),			       \
    TYPE(TYPE_FLOAT),			       \
    TYPE(TYPE_NUMBER),			       \
    TYPE(TYPE_VARCHAR),			       \
    TYPE(TYPE_DATE),			       \
    TYPE(TYPE_DATETIME),		       \
    TYPE(TYPE_TIME),			       \
    TYPE(TYPE_BLOB),			       \
    TYPE(TYPE_BOOL),			       \

#define ATTRIBUTE_TYPE(TYPE) TYPE
#define ATTRIBUTE_TYPE_STRING(TYPE) #TYPE

enum attribute_types
  {
    GENERATE_ATTRIBUTE_TYPES(ATTRIBUTE_TYPE)
  };

static const char* attribute_type_string[] =
  {
    GENERATE_ATTRIBUTE_TYPES(ATTRIBUTE_TYPE_STRING)
  };

typedef uint32_t uint32;
typedef  int32_t int32;
typedef  int32   bool32;

typedef struct
{
  char*  FileName;
  size_t FileSize;
  void*  BasePointer;
} file_info;
  
typedef struct
{
  int type;
  int address;
  int size;
} AK_tuple_dict;

typedef struct
{
  int  type;
  char att_name[MAX_ATT_NAME];
  int  integrity[MAX_CONSTRAINTS];
  char constr_name[MAX_CONSTRAINTS][MAX_CONSTR_NAME];
  char constr_code[MAX_CONSTRAINTS][MAX_CONSTR_CODE];
} AK_header;

typedef struct
{
  int address;
  int type;
  int chained_with;
  int AK_free_space;
  int last_tuple_dict_id;

  AK_header     header[MAX_ATTRIBUTES];
  AK_tuple_dict tuple_dict[DATA_BLOCK_SIZE];
  unsigned char data[DATA_BLOCK_SIZE * DATA_ENTRY_SIZE];
} AK_block;


#define DB_FILE_SIZE_EX 200
#define DB_FILE_BLOCKS_NUM_EX (int)(MEGABYTES(DB_FILE_SIZE_EX) / sizeof(AK_block))

#define ALLOCATION_TABLE_SIZE DB_FILE_BLOCKS_NUM_EX
#define BIT_TABLE_SIZE BITNSLOTS(DB_FILE_BLOCKS_NUM_EX)

typedef struct
{
  unsigned int  allocationtable[ALLOCATION_TABLE_SIZE];
  unsigned char bittable[BIT_TABLE_SIZE];

  int  last_allocated;
  int  last_initialized;
  int  prepared;
  long ltime;
} AK_allocation_table;

typedef struct
{
  void* BasePointer;
  int32 AllocationTableOffset;
  int32 BitTableOffset;
  int32 BlocksStartOffset;
} allocation_table_info;
