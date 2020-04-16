/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#ifndef __IAR_SYSTEMS_ICC__
#include <strings.h>
#endif

#include <ti/net/common.h>

#include "huffman.h"
#include "http2utils.h"
#include "http2std.h"
#include "hpack.h"

#define ENCODED_INT_VALUE_MAX         (0x80)
#define ENCODED_INT_BIT_MASK          (0x7F)
#define ENCODED_INT_BIT_LEN           (7)
#define ENCODED_INT_CONTINUATION_FLAG (0x80)

/* Header field representation bit patterns */
#define HEADER_FIELD_PATTERN_INDEXED                   (0x80)
#define HEADER_FIELD_PATTERN_INDEXED_NAME              (0x0)
#define HEADER_FIELD_PATTERN_NOTINDEXED                (0x0)
#define HEADER_FIELD_PATTERN_HUFFMAN_ENCODE_OFF        (0x0)
#define HEADER_FIELD_PATTERN_HUFFMAN_ENCODE_ON         (0x80)
#define HEADER_FIELD_PATTERN_INCREMENTAL_INDEXING      (0x40)
#define HEADER_FIELD_PATTERN_DYNAMIC_TABLE_UPDATE      (0x20)
#define HEADER_FIELD_PATTERN_DYNAMIC_TABLE_UPDATE_MASK (0xE0)

/* Header field representation bit prefix lengths */
#define HEADER_FIELD_PREFIX_INDEXED_LEN              (7)
#define HEADER_FIELD_PREFIX_INDEXED_NAME_LEN         (4)
#define HEADER_FIELD_PREFIX_NOTINDEXED_LEN           (4)
#define HEADER_FIELD_PREFIX_STRLEN_LEN               (7)
#define HEADER_FIELD_PREFIX_INCREMENTAL_INDEXING_LEN (6)
#define HEADER_FIELD_PREFIX_DYNAMIC_TABLE_UPDATE_LEN (5)

/* Header field representation bit prefix */
#define HEADER_FIELD_PREFIX_NOTINDEXED       (0)

/* See RFC 7541, section 4.1 */
#define DYNAMIC_TABLE_ENTRY_OVERHEAD (32)

/* Encoding array length */
#define ENCODE_INT_ARRAY_LEN (6)
#define STATIC_TABLE_LEN     (61)
#define HTTP2CLI_FIELD_LEN   (sizeof(HTTP2Hdr_Field))
#define NULL_CHAR_LEN        (1)

typedef struct TableEntry {
    char    *name;
    char    *value;
} TableEntry;

struct DynamicTableEntry {
    DynamicTableEntry *next;
    TableEntry entry;
    uint32_t entrySize;
};

const TableEntry staticTable[STATIC_TABLE_LEN] = {
    {":authority", ""},                        /* Index: 1 */
    {":method", "GET"},                        /* Index: 2 */
    {":method", "POST"},                       /* Index: 3 */
    {":path", "/"},                            /* Index: 4 */
    {":path", "/index.html"},                  /* Index: 5 */
    {":scheme", "http"},                       /* Index: 6 */
    {":scheme", "https"},                      /* Index: 7 */
    {":status", "200"},                        /* Index: 8 */
    {":status", "204"},                        /* Index: 9 */
    {":status", "206"},                        /* Index: 10 */
    {":status", "304"},                        /* Index: 11 */
    {":status", "400"},                        /* Index: 12 */
    {":status", "404"},                        /* Index: 13 */
    {":status", "500"},                        /* Index: 14 */
    {"accept-charset", ""},                    /* Index: 15 */
    {"accept-encoding", "gzip, deflate"},      /* Index: 16 */
    {"accept-language", ""},                   /* Index: 17 */
    {"accept-ranges", ""},                     /* Index: 18 */
    {"accept", ""},                            /* Index: 19 */
    {"access-control-allow-origin", ""},       /* Index: 20 */
    {"age", ""},                               /* Index: 21 */
    {"allow", ""},                             /* Index: 22 */
    {"authorization", ""},                     /* Index: 23 */
    {"cache-control", ""},                     /* Index: 24 */
    {"content-disposition", ""},               /* Index: 25 */
    {"content-encoding", ""},                  /* Index: 26 */
    {"content-language", ""},                  /* Index: 27 */
    {"content-length", ""},                    /* Index: 28 */
    {"content-location", ""},                  /* Index: 29 */
    {"content-range", ""},                     /* Index: 30 */
    {"content-type", ""},                      /* Index: 31 */
    {"cookie", ""},                            /* Index: 32 */
    {"date", ""},                              /* Index: 33 */
    {"etag", ""},                              /* Index: 34 */
    {"expect", ""},                            /* Index: 35 */
    {"expires", ""},                           /* Index: 36 */
    {"from", ""},                              /* Index: 37 */
    {"host", ""},                              /* Index: 38 */
    {"if-match", ""},                          /* Index: 39 */
    {"if-modified-since", ""},                 /* Index: 40 */
    {"if-none-match", ""},                     /* Index: 41 */
    {"if-range", ""},                          /* Index: 42 */
    {"if-unmodified-since", ""},               /* Index: 43 */
    {"last-modified", ""},                     /* Index: 44 */
    {"link", ""},                              /* Index: 45 */
    {"location", ""},                          /* Index: 46 */
    {"max-forwards", ""},                      /* Index: 47 */
    {"proxy-authenticate", ""},                /* Index: 48 */
    {"proxy-authorization", ""},               /* Index: 49 */
    {"range", ""},                             /* Index: 50 */
    {"referer", ""},                           /* Index: 51 */
    {"refresh", ""},                           /* Index: 52 */
    {"retry-after", ""},                       /* Index: 53 */
    {"server", ""},                            /* Index: 54 */
    {"set-cookie", ""},                        /* Index: 55 */
    {"strict-transport-security", ""},         /* Index: 56 */
    {"transfer-encoding", ""},                 /* Index: 57 */
    {"user-agent", ""},                        /* Index: 58 */
    {"vary", ""},                              /* Index: 59 */
    {"via", ""},                               /* Index: 60 */
    {"www-authenticate", ""},                  /* Index: 61 */
};

