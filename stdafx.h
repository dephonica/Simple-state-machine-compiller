#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#include <map>
#include <string>

using namespace std;

typedef BYTE   uint8_t;
typedef WORD   uint16_t;
typedef DWORD  uint32_t;

#define DEF_STRING_SIZE         1024
#define LOG_NAME                "ssmc.log"

#include "version.h"
#include "log.cpp"

CLog log(LOG_NAME);
#define LOG  log.Log

#include "instream.cpp"
#include "lex.cpp"
#include "crc32.cpp"

#include "./astyle/astyle_main.cpp"

///////////////////////////////////////////////////////////////////////
/// Program parameters
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
/// Other definitions
///////////////////////////////////////////////////////////////////////

#define STATE_OUTER             1
#define STATE_MUSTBE_REM        2
#define STATE_MUSTBE_STATEDEF   3
#define STATE_REM               4
#define STATE_PLAIN             5
#define STATE_MUSTBE_OUTREM     6

#define HEADER_STATE_NAME       "header"

// Header tokens
const uint32_t h_header  = hash(HEADER_STATE_NAME);
const uint32_t h_typedef = hash("typedef");
const uint32_t h_private = hash("private");
const uint32_t h_public  = hash("public");
const uint32_t h_impl    = hash("implementation");
const uint32_t h_shut    = hash("shutdown");
const uint32_t h_goto    = hash("goto");

struct goto_desc
{
        vector<string> ins;
        string tstate;
};

struct state_desc
{
        // Inputs section
        map<string, string> inputs;

        // Outputs section
        string simply;
        vector<goto_desc> outputs;

        // C++ code section
        string s_typedef, s_impl, s_private, s_public;
};

#include "code_cpp.cpp"
