// Copyright (c) 2022 Telltale Games. All rights reserved.
// See LICENSE for usage, modification, and distribution terms.
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "zlib.h"

#define CHUNK 16384
#define DECOMPRESS_BUFFER 1024 * 1024 * 32

void FormatDir(std::string& dir)
{
    char last_char = dir[dir.length() - 1];
    if (last_char != '\\' && last_char != '/')
        dir.append("\\");
}

void JoinPath(std::string& a, std::string& b, std::string& out)
{
    FormatDir(a);

    out = a + b;
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

    // Read the header
    const unsigned header_len = 540;
    char header[header_len];
    memcpy(header, pos, header_len);
    pos += header_len;
    uint64_t offset = 0;
    int32_t file_index = 0;

    // Output the files.
    // For now I'm just going off of 4 files.
    // There's a bitmore data at the end but I'm not sure what it's for
    while (file_index < 4) {
        uint32_t name_len;
        memcpy(&name_len, pos, sizeof(name_len));
        pos += sizeof(name_len);

        char* name = new char[name_len + 1u];
        memcpy(name, pos, name_len);
        name[name_len] = '\0';
        pos += name_len;
        printf("%s\n", name);

        int32_t file_len;
        memcpy(&file_len, pos, sizeof(file_len));
        pos += sizeof(file_len);

        char* file = new char[file_len];
        memcpy(file, pos, file_len);
        pos += file_len;

        memcpy(&file_index, pos, sizeof(file_index));
        pos += sizeof(file_index);

        std::string out_path;
        JoinPath(out_dir, std::string(name), out_path);
        FILE* out_file = fopen(out_path.c_str(), "wb");
        fwrite(file, 1, file_len, out_file);
        fclose(out_file);

        delete[] name;
        delete[] file;
    }

    delete[] dest;
    // ----------------------------------

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
