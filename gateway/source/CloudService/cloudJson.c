/******************************************************************************

 @file cloudJson.c

 @brief

 Group: WCS LPC
 Target Device: CC13xx CC32xx

 ******************************************************************************

 Copyright (c) 2016-2017, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name:
 Release Date:
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ThirdParty/jsmn.h>
#include <Common/commonDefs.h>
#include <Utils/uart_term.h>
#include "cloudJson.h"


char* jsonParseIn(char* payload_str, char* findToken)
{
    char *retToken = NULL;
    char *tempToken;
    int tokenNum;
    int tokenLen = 0;
    jsmn_parser jParser;
    jsmntok_t tokens[56]; /* We expect no more than 25 tokens */

    jsmn_init(&jParser);
    tokenNum = jsmn_parse(&jParser, payload_str, strlen(payload_str), tokens, sizeof(tokens)/sizeof(tokens[0]));
    Report("JSON Tokens: %d\r\n", tokenNum-1);
    for (int tokenIdx = 0; tokenIdx < tokenNum; tokenIdx++)
    {
        tokenLen = tokens[tokenIdx].end - tokens[tokenIdx].start;
        tempToken = (char*)(payload_str + tokens[tokenIdx].start);
        if(strncmp(tempToken, findToken, tokenLen) == 0)
        {
            tokenIdx++;
            tokenLen = tokens[tokenIdx].end-tokens[tokenIdx].start;
            retToken = (char*)malloc(tokenLen + 1);
            strncpy(retToken, payload_str + tokens[tokenIdx].start, tokenLen);
            retToken[tokenLen] = '\0';
            break;
        }
    }
    return retToken;
}
