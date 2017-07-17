/****************************************************************************
    MP4 output module

    Copyright (C) 2017 Krzysztof Nikiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#ifndef WORDS_BIGENDIAN
//#include <byteswap.h>
#endif
#include <string.h>
#include <sys/time.h>

enum ATOM_TYPE
{
    ATOM_STOP = 0 /* end of atoms */ ,
    ATOM_NAME /* plain atom */ ,
    ATOM_DESCENT,               /* starts group of children */
    ATOM_ASCENT,                /* ends group */
    ATOM_DATA,
};
typedef struct
{
    uint16_t opcode;
    void *data;
} creator_t;

#include "mp4atom.h"

mp4config_t mp4config = { 0 };

static FILE *g_fout = NULL;

static inline uint32_t bswap32(uint32_t u32)
{
#ifndef WORDS_BIGENDIAN
    //return __bswap_32(u32);
    return __builtin_bswap32(u32);
#endif
}

static inline uint16_t bswap16(uint16_t u16)
{
#ifndef WORDS_BIGENDIAN
    //return __bswap_16(u16);
    return __builtin_bswap16(u16);
#endif
}

static int dataout(const void *data, int size)
{
    if (fwrite(data, 1, size, g_fout) != size)
    {
        perror("mp4out");
        return -1;
    }
    return size;
}

static int stringout(const char *txt)
{
    return dataout(txt, strlen(txt));
}

static int u32out(uint32_t u32)
{
    u32 = bswap32(u32);
    return dataout(&u32, 4);
}

static int u16out(uint16_t u16)
{
    u16 = bswap16(u16);
    return dataout(&u16, 2);
}

static int u8out(uint8_t u8)
{
    if (fwrite(&u8, 1, 1, g_fout) != 1)
    {
        perror("mp4 out");
        return 0;
    }
    return 1;
}

static int ftypout(void)
{
    int size = 0;

    size += stringout("isom");
    size += u32out(0);
    //size += stringout("isommp42");
    size += stringout("mp41mp42");

    return size;
}

enum
{ SECSINDAY = 24 * 60 * 60 };
uint32_t mp4time(void)
{
    int y;
    struct timeval t;

    gettimeofday(&t, NULL);

    // add some time from the start of 1904 to the start of 1970
    for (y = 1904; y < 1970; y++)
    {
        t.tv_sec += 365 * SECSINDAY;
        if (!(y & 3))
            t.tv_sec += SECSINDAY;
    }

    return t.tv_sec;
}

static int mvhdout(void)
{
    int size = 0;
    int cnt;

    // version
    size += u8out(0);
    // flags
    size += u8out(0);
    size += u16out(0);
    // Creation time
    size += u32out(mp4time());
    // Modification time
    size += u32out(mp4time());
    // Time scale (samplerate)
    size += u32out(mp4config.samplerate);
    // Duration
    size += u32out(mp4config.samples);
    // rate
    size += u32out(0x00010000);
    // volume
    size += u16out(0x0100);
    // reserved
    size += u16out(0);
    size += u32out(0);
    size += u32out(0);
    // matrix
    size += u32out(0x00010000);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0x00010000);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0x40000000);

    for (cnt = 0; cnt < 6; cnt++)
        size += u32out(0);
    // Next track ID
    size += u32out(2);

    return size;
};

static int tkhdout(void)
{
    int size = 0;

    // version
    size += u8out(0);
    // flags
    // bits 8-23
    size += u16out(0);
    // bits 0-7
    size += u8out(1 /*track enabled */ );
    // Creation time
    size += u32out(mp4time());
    // Modification time
    size += u32out(mp4time());
    // Track ID
    size += u32out(1);
    // Reserved
    size += u32out(0);
    // Duration
    size += u32out(mp4config.samples);
    // Reserved
    size += u32out(0);
    size += u32out(0);
    // Layer
    size += u16out(0);
    // Alternate group
    size += u16out(0);
    // Volume
    size += u16out(0x0100);
    // Reserved
    size += u16out(0);
    // matrix
    size += u32out(0x00010000);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0x00010000);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0);
    size += u32out(0x40000000);

    // Track width
    size += u32out(0);
    // Track height
    size += u32out(0);

    return size;
};

static int mdhdout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Creation time
    size += u32out(mp4time());
    // Modification time
    size += u32out(mp4time());
    // Time scale
    size += u32out(mp4config.samplerate);
    // Duration
    size += u32out(mp4config.samples);
    // Language
    size += u16out(0 /*0=English */ );
    // pre_defined
    size += u16out(0);

    return size;
};


