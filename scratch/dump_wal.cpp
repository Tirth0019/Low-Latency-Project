#include <cstdio>
#include <cstdint>

int main() {
    FILE* f = fopen("engine.wal", "rb");
    if (!f) return 1;
    uint8_t buf[1024];
    fread(buf, 1, 1024, f);
    for (int i = 0; i < 512; ++i) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    fclose(f);
    return 0;
}
