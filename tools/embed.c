#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "rb");
    if (!in) { perror("fopen input"); return 1; }
    FILE *out = fopen(argv[2], "w");
    if (!out) { perror("fopen output"); fclose(in); return 1; }

    fseek(in, 0, SEEK_END);
    long sz = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char *buf = malloc(sz);
    fread(buf, 1, sz, in);
    fclose(in);

    // Extract basename for variable name
    const char *inname = argv[1];
    const char *base = strrchr(inname, '/');
    base = base ? base + 1 : inname;
    // strip extension
    char varname[256];
    strncpy(varname, base, 255);
    char *dot = strrchr(varname, '.');
    if (dot) *dot = 0;

    fprintf(out, "// Auto-generated from %s\n", argv[1]);
    fprintf(out, "#ifndef %s_H_\n", varname);
    fprintf(out, "#define %s_H_\n\n", varname);
    fprintf(out, "static const unsigned char %s_data[] = {\n", varname);

    for (long i = 0; i < sz; i++) {
        if (i % 16 == 0) fprintf(out, "    ");
        fprintf(out, "0x%02x", buf[i]);
        if (i < sz - 1) fprintf(out, ", ");
        if (i % 16 == 15) fprintf(out, "\n");
    }
    fprintf(out, "\n};\n");
    fprintf(out, "static const long %s_size = %ld;\n\n", varname, sz);
    fprintf(out, "#endif\n");

    fclose(out);
    free(buf);
    return 0;
}