static int hdlrout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // pre_defined
    size += u32out(0);
    // Component subtype
    size += stringout("soun");
    // reserved
    size += u32out(0);
    size += u32out(0);
    size += u32out(0);
    // name
    // null terminate
    size += u8out(0);

    return size;
};

static int smhdout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Balance
    size += u16out(0 /*center */ );
    // Reserved
    size += u16out(0);

    return size;
};

static int drefout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Number of entries
    size += u32out(1 /*url reference */ );

    return size;
};

static int urlout(void)
{
    int size = 0;

    size += u32out(1);

    return size;
};

static int stsdout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Number of entries(one 'mp4a')
    size += u32out(1);

    return size;
};

static int mp4aout(void)
{
    int size = 0;
    // Reserved (6 bytes)
    size += u32out(0);
    size += u16out(0);
    // Data reference index
    size += u16out(1);
    // Version
    size += u16out(0);
    // Revision level
    size += u16out(0);
    // Vendor
    size += u32out(0);
    // Number of channels
    size += u16out(mp4config.channels);
    // Sample size (bits)
    size += u16out(mp4config.bits);
    // Compression ID
    size += u16out(0);
    // Packet size
    size += u16out(0);
    // Sample rate (16.16)
    // rate integer part
    size += u16out(mp4config.samplerate);
    // rate reminder part
    size += u16out(0);

    return size;
}

static int esdsout(void)
{
    int size = 0;
    // descriptor definitions:
    // systems/mp4_file_format/libisomediafile/src/MP4Descriptors.h
    // systems/mp4_file_format/libisomediafile/src/MP4Descriptors.c
    //
    // descriptor tree:
    // MP4ES_Descriptor
    //   MP4DecoderConfigDescriptor
    //      MP4DecSpecificInfoDescriptor
    //   MP4SLConfigDescriptor
    struct
    {
        int es;
        int dc;                 // DecoderConfig
        int dsi;                // DecSpecificInfo
        int sl;                 // SLConfig
    } dsize;

    enum
    { TAG_ES = 3, TAG_DC = 4, TAG_DSI = 5, TAG_SLC = 6 };

    // calc sizes
#define DESCSIZE(x) (x + 2/*.tag+.size*/)
    dsize.sl = 1;
    dsize.dsi = mp4config.asc.size;
    dsize.dc = 13 + DESCSIZE(dsize.dsi);
    dsize.es = 3 + DESCSIZE(dsize.dc) + DESCSIZE(dsize.sl);

    // output esds atom data
    // version/flags ?
    size += u32out(0);
    // mp4es
    size += u8out(TAG_ES);
    size += u8out(dsize.es);
    // ESID
    size += u16out(0);
    // flags(url(bit 6); ocr(5); streamPriority (0-4)):
    size += u8out(0);

    size += u8out(TAG_DC);
    size += u8out(dsize.dc);
    size += u8out(0x40 /*MPEG-4 audio */ );
    // DC flags: upstream(bit 1); streamType(2-7)
    // this field is typically 0x15 but I think it
    // could store "Object Type ID", why not
    size += u8out(2 /*AAC LC*/ << 2);
    // buffer size (24 bits)
    size += u16out(mp4config.buffersize >> 8);
    size += u8out(mp4config.buffersize && 0xff);
    // bitrate
    size += u32out(mp4config.bitratemax);
    size += u32out(mp4config.bitrateavg);

    size += u8out(TAG_DSI);
    size += u8out(dsize.dsi);
    // AudioSpecificConfig
    size += dataout(mp4config.asc.data, mp4config.asc.size);

    size += u8out(TAG_SLC);
    size += u8out(dsize.sl);
    // "predefined" (no idea)
    size += u8out(2);

    return size;
}

static int sttsout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Number of entries
    size += u32out(1);
    // only one entry
    // Sample count (number of frames)
    size += u32out(mp4config.frame.ents);
    // Sample duration (samples per frame)
    size += u32out(1024);

    return size;
}

static int stszout(void)
{
    int size = 0;
    int cnt;

    // version/flags
    size += u32out(0);
    // Sample size
    size += u32out(0 /*i.e. variable size */ );
    // Number of entries
    if (!mp4config.frame.ents)
        return size;
    if (!mp4config.frame.data)
        return size;

    size += u32out(mp4config.frame.ents);
    for (cnt = 0; cnt < mp4config.frame.ents; cnt++)
        size += u32out(mp4config.frame.data[cnt]);

    return size;
}