/*
 *  ======== toLowerCase ========
 *  Converts chars in string to lower case
 *
 *  Returns the length of the string
 */
static size_t toLowerCase(char *str)
{
    size_t i = 0;

    if (str) {
        for (i = 0; str[i] != '\0'; i++) {
            str[i] = tolower(str[i]);
        }
    }

    return (i);
}

/*
 *  ======== createDynamicTableEntry ========
 *  Create an entry instance for dynamic table and initialize it
 */
static DynamicTableEntry *createDynamicTableEntry(DynamicTableEntry *next,
        char *name, uint32_t nameLen, char *value, uint32_t valueLen,
        uint32_t entrySize)
{
    DynamicTableEntry *dEntry;

    dEntry = (DynamicTableEntry *) calloc(1, sizeof(DynamicTableEntry));
    if (!dEntry) {
        goto createError;
    }

    dEntry->entry.name = HTTP2Utils_stringDuplicate(name, nameLen);
    if (!dEntry->entry.name) {
        goto createError;
    }

    dEntry->entry.value = HTTP2Utils_stringDuplicate(value, valueLen);
    if (!dEntry->entry.value) {
        goto createError;
    }

    dEntry->next = next;
    dEntry->entrySize = entrySize;

    return (dEntry);

createError:
    if (dEntry) {
        if (dEntry->entry.name) {
            free(dEntry->entry.name);
        }

        if (dEntry->entry.value) {
            free(dEntry->entry.value);
        }

        free(dEntry);
    }

    return (NULL);
}

/*
 *  ======== freeDynamicTable ========
 *  Free entries that were allocated on the dynamic table
 */
static void freeDynamicTable(DynamicTableEntry *freeEntry)
{
    if (freeEntry) {
        if (freeEntry->next) {
            freeDynamicTable(freeEntry->next);
        }

        free(freeEntry->entry.name);
        free(freeEntry->entry.value);
        free(freeEntry);
    }
}

/*
 *  ======== evictDynamicTable ========
 *  Evict entries from the dynamic table
 */
static void evictDynamicTable(HPACK_Handle hpack, uint32_t newTableSize)
{
    DynamicTableEntry *freeEntry;
    DynamicTableEntry *prevEntry = NULL;
    uint32_t entrySize;

    hpack->decoderDynamicTableMaxSize = newTableSize;
    hpack->decoderDynamicTableSize = 0;

    freeEntry = hpack->decoderDynamicTable;
    if (freeEntry) {
        entrySize = freeEntry->entrySize;
        while (entrySize < newTableSize) {
            hpack->decoderDynamicTableSize += freeEntry->entrySize;
            prevEntry = freeEntry;
            freeEntry = freeEntry->next;
            if (!freeEntry) {
                break;
            }

            entrySize += freeEntry->entrySize;
        }

        if (freeEntry) {
            freeDynamicTable(freeEntry);
        }

        if (prevEntry) {
            prevEntry->next = NULL;
        }
    }
}

