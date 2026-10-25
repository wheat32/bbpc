/* Exercise actual guest file operations on a linked overlay and writable saves. */
#include "../src/runtime_file.c"
#include <assert.h>
static int32_t guest_errno;
int32_t* runtime_errno(void)
{
    return &guest_errno;
}
int32_t runtime_guest_errno(int e)
{
    return e;
}
/* Reads into guest memory tell the GPU side (runtime_memory.c); nothing to tell here. */
void runtime_memory_note_write(uintptr_t address, uint64_t size)
{
    (void)address;
    (void)size;
}
uintptr_t runtime_lookup(const RuntimeExport* table, size_t n, const char* name)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (strcmp(table[i].name, name) == 0)
        {
            return (uintptr_t)table[i].function;
        }
    }
    return 0;
}
/* A PlaceObject2 row as the title layouts have them (long header, translation-only matrix). */
static size_t put_row(uint8_t* out, unsigned depth, unsigned character, int tx, int ty, const char* name)
{
    enum
    {
        N = 15, /* translation bits */
        MATRIX_BYTES = (2 + 5 + 2 * N + 7) / 8,
    };
    uint8_t body[64] = {TITLE_ROW_FLAGS, depth & 0xff, depth >> 8, character & 0xff, character >> 8};
    size_t len = 5;
    unsigned long long bits = (unsigned long long)N << (2 * N);
    bits |= ((unsigned long long)tx & ((1u << N) - 1)) << N;
    bits |= (unsigned long long)ty & ((1u << N) - 1);
    bits <<= MATRIX_BYTES * 8 - (2 + 5 + 2 * N);
    for (int i = MATRIX_BYTES - 1; i >= 0; --i)
    {
        body[len++] = (uint8_t)(bits >> (8 * i));
    }
    memcpy(body + len, name, strlen(name) + 1);
    len += strlen(name) + 1;
    out[0] = (26 << 6 | 0x3f) & 0xff;
    out[1] = (26 << 6 | 0x3f) >> 8;
    for (int i = 0; i < 4; ++i)
    {
        out[2 + i] = (uint8_t)(len >> (8 * i));
    }
    memcpy(out + 6, body, len);
    return 6 + len;
}
/* A title layout: a sprite with Item_4_0 .. Item_0_0 (as menu/title.gfx orders them), `step` twips
 * apart, row 2 of clip `odd_character`, row 3 moved by `bump`; returns its size. */
