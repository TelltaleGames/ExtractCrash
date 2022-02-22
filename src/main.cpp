#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "zlib.h"
#define CHUNK 16384

int main(int argc, const char** argv) {
    if(argc != 3) {
        printf("Usage: extractcrash <input> <output>\n");
        return 0;
    }
    FILE* source = fopen(argv[1], "rb");
    FILE* dest = fopen(argv[2], "wb");

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    unsigned have;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0; // size of input
    strm.next_in = Z_NULL; // input char array

    // the actual DE-compression work.
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        printf("Decompress failed! %d\n", ret);
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);

        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }

        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }

            have = CHUNK - strm.avail_out;

            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;

    return 0;
}