static int stscout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Number of entries
    size += u32out(2);
    // 1st entry
    // first chunk
    size += u32out(1);
    // frames per chunks (256 for simplicity)
    size += u32out(0x100);
    // sample id
    size += u32out(1);
    // 2nd entry
    // last chunk
    size += u32out((mp4config.frame.ents >> 8) + 1);
    // frames in last chunk
    size += u32out(mp4config.frame.ents & 0xff);

    // sample id
    size += u32out(1);

    return size;
}

static int stcoout(void)
{
    int size = 0;
    int chunks = (mp4config.frame.ents >> 8) + 1;
    int cnt;
    uint32_t ofs = mp4config.mdatofs;

    // version/flags
    size += u32out(0);
    // Number of entries
    size += u32out(chunks);
    // Chunk offset table
    if (!mp4config.frame.ents)
        return size;
    if (!mp4config.frame.data)
        return size;
    for (cnt = 0; cnt < mp4config.frame.ents; cnt++)
    {
        if (!(cnt & 0xff))
            size += u32out(ofs);
        ofs += mp4config.frame.data[cnt];
    }

    return size;
}

static int tagtxt(char *tagname, const char *tagtxt)
{
    int txtsize = strlen(tagtxt);
    int size = 0;
    int datasize = txtsize + 16;

    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(1);
    size += u32out(0);
    size += dataout(tagtxt, txtsize);

    return size;
}

static int tagu32(char *tagname, uint32_t n)
{
    int numsize = 4;
    int size = 0;
    int datasize = numsize + 16;

    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(0);
    size += u32out(0);
    size += u32out(n);

    return size;
}

static int metaout(void)
{
    int size = 0;

    // version/flags
    size += u32out(0);
    // Predefined
    size += u32out(0);
    // Handler type
    size += stringout("mdir");
    size += stringout("appl");
    // Reserved
    size += u32out(0);
    size += u32out(0);
    // null terminator
    size += u8out(0);

    return size;
};

static int ilstout(void)
{
    int size = 0;

    size += tagtxt("\xa9" "too", mp4config.tag.encoder);
    if (mp4config.tag.artist)
        size += tagtxt("\xa9" "ART", mp4config.tag.artist);
    if (mp4config.tag.composer)
        size += tagtxt("\xa9" "wrt", mp4config.tag.composer);
    if (mp4config.tag.title)
        size += tagtxt("\xa9" "nam", mp4config.tag.title);
    if (mp4config.tag.genre)
        size += tagtxt("gnre", mp4config.tag.genre);
    if (mp4config.tag.album)
        size += tagtxt("\xa9" "alb", mp4config.tag.album);
    if (mp4config.tag.compilation)
        size += tagu32("cpil", mp4config.tag.compilation);
    if (mp4config.tag.trackno)
        size += tagu32("trkn", mp4config.tag.trackno);
    if (mp4config.tag.discno)
        size += tagu32("disk", mp4config.tag.discno);
    if (mp4config.tag.year)
        size += tagtxt("\xa9" "day", mp4config.tag.year);
#if 0
    if (mp4config.tag.cover)
        size += tagtxt("\xa9" "covr", mp4config.tag.cover);
#endif
    if (mp4config.tag.comment)
        size += tagtxt("\xa9" "cmt", mp4config.tag.comment);

    return size;
};

static creator_t g_head[] = {
    {ATOM_NAME, "ftyp"},
    {ATOM_DATA, ftypout},
    {ATOM_NAME, "free"},
    {ATOM_NAME, "mdat"},
    {0}
};