/*
 *  ======== addDynamicTableEntry ========
 *  Add an entry to the dynamic table
 */
static int addDynamicTableEntry(HPACK_Handle hpack, char *name, char *value)
{
    int ret = 0;
    uint32_t entrySize;
    uint32_t nameLen;
    uint32_t valueLen;
    DynamicTableEntry *entry;

    nameLen = strlen(name);
    valueLen = strlen(value);
    entrySize = nameLen + valueLen + DYNAMIC_TABLE_ENTRY_OVERHEAD;

    if ((hpack->decoderDynamicTableSize + entrySize)
            > hpack->decoderDynamicTableMaxSize) {
        evictDynamicTable(hpack,
                (hpack->decoderDynamicTableMaxSize - entrySize));
    }

    entry = createDynamicTableEntry(hpack->decoderDynamicTable, name, nameLen,
            value, valueLen, entrySize);
    if (entry) {
        hpack->decoderDynamicTable = entry;
        hpack->decoderDynamicTableSize += entrySize;
    }
    else {
        ret = HPACK_EINSUFFICIENTHEAP;
    }

    return (ret);
}

/*
 *  ======== getEntry ========
 *  Get an entry from static/dynamic table for the input index
 */
const TableEntry *getEntry(HPACK_Handle hpack, uint32_t index)
{
    const TableEntry *entry = NULL;
    DynamicTableEntry *dEntry = NULL;
    uint32_t i;

    if (index <= STATIC_TABLE_LEN) {
        entry = &staticTable[index - 1];
    }
    else {
        index -= STATIC_TABLE_LEN;
        dEntry = hpack->decoderDynamicTable;

        for (i = 1; dEntry != NULL; i++) {
            if (i == index) {
                entry = &(dEntry->entry);
                break;
            }
            dEntry = dEntry->next;
        }
    }

    return (entry);
}

/*
 *  ======== encodeInt ========
 *  Encoded integer representation
 *
 *  Encode the 32 bit 'value' into the integer representation
 *  starting with a bit 'pattern' that indicates encoding type.
 *  The 'nbits' param contains number of available bits for
 *  encoding the 'value' in the first octet.
 *
 *  The encoded data is returned by appending it to the 'headerBlock'.
 *
 *  Based on RFC 7541, Section 5.1 psuedo code.
 *
 *  Returns the new length of the 'headerBlock' on success or < 0 on failure.
 */
static int encodeInt(uint8_t pattern, uint8_t nbits, uint32_t value,
        uint8_t **headerBlock, uint32_t len)
{
    int nOctets;
    uint8_t enInt[ENCODE_INT_ARRAY_LEN] = {0};
    uint8_t mask;
    uint8_t *hb = NULL;
    int i = 1;

    mask = (1 << nbits) - 1;
    if (value < mask) {
        enInt[0] = pattern | value;
    }
    else {
        enInt[0] = pattern | mask;
        value = value - mask;
        while (value >= ENCODED_INT_VALUE_MAX) {
            enInt[i++] = ENCODED_INT_CONTINUATION_FLAG
                    | (ENCODED_INT_BIT_MASK & value);
            value = value / ENCODED_INT_VALUE_MAX;
        }
        enInt[i++] = value;
    }

    hb = *headerBlock;
    hb = (uint8_t *) realloc(hb, (len + i));
    if (hb) {
        memcpy(hb + len, enInt, i);
        *headerBlock = hb;
        nOctets = len + i;
    }
    else {
        free(*headerBlock);
        *headerBlock = NULL;
        nOctets = HPACK_EINSUFFICIENTHEAP;
    }

    return (nOctets);
}

/*
 *  ======== decodeInt ========
 *  Decoding the integer representation
 *
 *  Decodes the 'headerBlock' octets by extracting the 'nbits'
 *  from the first octet. If the 'value' extracted is the max
 *  value possible for 'nbits', then more encoded data is available
 *  in the following octets. For each following octet, remove the most
 *  significant bit and concat to the 'value'.
 *
 *  The 'len' param contains remaining 'headerBlock' octets available.
 *
 *  Based on RFC 7541, Section 5.1 psuedo code.
 *
 *  Returns the number of octets decoded on success or < 0 on failure.
 */
