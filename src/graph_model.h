#ifndef APMICRO_GRAPH_MODEL_H
#define APMICRO_GRAPH_MODEL_H

#include <stdbool.h>

#include "generated/apmicro_content.h"

typedef struct {
    const char *id;
    const char *name;
    const char *type;
    const char *location;
    const char *meaning;
    const char *used_for;
    const char *related_terms;
    const char *related_formulas;
    bool area_capable;
    bool shift_capable;
    int anchor_x;
    int anchor_y;
} GraphElementEntry;

enum {
    GRAPH_FEATURE_LABELS = 1 << 0,
    GRAPH_FEATURE_POINTS = 1 << 1,
    GRAPH_FEATURE_REGIONS = 1 << 2,
    GRAPH_FEATURE_CONCEPTS = 1 << 3,
    GRAPH_FEATURE_SHIFT = 1 << 4,
    GRAPH_FEATURE_INFO = 1 << 5,
};

typedef struct {
    int supported_flags;
    int default_flags;
    int toggle_flags[3];
    const char *toggle_labels[3];
} GraphCapabilities;

typedef struct {
    bool show_labels;
    bool show_points;
    bool show_area;
    bool show_concepts;
    bool show_shift;
    bool show_info;
} GraphRenderOptions;

const GraphElementEntry *graphs_get_element(const GraphEntry *graph, int index);
int graphs_get_element_count(const GraphEntry *graph);
int graphs_find_element_by_id(const GraphEntry *graph, const char *id);
int graphs_find_element_by_term(const GraphEntry *graph, const char *term);
const GraphCapabilities *graphs_get_capabilities(const GraphEntry *graph);

#endif