static creator_t g_tail[] = {
    {ATOM_NAME, "moov"},
    {ATOM_DESCENT},
    {ATOM_NAME, "mvhd"},
    {ATOM_DATA, mvhdout},
    {ATOM_NAME, "trak"},
    {ATOM_DESCENT},
    {ATOM_NAME, "tkhd"},
    {ATOM_DATA, tkhdout},
    {ATOM_NAME, "mdia"},
    {ATOM_DESCENT},
    {ATOM_NAME, "mdhd"},
    {ATOM_DATA, mdhdout},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, hdlrout},
    {ATOM_NAME, "minf"},
    {ATOM_DESCENT},
    {ATOM_NAME, "smhd"},
    {ATOM_DATA, smhdout},
    {ATOM_NAME, "dinf"},
    {ATOM_DESCENT},
    {ATOM_NAME, "dref"},
    {ATOM_DATA, drefout},
    {ATOM_DESCENT},
    {ATOM_NAME, "url "},
    {ATOM_DATA, urlout},
    {ATOM_ASCENT},
    {ATOM_ASCENT},
    {ATOM_NAME, "stbl"},
    {ATOM_DESCENT},
    {ATOM_NAME, "stsd"},
    {ATOM_DATA, stsdout},
    {ATOM_DESCENT},
    {ATOM_NAME, "mp4a"},
    {ATOM_DATA, mp4aout},
    {ATOM_DESCENT},
    {ATOM_NAME, "esds"},
    {ATOM_DATA, esdsout},
    {ATOM_ASCENT},
    {ATOM_ASCENT},
    {ATOM_NAME, "stts"},
    {ATOM_DATA, sttsout},
    {ATOM_NAME, "stsz"},
    {ATOM_DATA, stszout},
    {ATOM_NAME, "stsc"},
    {ATOM_DATA, stscout},
    {ATOM_NAME, "stco"},
    {ATOM_DATA, stcoout},
    {ATOM_ASCENT},
    {ATOM_ASCENT},
    {ATOM_ASCENT},
    {ATOM_ASCENT},
    {ATOM_NAME, "udta"},
    {ATOM_DESCENT},
    {ATOM_NAME, "meta"},
    {ATOM_DESCENT},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, metaout},
    {ATOM_NAME, "ilst"},
    {ATOM_DATA, ilstout},
    {0}
};

static creator_t *g_atom = 0;
static int create(void)
{
    long apos = ftell(g_fout);;
    int size;
    static int level = 0;

    if (g_atom->opcode != ATOM_NAME)
    {
        static int fail = 0;
        if (++fail > 10)
            exit(1);
        printf("internal error: create: not a 'name' opcode\n");
        return 0;
    }

    size = u32out(8);
    size += dataout(g_atom->data, 4);

    g_atom++;
    if (g_atom->opcode == ATOM_DATA)
    {
        size += ((int (*)(void)) g_atom->data) ();
        g_atom++;
    }
    if (g_atom->opcode == ATOM_DESCENT)
    {
        level++;
        g_atom++;
        while (g_atom->opcode != ATOM_STOP)
        {
            if (g_atom->opcode == ATOM_ASCENT)
            {
                g_atom++;
                break;
            }
            size += create();
        }
        level--;
    }

    fseek(g_fout, apos, SEEK_SET);
    u32out(size);
    fseek(g_fout, apos + size, SEEK_SET);

    return size;
}

enum {BUFSTEP = 0x10000};
int mp4atom_frame(uint8_t * buf, int size, int samples)
{
    if (mp4config.framesamples <= samples)
    {
        int bitrate = 8.0 * size * mp4config.samplerate / samples;

        if (mp4config.bitratemax < bitrate)
            mp4config.bitratemax = bitrate;

        mp4config.framesamples = samples;
    }
    if (mp4config.buffersize < size)
        mp4config.buffersize = size;
    mp4config.samples += samples;
    mp4config.mdatsize += dataout(buf, size);

    if (((mp4config.frame.ents + 1)* sizeof(*(mp4config.frame.data)))
        < mp4config.frame.bufsize)
    {
        mp4config.frame.bufsize += BUFSTEP;
        mp4config.frame.data = realloc(mp4config.frame.data,
                                       mp4config.frame.bufsize);
    }
    mp4config.frame.data[mp4config.frame.ents++] = size;

    return 0;
}

int mp4atom_close(void)
{
    if (g_fout)
    {
        fseek(g_fout, mp4config.mdatofs - 8, SEEK_SET);
        u32out(mp4config.mdatsize + 8);
        fclose(g_fout);
        g_fout = 0;
    }
    if (mp4config.frame.data)
    {
        free(mp4config.frame.data);
        mp4config.frame.data = 0;
    }
    return 0;
}

int mp4atom_open(char *name)
{
    mp4atom_close();

    g_fout = fopen(name, "wb");
    if (!g_fout)
        return 1;

    mp4config.mdatsize = 0;
    mp4config.frame.bufsize = BUFSTEP;
    mp4config.frame.data = malloc(mp4config.frame.bufsize);

    return 0;
}


int mp4atom_head(void)
{
    g_atom = g_head;
    while (g_atom->opcode != ATOM_STOP)
        create();
    mp4config.mdatofs = ftell(g_fout);

    return 0;
}

int mp4atom_tail(void)
{
    mp4config.bitrateavg = 8.0 * mp4config.mdatsize
        * mp4config.samplerate / mp4config.samples;

    g_atom = g_tail;
    while (g_atom->opcode != ATOM_STOP)
        create();

    return 0;
}