static int decodeInt(uint8_t *headerBlock, uint32_t len, uint8_t nbits,
        uint32_t *value)
{
    int nOctets;
    uint8_t mask;

    if (!len) {
        return (HPACK_EINSUFFICIENTDATA);
    }

    mask = (1 << nbits) - 1;
    *value = (*headerBlock & mask);
    nOctets = 1;

    /* more integer data available */
    if (*value == mask) {
        do {
            if (!(--len)) {
                nOctets = HPACK_EINSUFFICIENTDATA;
                break;
            }

            headerBlock++;
            *value += ((*headerBlock & ENCODED_INT_BIT_MASK)
                    << ((nOctets - 1) * ENCODED_INT_BIT_LEN));
            nOctets++;
        } while (*headerBlock & ENCODED_INT_CONTINUATION_FLAG);
    }

    return (nOctets);
}

/*
 *  ======== encodeStr ========
 *  Encoded string literal representation
 *
 *  The encoded data is a sequence of octets with integer representation
 *  of the string length followed by string literal's octet. This
 *  implementation does not encode the string literals using Huffman
 *  encoding.
 *
 *  The encoded data is returned by appending it to the 'headerBlock'.
 *
 *  Based on RFC 7541, Section 5.2.
 *
 *  Returns the new length of the 'headerBlock' on success or < 0 on failure.
 */
static int encodeStr(const char *str, uint8_t **headerBlock, uint32_t len)
{
    int nOctets;
    size_t slen;
    uint8_t *hb = NULL;

    slen = strlen(str);

    /* Huffman encoding turned off */
    nOctets = encodeInt(HEADER_FIELD_PATTERN_HUFFMAN_ENCODE_OFF,
            HEADER_FIELD_PREFIX_STRLEN_LEN, slen, headerBlock, len);
    if ((nOctets > 0) && slen) {
        hb = *headerBlock;
        hb = (uint8_t *) realloc(hb, nOctets + slen);
        if (hb) {
            memcpy(hb + nOctets, str, slen);
            *headerBlock = hb;
            nOctets += slen;
        }
        else {
            free(*headerBlock);
            *headerBlock = NULL;
            nOctets = HPACK_EINSUFFICIENTHEAP;
        }
    }

    return (nOctets);
}

/*
 *  ======== decodeStr ========
 *  Decoding the string literal representation
 *
 *  The 'headerBlock' contains a sequence of octets with string length
 *  followed by string literals. The integer representation of length is
 *  decoded and is used to read the string literals. If the string is
 *  Huffman encoded as indicated by MSB on the first octet, the string is
 *  decoded using the Huffman Table (RFC 7541, Appendix B).
 *
 *  The decoded string is returned through 'str' and number of decoded
 *  octets is returned.
 *
 *  The 'len' param contains remaining 'headerBlock' octets available.
 *
 *  Based on RFC 7541, Section 5.2.
 *
 *  Returns the number of octets decoded on success or < 0 on failure.
 */
static int decodeStr(uint8_t *headerBlock, uint32_t len, char **str)
{
    int ret;
    int nOctets;
    uint32_t slen;
    char *s;
    bool huffman;

    if (!len) {
        return (HPACK_EINSUFFICIENTDATA);
    }

    /* Is Huffman encoded string */
    huffman = (*headerBlock & HEADER_FIELD_PATTERN_HUFFMAN_ENCODE_ON);

    nOctets = decodeInt(headerBlock, len, HEADER_FIELD_PREFIX_STRLEN_LEN,
            &slen);
    if (nOctets > 0) {
        len -= nOctets;
        if (len < slen) {
            nOctets = HPACK_EINSUFFICIENTDATA;
        }
        else {
            s = (char *)malloc(slen + NULL_CHAR_LEN);
            if (s) {
                headerBlock += nOctets;
                memcpy(s, headerBlock, slen);

                nOctets += slen;

                if (huffman) {
                    ret = Huffman_decode((uint8_t *)s, slen, str);
                    if (ret < 0) {
                        if (ret == HUFFMAN_EINSUFFICIENTHEAP) {
                            nOctets = HPACK_EINSUFFICIENTHEAP;
                        }
                        else {
                            nOctets = HPACK_EHUFFMANDECODEFAIL;
                        }
                        *str = NULL;
                    }

                    free(s);
                    s = NULL;
                }
                else {
                    s[slen] = '\0';
                    *str = s;
                }
            }
            else {
                *str = NULL;
                nOctets = HPACK_EINSUFFICIENTHEAP;
            }
        }
    }

    return (nOctets);
}

/*
 *  ======== HPACK_construct ========
 */
