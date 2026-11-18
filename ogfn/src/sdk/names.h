#pragma once
#include <cstdint>
#include <cstring>
#include <atomic>
#include "../mem/process.h"
#include "../core/log.h"
#include "offsets.h"

namespace sdk {
    namespace fname {
        inline uint8_t keystream[96];
        inline int     keylen = 0;

        inline uint32_t obj_name_index(uint64_t obj) {
            return process::rpm<uint32_t>(obj + guobject_array_offsets::UObject_Name);
        }

        inline uint64_t entry_addr(uint32_t comp_index) {
            uint32_t block = comp_index >> 16;
            uint32_t offset = comp_index & 0xFFFF;
            uint64_t blocks = process::base + globals::GNames + 0x10;
            uint64_t bptr = process::rpm<uint64_t>(blocks + (uint64_t)block * 8);
            if (bptr < 0x10000) return 0;
            return bptr + 2ull * offset;
        }

        inline int entry_len(uint64_t entry, bool& wide) {
            uint16_t h = process::rpm<uint16_t>(entry);
            wide = (h & 1) != 0;
            return h >> 6;
        }

        inline bool clean(const char* s) {
            int n = 0;
            for (; *s; ++s, ++n) { unsigned char c = (unsigned char)*s; if (c < 0x20 || c > 0x7E) return false; }
            return n >= 1;
        }

        inline void harvest(uint64_t obj, const char* expected) {
            if (!obj) return;
            uint64_t entry = entry_addr(obj_name_index(obj));
            if (!entry) return;
            bool wide = false;
            int len = entry_len(entry, wide);
            int exp = (int)strlen(expected);
            if (wide || len != exp || len <= 0 || len > 96) return;
            uint8_t cipher[96];
            if (!process::read_raw(entry + 2, cipher, len)) return;
            for (int i = 0; i < len; i++) {
                keystream[i] = cipher[i] ^ (uint8_t)expected[i];
                if (i + 1 > keylen) keylen = i + 1;
            }
        }

        inline bool name_of(uint64_t obj, char* out, int out_size) {
            if (!obj || keylen <= 0) return false;
            uint64_t entry = entry_addr(obj_name_index(obj));
            if (!entry) return false;
            bool wide = false;
            int len = entry_len(entry, wide);
            if (wide || len <= 0) return false;
            int m = len < keylen ? len : keylen;
            if (m > out_size - 1) m = out_size - 1;
            uint8_t cipher[256];
            if (!process::read_raw(entry + 2, cipher, m)) return false;
            for (int i = 0; i < m; i++) out[i] = (char)(cipher[i] ^ keystream[i]);
            out[m] = 0;
            return clean(out);
        }

        inline std::atomic<bool> valid{ false };

        inline void build_keystream(uint64_t pawn_class) {
            if (valid.load()) return;
            keylen = 0;
            uint64_t c0 = pawn_class;
            uint64_t c1 = c0 ? process::rpm<uint64_t>(c0 + guobject_array_offsets::UStruct_SuperStruct) : 0;
            uint64_t c2 = c1 ? process::rpm<uint64_t>(c1 + guobject_array_offsets::UStruct_SuperStruct) : 0;
            harvest(c0, "PlayerPawn_Athena_C");
            harvest(c1, "PlayerPawn_Athena_Generic_C");
            harvest(c2, "PlayerPawn_Athena_Generic_Parent_C");

            char nm[128];
            if (!name_of(pawn_class, nm, sizeof(nm)) || strcmp(nm, "PlayerPawn_Athena_C") != 0)
                logx::print("[names] keystream harvest failed (len=%d)\n", keylen);
            else
                valid.store(true);
        }

        inline bool ready() { return valid.load(); }
    }

}