static size_t make_title_layout(uint8_t* out, int step, unsigned odd_character, int bump, bool with_sixth)
{
    enum
    {
        CHARACTER = 31,
        TX = -10283,
        FIRST_TY = -948,
    };
    size_t at = 8;
    out[at++] = 0; /* RECT: 0-bit fields */
    out[at++] = 0;
    out[at++] = 30; /* frame rate */
    out[at++] = 1;
    out[at++] = 0; /* frame count */
    const size_t sprite = at;
    at += 6;
    out[at++] = 32; /* sprite id */
    out[at++] = 0;
    out[at++] = 1; /* frames */
    out[at++] = 0;
    for (int row = 4; row >= 0; --row)
    {
        char name[16];
        snprintf(name, sizeof(name), "Item_%d_0", row);
        at += put_row(out + at, 1 + 5 * (4 - row), row == 2 ? odd_character : CHARACTER, TX,
                      FIRST_TY + row * step + (row == 3 ? bump : 0), name);
    }
    if (with_sixth)
    {
        at += put_row(out + at, 26, CHARACTER, TX, FIRST_TY + 5 * step, "Item_5_0");
    }
    out[at++] = 1 << 6; /* ShowFrame */
    out[at++] = 0;
    out[at++] = 0; /* End */
    out[at++] = 0;
    const uint32_t sprite_len = (uint32_t)(at - sprite - 6);
    out[sprite] = (39 << 6 | 0x3f) & 0xff;
    out[sprite + 1] = (39 << 6 | 0x3f) >> 8;
    for (int i = 0; i < 4; ++i)
    {
        out[sprite + 2 + i] = (uint8_t)(sprite_len >> (8 * i));
    }
    out[at++] = 1 << 6; /* ShowFrame */
    out[at++] = 0;
    out[at++] = 0; /* End */
    out[at++] = 0;
    memcpy(out, "GFX", 3);
    out[3] = 11;
    for (int i = 0; i < 4; ++i)
    {
        out[4 + i] = (uint8_t)(at >> (8 * i));
    }
    return at;
}
/* The title layouts get a sixth row (Quit Game): one row below, the same clip, a new depth. */
static void test_title_layout(void)
{
    enum
    {
        STEP = 797,
        CHARACTER = 31,
    };
    uint8_t buf[1024] = {};
    const size_t size = make_title_layout(buf, STEP, CHARACTER, 0, false);
    uint8_t* fixed = nullptr;
    size_t fixed_size = 0;
    assert(fix_title_layout(buf, size, &fixed, &fixed_size));
    assert(fixed_size > size);
    assert((size_t)(fixed[4] | fixed[5] << 8 | fixed[6] << 16 | (uint32_t)fixed[7] << 24) == fixed_size);
    /* Walk the sprite again: Item_5_0 is there, one step below Item_4_0, above the rows in depth. */
    unsigned code, c;
    size_t body, len, next, inner, ilen, inext;
    bool sixth = false;
    assert(swf_tag(fixed, 13, fixed_size, &code, &body, &len, &next) && code == 39);
    for (size_t t = body + 4; swf_tag(fixed, t, next, &c, &inner, &ilen, &inext) && c; t = inext)
    {
        const char* name = c == 26 ? swf_place_name(fixed + inner, c, ilen) : nullptr;
        if (name != nullptr && strcmp(name, "Item_5_0") == 0)
        {
            TitleRow row;
            assert(title_row(fixed, t, inner, ilen, &row));
            assert(row.depth == 26 && row.character == CHARACTER && row.tx == -10283 && row.ty == -948 + 5 * STEP);
            sixth = true;
        }
    }
    assert(sixth);
    /* Its own result has the sixth row already: left as it is. */
    uint8_t* again = nullptr;
    size_t again_size = 0;
    assert(fix_title_layout(fixed, fixed_size, &again, &again_size) == false);
    free(fixed);
    /* Anything unexpected: no change (another clip in a row, uneven rows, a sixth row already). */
    uint8_t other[1024] = {};
    assert(fix_title_layout(other, make_title_layout(other, STEP, CHARACTER + 1, 0, false), &again, &again_size) ==
           false);
    assert(fix_title_layout(other, make_title_layout(other, STEP, CHARACTER, 1, false), &again, &again_size) == false);
    assert(fix_title_layout(other, make_title_layout(other, STEP, CHARACTER, 0, true), &again, &again_size) == false);
}
int main(void)
{
    test_title_layout();
    char root[] = "/tmp/bbport-mod-files-XXXXXX";
    assert(mkdtemp(root));
    char game[512], user[512], source[512], link[512];
    snprintf(game, sizeof(game), "%s/game", root);
    snprintf(user, sizeof(user), "%s/user", root);
    snprintf(source, sizeof(source), "%s/mod.dcx", root);
    snprintf(link, sizeof(link), "%s/game/asset.dcx", root);
    assert(mkdir(game, 0755) == 0);
    FILE* f = fopen(source, "w");
    assert(f);
    assert(fputs("modded", f) >= 0);
    assert(fclose(f) == 0);
    assert(symlink(source, link) == 0);
    runtime_file_configure(game, user);
    for (int i = 0; i < 3; ++i)
    {
        const char* path = i == 0 ? "/app0/asset.dcx" : i == 1 ? "/hostapp/asset.dcx" : "asset.dcx";
        int fd = (int)do_open(path, 0, 0);
        assert(fd >= 3);
        char content[8] = {};
        assert(do_read(fd, content, 6) == 6 && strcmp(content, "modded") == 0);
        GuestStat info;
        assert(do_stat(path, &info) == 0 && info.size == 6);
        assert(do_close(fd) == 0);
        assert(do_open(path, 2, 0) == -EROFS);
        assert(do_open(path, 0x400, 0) == -EROFS);
        assert(do_truncate(path, 0) == -EROFS);
        assert(path_op(path, 2, 0) == -EROFS);
        assert(do_rename(path, "/data/moved") == -EROFS);
    }
    int dir = (int)do_open("/app0", 0x20000, 0);
    assert(dir >= 3);
    char entries[1024];
    int64_t count = do_getdents(dir, entries, sizeof(entries), nullptr);
    assert(count > 0);
    int found = 0;
    for (int64_t p = 0; p < count;)
    {
        uint16_t length;
        memcpy(&length, entries + p + 4, 2);
        assert(length);
        if (strcmp(entries + p + 8, "asset.dcx") == 0)
        {
            assert(entries[p + 6] == 8);
            found = 1;
        }
        p += length;
    }
    assert(found && do_close(dir) == 0);
    int save = (int)do_open("/data/test-save", 0x202, 0644);
    assert(save >= 3);
    assert(do_write(save, "save", 4) == 4 && do_close(save) == 0);
    assert(path_op("/data/test-save", 2, 0) == 0);
    assert(unlink(link) == 0 && unlink(source) == 0 && rmdir(game) == 0);
    const char* dirs[] = {"temp0", "download0", "data"};
    for (int i = 0; i < 3; ++i)
    {
        char p[1024];
        snprintf(p, sizeof(p), "%s/%s", user, dirs[i]);
        assert(rmdir(p) == 0);
    }
    assert(rmdir(user) == 0 && rmdir(root) == 0);
    puts("Guest mod files: reads, stat, merged listing, readonly assets and writable saves PASS");
}