void HPACK_construct(HPACK_Struct *hpack)
{
    if (hpack) {
        memset(hpack, 0, sizeof(HPACK_Struct));
        hpack->decoderDynamicTableMaxSize =
                HTTP2Std_SETTINGS_HEADER_TABLE_SIZE_DEFAULT;
    }
}

/*
 *  ======== HPACK_destruct ========
 */
void HPACK_destruct(HPACK_Struct *hpack)
{
    if (hpack) {
        freeDynamicTable(hpack->decoderDynamicTable);
        memset(hpack, 0, sizeof(HPACK_Struct));
    }
}

/*
 *  ======== HPACK_encode ========
 *  Encode the input header list to binary header block
 *
 *  Based on RFC 7541, Sections 6.1, 6.2.2 and 6.3
 *
 *  Returns the 'headerBlock' length on success or < 0 on failure
 */
int HPACK_encode(HPACK_Handle hpack, HTTP2Hdr_Field *headersList, uint32_t len,
        uint8_t **headerBlock)
{
    uint32_t i;
    uint32_t j;
    uint32_t index;
    int hbcnt = 0;
    size_t enlen;
    size_t evlen;
    size_t hnlen;
    size_t hvlen;
    uint8_t *hb = NULL;
    HTTP2Hdr_Field header;
    TableEntry entry;

    (void)hpack;

    for (i = 0; i < len; i++) {
        header = headersList[i];
        hvlen = strlen(header.value);

        /* Convert header names to lower case as per RFC 7540, Section 8.1.2 */
        hnlen = toLowerCase(header.name);

        for (j = 0; j < STATIC_TABLE_LEN; j++) {
            entry = staticTable[j];
            index = j + 1; /* Index value starts from 1 */

            enlen = strlen(entry.name);
            if (enlen != hnlen) {
                /* Inner loop */
                continue;
            }

            if (strncmp(entry.name, header.name, enlen + NULL_CHAR_LEN) == 0) {

                evlen = strlen(entry.value);
                if (evlen) {

                    if ((evlen == hvlen)
                            && (strncasecmp(entry.value, header.value,
                            evlen + NULL_CHAR_LEN) == 0)) {

                        /* Indexed Header Field */
                        hbcnt = encodeInt(HEADER_FIELD_PATTERN_INDEXED,
                                HEADER_FIELD_PREFIX_INDEXED_LEN, index,
                                &hb, hbcnt);
                        if (hbcnt < 0) {
                            goto error;
                        }

                        goto done;
                    }
                    else {

                        /* Inner loop for next static table entry */
                        continue;
                    }
                }
                else {

                    /* Literal Header Field without Indexing -- Indexed name */
                    hbcnt = encodeInt(HEADER_FIELD_PATTERN_INDEXED_NAME,
                            HEADER_FIELD_PREFIX_INDEXED_NAME_LEN, index,
                            &hb, hbcnt);
                    if (hbcnt < 0) {
                        goto error;
                    }

                    hbcnt = encodeStr(header.value, &hb, hbcnt);
                    if (hbcnt < 0) {
                        goto error;
                    }

                    goto done;
                }
            }
        }

        /* Literal Header Field without Indexing -- new name */
        hbcnt = encodeInt(HEADER_FIELD_PATTERN_NOTINDEXED,
                HEADER_FIELD_PREFIX_NOTINDEXED_LEN,
                HEADER_FIELD_PREFIX_NOTINDEXED, &hb, hbcnt);
        if (hbcnt < 0) {
            goto error;
        }

        hbcnt = encodeStr(header.name, &hb, hbcnt);
        if (hbcnt < 0) {
            goto error;
        }

        hbcnt = encodeStr(header.value, &hb, hbcnt);
        if (hbcnt < 0) {
            goto error;
        }

done:
        continue;
    }

    *headerBlock = hb;

error:
    return (hbcnt);
}

/*
 *  ======== HPACK_decode ========
 *  Decode the input binary header block and create a list of headers
 *
 *  Based on RFC 7541, Sections 6.1, 6.2.2 and 6.3
 *
 *  Returns the number of headers in 'headersList' on success or < 0 on failure.
 */
