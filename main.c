#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct {
    char ext[32];
    int count;
} ExtStat;

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

static void ask_input(const char* prompt, char* buf, size_t size) {
    printf("%s: ", prompt);
    if (fgets(buf, (int)size, stdin) == NULL) {
        snprintf(buf, size, "UNKNOWN");
        return;
    }
    trim_newline(buf);
    if (strlen(buf) == 0) {
        snprintf(buf, size, "UNKNOWN");
    }
}

static unsigned long random_id() {
    srand((unsigned)time(NULL));
    return (unsigned long)(1000000 + rand() % 9000000);
}

static const char* detect_type(const char* name) {
    const char* dot = strrchr(name, '.');
    if (!dot) return "OTHER";
    if (strcmp(dot, ".c") == 0) return "C";
    if (strcmp(dot, ".h") == 0) return "C HEADER";
    if (strcmp(dot, ".cpp") == 0) return "C++";
    if (strcmp(dot, ".py") == 0) return "PYTHON";
    if (strcmp(dot, ".js") == 0) return "JAVASCRIPT";
    if (strcmp(dot, ".java") == 0) return "JAVA";
    if (strcmp(dot, ".cmake") == 0 || strcmp(dot, "CMakeLists.txt") == 0) return "CMAKE";
    if (strcmp(dot, ".exe") == 0 || strcmp(dot, ".dll") == 0 || strcmp(dot, ".bin") == 0) return "BINARY";
    return "OTHER";
}

static int is_binary_ext(const char* name) {
    const char* dot = strrchr(name, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".exe") == 0 ||
            strcmp(dot, ".dll") == 0 ||
            strcmp(dot, ".bin") == 0);
}

static void write_file_content(FILE* out, const char* path, const char* name) {
    if (is_binary_ext(name)) {
        fprintf(out, "%s [\n] (DETECTED ERROR COUNT: 0)\n", name);
        return;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(out, "%s [CANNOT OPEN FILE]\n", name);
        return;
    }

    fprintf(out, "%s [\n", name);
    char line[2048];
    int lineCount = 0;
    while (fgets(line, sizeof(line), f)) {
        lineCount++;
        if (lineCount > 1700) {
            fprintf(out, "...\n");
            break;
        }
        fputs(line, out);
    }
    fprintf(out, "] (DETECTED ERROR COUNT: 0)\n");
    fclose(f);
}

static void scan_dir(const char* base, FILE* out, ExtStat* stats, int* statCount, int* fileCount) {
    DIR* dir = opendir(base);
    if (!dir) return;

    struct dirent* entry;
    char path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", base, entry->d_name);
        struct stat st;
        if (stat(path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir(path, out, stats, statCount, fileCount);
        } else {
            (*fileCount)++;
            const char* t = detect_type(entry->d_name);

            int found = 0;
            for (int i = 0; i < *statCount; i++) {
                if (strcmp(stats[i].ext, t) == 0) {
                    stats[i].count++;
                    found = 1;
                    break;
                }
            }
            if (!found && *statCount < 100) {
                strcpy(stats[*statCount].ext, t);
                stats[*statCount].count = 1;
                (*statCount)++;
            }

            write_file_content(out, path, entry->d_name);
        }
    }

    closedir(dir);
}

int main(int argc, char** argv) {
    const char* path = ".";
    if (argc >= 4 && strcmp(argv[1], "bundle") == 0 && strcmp(argv[2], "--path") == 0) {
        path = argv[3];
    }

    char pkg[256], desc[512], lic[256], auth[256];
    ask_input("Package Name", pkg, sizeof(pkg));
    ask_input("Description", desc, sizeof(desc));
    ask_input("License File", lic, sizeof(lic));
    ask_input("Author", auth, sizeof(auth));

    FILE* out = fopen(".bundler", "w");
    if (!out) {
        printf("Could not create .bundler\n");
        return 1;
    }

    fprintf(out, "<! START !>\n\n");

    ExtStat stats[100] = {0};
    int statCount = 0, fileCount = 0;
    FILE* tmp = tmpfile();
    scan_dir(path, tmp, stats, &statCount, &fileCount);

    fprintf(out, "<! STAT !>\n");
    int total = 0;
    for (int i = 0; i < statCount; i++) total += stats[i].count;
    for (int i = 0; i < statCount; i++) {
        double perc = (stats[i].count * 100.0) / total;
        fprintf(out, "%%%0.1f %s", perc, stats[i].ext);
        if (i != statCount-1) fprintf(out, " | ");
    }
    fprintf(out, "\n\n");

    fprintf(out, "<! NAMES !>\n");
    for (int i = 0; i < statCount; i++) {
        fprintf(out, "%s | file\n", stats[i].ext);
    }
    fprintf(out, "\n");

    fprintf(out, "<! DIR-NAME !>\n\"%s\"\n\n", path);
    fprintf(out, "<! BUNDLE-ID !>\n\"%lu\"\n\n", random_id());
    fprintf(out, "<! PROJECT-TYPE !>\n");
    for (int i = 0; i < statCount; i++) {
        fprintf(out, "[%s]%s", stats[i].ext, (i != statCount-1 ? ", " : ""));
    }
    fprintf(out, "\n\n");

    fprintf(out, "<! BUNDLE-NAME !>\n\"%s\"\n\n", pkg);
    fprintf(out, "<! BUNDLE-DESCRIPTION !>\n\"%s\"\n\n", desc);
    fprintf(out, "<! END !>\n\n");

    fprintf(out, "VIEW LICENSE IN <%s>, CANNOT BE COPIED.\n", lic);
    fprintf(out, "THIS PROJECT IS MADE BY AUTHOR [%s].\n\n", auth);
    fprintf(out, "PROJECT SUPPORTED BY BUNDLEME\n");
    fprintf(out, "PROJECT USES BUNDLEME FOR BUNDLING PROCESS\n\n");

    fprintf(out, "<! FILE-CONTENTS !>\n");
    rewind(tmp);
    int ch;
    while ((ch = fgetc(tmp)) != EOF) fputc(ch, out);
    fclose(tmp);

    fclose(out);
    printf(".bundler file created with %d files.\n", fileCount);
    return 0;
}
