#include <stdint.h>
#include <stdbool.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define S8_MIN  ((s8) 0x80)
#define S16_MIN ((s16)0x8000)
#define S32_MIN ((s32)0x80000000)
#define S64_MIN ((s64)0x8000000000000000LL)

#define S8_MAX  ((s8) 0x7F)
#define S16_MAX ((s16)0x7FFF)
#define S32_MAX ((s32)0x7FFFFFFF)
#define S64_MAX ((s64)0x7FFFFFFFFFFFFFFFLL)

typedef s64 smm;

#define SMM_MIN S64_MIN
#define SMM_MAX S64_MAX

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define U8_MAX  ((u8) 0xFF)
#define U16_MAX ((u16)0xFFFF)
#define U32_MAX ((u32)0xFFFFFFFF)
#define U64_MAX ((u64)0xFFFFFFFFFFFFFFFFULL)

typedef u64 umm;

#define UMM_MAX U64_MAX

typedef float f32;
typedef double f64;

typedef struct String
{
	u32 len;
	u8* data;
} String;

#define STRING(S) (String){ .data = (u8*)(S), .len = sizeof(S)-1 }

#define ARRAY_LEN(A) (sizeof(A)/sizeof(0[A]))

#define CONCAT_(A, B) A##B
#define CONCAT(A, B) CONCAT_(A, B)

#define NOT_IMPLEMENTED (*(volatile int*)0 = 0)

typedef void (*Game_Init_Func)(bool is_reload);
typedef void (*Game_Tick_Func)(void);