int HPACK_decode(HPACK_Handle hpack, uint8_t *headerBlock, uint32_t len,
        HTTP2Hdr_Field **headersList)
{
    int nOctets;
    int hlcnt = 0;
    uint8_t prefixLen;
    uint8_t compressionType;
    uint8_t *hbEnd;
    uint32_t index;
    char *name;
    char *value;

    const TableEntry *entry;

    hbEnd = headerBlock + len;

    *headersList = NULL;

    while (headerBlock < hbEnd) {
        /* set to NULL to avoid invalid/double free */
        name = NULL;
        value = NULL;

        compressionType = *headerBlock;
        if (compressionType & HEADER_FIELD_PATTERN_INDEXED) {
            /* Indexed Header Field */
            nOctets = decodeInt(headerBlock, (hbEnd - headerBlock),
                    HEADER_FIELD_PREFIX_INDEXED_LEN, &index);
            if (nOctets < 0) {
                goto error;
            }
            headerBlock += nOctets;

            if (!index) {
                nOctets = HPACK_EDECODEERROR;
                goto error;
            }

            entry = getEntry(hpack, index);
            if (!entry) {
                nOctets = HPACK_EDECODEERROR;
                goto error;
            }

            name = HTTP2Utils_stringDuplicate(entry->name, strlen(entry->name));
            if (name == NULL) {
                nOctets = HPACK_EINSUFFICIENTHEAP;
                goto error;
            }

            value = HTTP2Utils_stringDuplicate(entry->value,
                    strlen(entry->value));
            if (value == NULL) {
                nOctets = HPACK_EINSUFFICIENTHEAP;
                goto error;
            }

            hlcnt = HTTP2Hdr_add(headersList, hlcnt, name, value);
            if (hlcnt < 0) {
                nOctets = HPACK_EINSUFFICIENTHEAP;
                goto error;
            }
        }
        else if ((compressionType
                & HEADER_FIELD_PATTERN_DYNAMIC_TABLE_UPDATE_MASK)
                == HEADER_FIELD_PATTERN_DYNAMIC_TABLE_UPDATE) {
            /* Dynamic table size update */
            index = 0;

            nOctets = decodeInt(headerBlock, (hbEnd - headerBlock),
                    HEADER_FIELD_PREFIX_DYNAMIC_TABLE_UPDATE_LEN, &index);
            if (nOctets < 0) {
                goto error;
            }
            headerBlock += nOctets;

            if (index > HTTP2Std_SETTINGS_HEADER_TABLE_SIZE_DEFAULT) {
                nOctets = HPACK_EDECODEERROR;
                goto error;
            }
            else if (index >= hpack->decoderDynamicTableMaxSize) {
                hpack->decoderDynamicTableMaxSize = index;
            }
            else {
                evictDynamicTable(hpack, index);
            }

            continue;
        }
        else {
            if (compressionType & HEADER_FIELD_PATTERN_INCREMENTAL_INDEXING) {
                prefixLen = HEADER_FIELD_PREFIX_INCREMENTAL_INDEXING_LEN;
            }
            else {
                prefixLen = HEADER_FIELD_PREFIX_NOTINDEXED_LEN;
            }

            nOctets = decodeInt(headerBlock, (hbEnd - headerBlock), prefixLen,
                    &index);
            if (nOctets < 0) {
                goto error;
            }
            headerBlock += nOctets;

            if (index) {

                entry = getEntry(hpack, index);
                if (!entry) {
                    nOctets = HPACK_EDECODEERROR;
                    goto error;
                }

                name = HTTP2Utils_stringDuplicate(entry->name,
                        strlen(entry->name));
                if (name == NULL) {
                    nOctets = HPACK_EINSUFFICIENTHEAP;
                    goto error;
                }
            }
            else {
                nOctets = decodeStr(headerBlock, (hbEnd - headerBlock), &name);
                if (nOctets < 0) {
                    goto error;
                }
                headerBlock += nOctets;
            }

            nOctets = decodeStr(headerBlock, (hbEnd - headerBlock), &value);
            if (nOctets < 0) {
                goto error;
            }
            headerBlock += nOctets;

            hlcnt = HTTP2Hdr_add(headersList, hlcnt, name, value);
            if (hlcnt < 0) {
                nOctets = HPACK_EINSUFFICIENTHEAP;
                goto error;
            }

            if (compressionType & HEADER_FIELD_PATTERN_INCREMENTAL_INDEXING) {
                nOctets = addDynamicTableEntry(hpack, name, value);
                if (nOctets < 0) {
                    goto error;
                }
            }
        }
    }

    return (hlcnt);

error:
    if (name) {
        free(name);
    }

    if (value) {
        free(value);
    }

    if (hlcnt > 0) {
        HTTP2Hdr_free(headersList, hlcnt);
    }

    return (nOctets);
}
