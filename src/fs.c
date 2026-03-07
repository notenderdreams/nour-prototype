#include "fs.h"
#include "log.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

int ensure_directory(const char *path) {
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    perror("mkdir failed");
    return -1;
}

FileList expand_glob(Arena *arena, const char *pattern) {
    FileList result = {NULL, 0};

    if (!arena || !pattern) return result;

    glob_t g;
    int ret = glob(pattern, GLOB_NOSORT, NULL, &g);

    if (ret != 0) {
        if (ret == GLOB_NOMATCH) {
            log_print(LOG_WARN, "No files matched pattern: %s\n", pattern);
        } else {
            log_print(LOG_ERROR, "glob() failed for pattern: %s\n", pattern);
        }
        globfree(&g);
        return result;
    }

    char **files = arena_alloc(arena, sizeof(char *) * g.gl_pathc);
    if (!files) {
        globfree(&g);
        return result;
    }

    for (size_t i = 0; i < g.gl_pathc; i++) {
        size_t len = strlen(g.gl_pathv[i]);
        char *copy = arena_alloc(arena, len + 1);
        if (!copy) {
            globfree(&g);
            return result;
        }
        memcpy(copy, g.gl_pathv[i], len + 1);
        files[i] = copy;
    }

    result.files = files;
    result.count = g.gl_pathc;

    globfree(&g);
    return result;
}

FileList get_dependent_files(Arena *arena, const char *filepath){
    FileList result = {NULL, 0};

    if (!arena || !filepath) return result;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        log_print(LOG_ERROR, "Failed to open file: %s\n", filepath);
        return result;
    }

    // First pass: count #include "..." directives
    size_t count = 0;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "#include \"", 10) == 0) {
            count++;
        }
    }

    if (count == 0) {
        fclose(f);
        return result;
    }

    char **files = arena_alloc(arena, sizeof(char *) * count);
    if (!files) {
        fclose(f);
        return result;
    }

    // Extract the directory from filepath to resolve relative includes
    const char *last_slash = strrchr(filepath, '/');
    size_t dir_len = last_slash ? (size_t)(last_slash - filepath + 1) : 0;

    // Second pass: extract filenames
    rewind(f);
    size_t idx = 0;

    while (fgets(line, sizeof(line), f) && idx < count) {
        if (strncmp(line, "#include \"", 10) == 0) {
            char dep[256];
            // Parse the filename between quotes
            if (sscanf(line, "#include \"%255[^\"]\"", dep) == 1) {
                size_t dep_len = strlen(dep);
                size_t total_len = dir_len + dep_len;
                char *copy = arena_alloc(arena, total_len + 1);
                if (!copy) {
                    fclose(f);
                    result.files = files;
                    result.count = idx;
                    return result;
                }
                // Prepend the directory of the source file
                if (dir_len > 0) {
                    memcpy(copy, filepath, dir_len);
                }
                memcpy(copy + dir_len, dep, dep_len + 1);
                files[idx++] = copy;
            }
        }
    }

    fclose(f);

    result.files = files;
    result.count = idx;
    return result;
}

DepGraph build_dep_graph(Arena *arena, FileList sources) {
    DepGraph graph = {NULL, 0};
    if (!arena || sources.count == 0 || !sources.files) return graph;

    // Collect deps for every source file
    FileList *all_deps = arena_alloc(arena, sizeof(FileList) * sources.count);
    if (!all_deps) return graph;

    size_t total_pairs = 0;
    for (size_t i = 0; i < sources.count; i++) {
        all_deps[i] = get_dependent_files(arena, sources.files[i]);
        total_pairs += all_deps[i].count;
    }

    if (total_pairs == 0) return graph;

    // Upper bound: at most total_pairs unique dep-file nodes
    DepNode *nodes = arena_alloc(arena, sizeof(DepNode) * total_pairs);
    if (!nodes) return graph;

    size_t node_count = 0;

    for (size_t i = 0; i < sources.count; i++) {
        for (size_t j = 0; j < all_deps[i].count; j++) {
            const char *dep = all_deps[i].files[j];

            // Find existing node for this dep
            DepNode *node = NULL;
            for (size_t k = 0; k < node_count; k++) {
                if (strcmp(nodes[k].file, dep) == 0) {
                    node = &nodes[k];
                    break;
                }
            }

            if (!node) {
                // New node — pre-allocate dependents for worst-case (all sources)
                nodes[node_count].file = (char *)dep;
                nodes[node_count].dependents = arena_alloc(arena, sizeof(char *) * sources.count);
                nodes[node_count].count = 0;
                if (!nodes[node_count].dependents) return graph;
                node = &nodes[node_count++];
            }

            node->dependents[node->count++] = sources.files[i];
        }
    }

    graph.nodes = nodes;
    graph.count = node_count;
    return graph;
}

void print_leaf(char *item, const char *padding, int last) {
    printf("%s%s %s\n", padding, last ? "└──" : "├──", item);
}

void print_tree(char **items, size_t count, const char *padding) {
    if (!items || count == 0) return;
    for (size_t i = 0; i < count; i++) {
        int last = (i == count - 1);
        print_leaf(items[i], padding, last);
    }
}

void print_dep_graph(const DepGraph *graph) {
    if (!graph || graph->count == 0) {
        log_print(LOG_INFO, "No dependencies found.\n");
        return;
    }
    log_print(LOG_INFO, "Dependency graph:\n");

    for (size_t i = 0; i < graph->count; ++i) {
        int last_node = (i == graph->count - 1);
        print_leaf(graph->nodes[i].file, "", last_node);
        const char *padding = last_node ? "    " : "│   ";
        print_tree(graph->nodes[i].dependents, graph->nodes[i].count, padding);
    }
}