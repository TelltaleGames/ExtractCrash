// Copyright (c) 2022 Telltale Games. All rights reserved.
// See LICENSE for usage, modification, and distribution terms.
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "zlib.h"

#define CHUNK 16384
#define DECOMPRESS_BUFFER 1024 * 1024 * 32

int32_t length_buf;
char str_buf[1024];

void FormatDir(std::string& dir) {
    char last_char = dir[dir.length() - 1];
    if (last_char != '\\' && last_char != '/')
        dir.append("\\");
}

void JoinPath(std::string& a, std::string& b, std::string& out) {
    FormatDir(a);

    out = a + b;
}

int32_t& ReadCrashLength(uint8_t*& source) {
    memcpy(&length_buf, source, 4);
    source += 4;

    return length_buf;
}

char* ReadCrashString(uint8_t*& source) {
    int32_t& str_len = ReadCrashLength(source);
    memset(str_buf, 0, sizeof(str_buf));
    memcpy(str_buf, source, str_len);
    str_buf[sizeof(str_buf) - 1] = '\0';
    source += str_len;

    return str_buf;
}

int main(int argc, const char** argv) {
    if(argc != 3) {
        printf("Usage: extractcrash <input> <output>\n");
        return 0;
    }
    FILE* source = fopen(argv[1], "rb");
    uint8_t* dest = new uint8_t[DECOMPRESS_BUFFER];
    uint64_t dest_pos = 0;
    std::string out_dir(argv[2]);

    // ----------------------------------
    // Decompress the input file
    // ----------------------------------
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    uint64_t have;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    // Decompress the file
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        printf("Decompress failed! %d\n", ret);
        return ret;
    }

    // Keep inflating until the stream ends or end of file
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);

        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }

        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        // Run inflate() on input until output buffer not full
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);

            assert(ret != Z_STREAM_ERROR);

            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR; // And fall through
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }

            have = CHUNK - strm.avail_out;
            memcpy(dest + dest_pos, out, have);
            dest_pos += have;
        } while (strm.avail_out == 0);

        // Done when inflate() says it's done
    } while (ret != Z_STREAM_END);

    // Cleanup the decompress stream
    (void)inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        printf("Decompress failed.");
        return Z_DATA_ERROR;
    }
    // ----------------------------------


    // ----------------------------------
    // Output to files
    // ----------------------------------
    uint8_t* pos = dest;
    int32_t file_index = 0;

    // Read the header
    std::string header1 = ReadCrashString(pos);
    std::string header2 = ReadCrashString(pos);
    pos += 12;

    // Output the files.
    // For now I'm just going off of 4 files.
    // There's a bit more data at the end but I'm not sure what it's for
    while (file_index < 4) {
        std::string file_name = ReadCrashString(pos);
        printf("Getting file: %s\n", file_name.c_str());

        int32_t file_len = ReadCrashLength(pos);
        char* file_contents = new char[file_len];
        memcpy(file_contents, pos, file_len);
        pos += file_len;

        file_index = ReadCrashLength(pos);
        
        // Output file
        std::string out_path;
        JoinPath(out_dir, file_name, out_path);
        FILE* out_file = fopen(out_path.c_str(), "wb");
        fwrite(file_contents, 1, file_len, out_file);
        fclose(out_file);

        delete[] file_contents;
    }

    delete[] dest;
    // ----------------------------------

    return 0;
}
