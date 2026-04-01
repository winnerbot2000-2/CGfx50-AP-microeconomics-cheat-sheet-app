#include "app.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOME_COUNT 9
#define VOCAB_MODE_COUNT 3
#define UNIT_PAGE_COUNT 2
#define TOPIC_PAGE_COUNT 2
#define VOCAB_PAGE_COUNT 4
#define CATEGORY_COUNT 12
#define GRAPH_PAGE_COUNT 6
#define CONCEPT_PAGE_COUNT 4
#define FORMULA_PAGE_COUNT 2
#define STRUCTURE_PAGE_COUNT 2
#define REVISION_ITEM_COUNT 6
#define RECENT_CAPACITY 16
#define PAGE_BUFFER_SIZE 16384
#define GRAPH_MODE_LABELS GRAPH_FEATURE_LABELS
#define GRAPH_MODE_POINTS GRAPH_FEATURE_POINTS
#define GRAPH_MODE_AREA GRAPH_FEATURE_REGIONS
#define GRAPH_MODE_CONCEPTS GRAPH_FEATURE_CONCEPTS
#define GRAPH_MODE_SHIFT GRAPH_FEATURE_SHIFT
#define GRAPH_MODE_INFO GRAPH_FEATURE_INFO

typedef struct {
    const char *label;
    const char *subtitle;
} MenuItem;

typedef struct {
    ViewType type;
    int index;
    int page;
    int focus_index;
    int mode_flags;
    char title[64];
} RecentEntry;

typedef struct {
    ViewState stack[STACK_DEPTH];
    int depth;
    RecentEntry recent[RECENT_CAPACITY];
    int recent_count;
} AppState;

static AppState g_app = {.depth = 1};
static char g_page_body[PAGE_BUFFER_SIZE];
static char g_result_title[64];
static char g_result_body[4096];

static const MenuItem k_home_items[HOME_COUNT] = {
    {"Units", "Unit overviews, topics, and links"},
    {"Concepts", "Big ideas with mini graph previews"},
    {"Vocabulary", "Unit, A-Z, and category lookup"},
    {"Graphs", "Graph references with subpages"},
    {"Formulas", "Formula cards and helpers"},
    {"Structures", "Market structure comparisons"},
    {"Revision Hub", "Quick review, cram, reference, and formula guides"},
    {"Recent", "Jump back to recent pages"},
    {"About / Help", "Controls, source audit, build info"},
};

static const MenuItem k_revision_items[REVISION_ITEM_COUNT] = {
    {"Quick Review", "Compact cue sheets and fast reminders"},
    {"Exam Cram", "High-yield summaries for last-minute review"},
    {"Reference Sheets", "Rules, shifts, policy effects, and formula sheets"},
    {"Market Structures", "Side-by-side comparison pages"},
    {"Graph Atlas", "Graph list plus graph-id study guide"},
    {"Formula Cards", "Formula sheet and calculator helpers"},
};

static const MenuItem k_vocab_modes[VOCAB_MODE_COUNT] = {
    {"By Unit", "Terms grouped by AP unit"},
    {"A-Z Master List", "Alphabetical lookup"},
    {"By Category", "Graph area, market structure, welfare, and more"},
};

static const char *k_categories[CATEGORY_COUNT] = {
    "core concept",
    "graph label",
    "graph area",
    "graph point",
    "formula variable",
    "elasticity term",
    "cost/revenue term",
    "market structure term",
    "labor market term",
    "government intervention term",
    "externality term",
    "efficiency/welfare term",
};

static const char *k_graph_pages[GRAPH_PAGE_COUNT] = {
    "Graph",
    "Labels",
    "Shifts",
    "Reading Guide",
    "Common Questions",
    "Mistakes",
};

static const char *k_vocab_pages[VOCAB_PAGE_COUNT] = {
    "Summary",
    "Deep Dive",
    "Visual",
    "Links",
};

static const char *k_concept_pages[CONCEPT_PAGE_COUNT] = {
    "Summary",
    "Deep Dive",
    "Visual",
    "Links",
};

static const char *k_unit_pages[UNIT_PAGE_COUNT] = {
    "Overview",
    "Study Links",
};

static const char *k_topic_pages[TOPIC_PAGE_COUNT] = {
    "Overview",
    "Study Links",
};

static const char *k_formula_pages[FORMULA_PAGE_COUNT] = {
    "Formula",
    "Links",
};

static const char *k_structure_pages[STRUCTURE_PAGE_COUNT] = {
    "Comparison",
    "Links",
};

static const char *k_about_text =
    "CG Micro\n\n"
    "AP Microeconomics study app for the Casio fx-CG50.\n\n"
    "Navigation\n"
    "- UP/DOWN scroll inside the current section\n"
    "- LEFT/RIGHT pan sideways across wide text sections\n"
    "- EXE open\n"
    "- EXIT back\n"
    "- F1/F2 move between logical sections on detail pages\n\n"
    "Graph atlas controls\n"
    "- F1/F2 switch graph sections\n"
    "- F3 cycle highlighted element\n"
    "- F4/F5/F6 toggle only the overlays supported by the current graph\n"
    "- EXE open the focused term\n"
    "- On graph info pages, F6 opens the focused formula\n\n"
    "Sections\n"
    "- Units\n"
    "- Concepts / theories with embedded mini graphs\n"
    "- Rich vocabulary with graph/formula metadata and mini graph pages\n"
    "- Graph reference pages with labels, shifts, guide, questions, and mistakes\n"
    "- Formula cards with calculators\n"
    "- Market structure comparisons\n"
    "- Revision Hub for quick review, exam cram, reference sheets, graph guides, and formula cards\n"
    "- Recent pages for faster lookup\n\n"
    "Build\n"
    "Build this project with fxSDK/gint to generate cgmicro.g3a.";

static void set_result_text(const char *title, const char *body);
static void render_simple_menu(ListCursorState *cursor, const char *title, const char *subtitle, const MenuItem *items, int count);
static void append_section(char *buffer, size_t size, const char *title, const char *value);
static void ensure_list_window(ListCursorState *cursor, int count, int visible_rows);
static void reset_text_viewport(TextScrollState *text);
static void reset_paged_viewport(PagedTextState *detail);
static bool change_text_section(TextScrollState *text, int page_count, int delta);
static bool change_paged_section(PagedTextState *detail, int page_count, int delta);
static int graph_default_mode_flags(const GraphEntry *graph);
static int graph_supported_mode_flags(const GraphEntry *graph);
static const char *graph_toggle_label(const GraphEntry *graph, int slot);
static int graph_toggle_flag(const GraphEntry *graph, int slot);
static int sanitize_graph_mode_flags(const GraphEntry *graph, int flags);
static int graph_focus_overlay_flags(const GraphEntry *graph, const GraphElementEntry *focus, const char *highlight_mode);
static const char *graph_view_group_name(const GraphEntry *graph, const GraphElementEntry *focus, int flags);
static const char *build_graph_visual_body(int index, int focus_index, int mode_flags);


static ViewState *current_view(void)
{
    return &g_app.stack[g_app.depth - 1];
}

static const char *safe_str(const char *value)
{
    return value ? value : "";
}

static int clamp_index_or_zero(int index, int count)
{
    if(count <= 0) return 0;
    if(index < 0) return 0;
    if(index >= count) return count - 1;
    return index;
}

static int clamp_page_or_zero(int page, int count)
{
    if(count <= 0) return 0;
    if(page < 0) return 0;
    if(page >= count) return count - 1;
    return page;
}

static const UnitEntry *unit_entry_at(int index)
{
    if(index < 0 || index >= UNIT_COUNT) return NULL;
    return &g_units[index];
}

static const TopicEntry *topic_entry_at(int index)
{
    if(index < 0 || index >= TOPIC_COUNT) return NULL;
    return &g_topics[index];
}

static const VocabularyEntry *vocab_entry_at(int index)
{
    if(index < 0 || index >= VOCAB_COUNT) return NULL;
    return &g_vocabulary[index];
}

static const ConceptEntry *concept_entry_at(int index)
{
    if(index < 0 || index >= CONCEPT_COUNT) return NULL;
    return &g_concepts[index];
}

static const GraphEntry *graph_entry_at(int index)
{
    if(index < 0 || index >= GRAPH_COUNT) return NULL;
    return &g_graphs[index];
}

static const FormulaEntry *formula_entry_at(int index)
{
    if(index < 0 || index >= FORMULA_COUNT) return NULL;
    return &g_formulas[index];
}

static const StructureEntry *structure_entry_at(int index)
{
    if(index < 0 || index >= STRUCTURE_COUNT) return NULL;
    return &g_structures[index];
}

static const TextSheetEntry *quick_entry_at(int index)
{
    if(index < 0 || index >= QUICK_REVIEW_COUNT) return NULL;
    return &g_quick_review[index];
}

static const TextSheetEntry *exam_cram_entry_at(int index)
{
    if(index < 0 || index >= EXAM_CRAM_COUNT) return NULL;
    return &g_exam_cram[index];
}

static const TextSheetEntry *reference_entry_at(int index)
{
    if(index < 0 || index >= REFERENCE_COUNT) return NULL;
    return &g_reference[index];
}

static const TextSheetEntry *audit_entry_at(int index)
{
    if(index < 0 || index >= AUDIT_COUNT) return NULL;
    return &g_source_audit[index];
}

static const char *unit_title_at(int index)
{
    const UnitEntry *entry = unit_entry_at(index);
    return entry ? safe_str(entry->title) : "AP Unit";
}

static const char *topic_title_at(int index)
{
    const TopicEntry *entry = topic_entry_at(index);
    return entry ? safe_str(entry->title) : "Topic";
}

static const char *vocab_term_at(int index)
{
    const VocabularyEntry *entry = vocab_entry_at(index);
    return entry ? safe_str(entry->term) : "Vocabulary";
}

static const char *concept_title_at(int index)
{
    const ConceptEntry *entry = concept_entry_at(index);
    return entry ? safe_str(entry->title) : "Concept";
}

static const char *graph_title_at(int index)
{
    const GraphEntry *entry = graph_entry_at(index);
    return entry ? safe_str(entry->title) : "Graph";
}

static const char *formula_title_at(int index)
{
    const FormulaEntry *entry = formula_entry_at(index);
    return entry ? safe_str(entry->title) : "Formula";
}

static const char *structure_title_at(int index)
{
    const StructureEntry *entry = structure_entry_at(index);
    return entry ? safe_str(entry->title) : "Structure";
}

static const char *sheet_title_or(const TextSheetEntry *entry, const char *fallback)
{
    return (entry && entry->title && entry->title[0]) ? entry->title : fallback;
}

static const char *sheet_body_or(const TextSheetEntry *entry)
{
    return (entry && entry->body) ? entry->body : "No content is available for this page.";
}

static const char *build_unavailable_body(const char *message)
{
    g_page_body[0] = 0;
    append_section(g_page_body, sizeof g_page_body, "Unavailable", message ? message : "This content is not available.");
    return g_page_body;
}

static void sanitize_view_state(ViewState *view);

static ViewState *push_empty_view(ViewType type)
{
    if(g_app.depth >= STACK_DEPTH) return NULL;
    memset(&g_app.stack[g_app.depth], 0, sizeof g_app.stack[g_app.depth]);
    g_app.stack[g_app.depth].type = type;
    g_app.depth++;
    return &g_app.stack[g_app.depth - 1];
}

static void pop_view(void)
{
    if(g_app.depth > 1) g_app.depth--;
}

static void enter_home_view(void)
{
    memset(&g_app.stack[0], 0, sizeof g_app.stack[0]);
    g_app.stack[0].type = VIEW_HOME;
}

static void enter_units_view(void)
{
    ViewState *view = push_empty_view(VIEW_UNITS);
    if(view) ensure_list_window(&view->screen.units.cursor, UNIT_COUNT, 9);
}

static void enter_unit_detail_view(int unit_index)
{
    ViewState *view = push_empty_view(VIEW_UNIT_DETAIL);
    if(!view) return;
    view->screen.unit_detail.unit_index = unit_index;
}

static void enter_topic_list_view(int unit_index)
{
    ViewState *view = push_empty_view(VIEW_TOPIC_LIST);
    if(!view) return;
    view->screen.topic_list.unit_index = unit_index;
}

static void enter_topic_detail_view(int topic_index)
{
    ViewState *view = push_empty_view(VIEW_TOPIC_DETAIL);
    if(!view) return;
    view->screen.topic_detail.topic_index = topic_index;
}

static void enter_concept_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_CONCEPT_LIST);
    if(view) ensure_list_window(&view->screen.concept_list.cursor, CONCEPT_COUNT, 8);
}

static void enter_concept_detail_view(int concept_index)
{
    ViewState *view = push_empty_view(VIEW_CONCEPT_DETAIL);
    if(!view) return;
    view->screen.concept_detail.concept_index = concept_index;
}

static void enter_vocab_mode_view(void)
{
    ViewState *view = push_empty_view(VIEW_VOCAB_MODE);
    if(view) ensure_list_window(&view->screen.vocab_mode.cursor, VOCAB_MODE_COUNT, 8);
}

static void enter_vocab_unit_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_VOCAB_UNIT_LIST);
    if(view) ensure_list_window(&view->screen.vocab_unit_list.cursor, UNIT_COUNT, 9);
}

static void enter_vocab_category_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_VOCAB_CATEGORY_LIST);
    if(view) ensure_list_window(&view->screen.vocab_category_list.cursor, CATEGORY_COUNT, 9);
}

static void enter_vocab_list_view(int group_mode, int unit_filter_index, int category_filter_index)
{
    ViewState *view = push_empty_view(VIEW_VOCAB_LIST);
    if(!view) return;
    view->screen.vocab_list.group_mode = group_mode;
    view->screen.vocab_list.unit_filter_index = unit_filter_index;
    view->screen.vocab_list.category_filter_index = category_filter_index;
}

static void enter_vocab_detail_view(int vocab_index)
{
    ViewState *view = push_empty_view(VIEW_VOCAB_DETAIL);
    if(!view) return;
    view->screen.vocab_detail.vocab_index = vocab_index;
}

static void enter_graph_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_GRAPH_LIST);
    if(view) ensure_list_window(&view->screen.graph_list.cursor, GRAPH_COUNT, 8);
}

static void enter_graph_detail_view(int graph_index, int page, int focus_index, int mode_flags)
{
    ViewState *view = push_empty_view(VIEW_GRAPH_DETAIL);
    const GraphEntry *graph = graph_entry_at(graph_index);
    int element_count = 0;
    if(!view) return;
    view->screen.graph_detail.graph_index = graph ? graph_index : 0;
    view->screen.graph_detail.detail.page = clamp_page_or_zero(page, GRAPH_PAGE_COUNT);
    view->screen.graph_detail.mode_flags = (mode_flags < 0) ? graph_default_mode_flags(graph) : sanitize_graph_mode_flags(graph, mode_flags);
    view->screen.graph_detail.focus_index = focus_index;
    element_count = graph ? graphs_get_element_count(graph) : 0;
    if(view->screen.graph_detail.focus_index < 0) view->screen.graph_detail.focus_index = 0;
    if(element_count <= 0) view->screen.graph_detail.focus_index = 0;
    else if(view->screen.graph_detail.focus_index >= element_count) view->screen.graph_detail.focus_index = element_count - 1;
}

static void enter_formula_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_FORMULA_LIST);
    if(view) ensure_list_window(&view->screen.formula_list.cursor, FORMULA_COUNT, 8);
}

static void enter_formula_detail_view(int formula_index)
{
    ViewState *view = push_empty_view(VIEW_FORMULA_DETAIL);
    if(!view) return;
    view->screen.formula_detail.formula_index = formula_index;
}

static void enter_structure_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_STRUCTURE_LIST);
    if(view) ensure_list_window(&view->screen.structure_list.cursor, STRUCTURE_COUNT, 8);
}

static void enter_structure_detail_view(int structure_index)
{
    ViewState *view = push_empty_view(VIEW_STRUCTURE_DETAIL);
    if(!view) return;
    view->screen.structure_detail.structure_index = structure_index;
}

static void enter_revision_menu_view(void)
{
    ViewState *view = push_empty_view(VIEW_REVISION_MENU);
    if(view) ensure_list_window(&view->screen.revision_menu.cursor, REVISION_ITEM_COUNT, 8);
}

static void enter_quick_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_QUICK_LIST);
    if(view) ensure_list_window(&view->screen.quick_list.cursor, QUICK_REVIEW_COUNT, 9);
}

static void enter_quick_detail_view(int entry_index)
{
    ViewState *view = push_empty_view(VIEW_QUICK_DETAIL);
    if(!view) return;
    view->screen.quick_detail.entry_index = entry_index;
}

static void enter_exam_cram_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_EXAM_CRAM_LIST);
    if(view) ensure_list_window(&view->screen.exam_cram_list.cursor, EXAM_CRAM_COUNT, 9);
}

static void enter_exam_cram_detail_view(int entry_index)
{
    ViewState *view = push_empty_view(VIEW_EXAM_CRAM_DETAIL);
    if(!view) return;
    view->screen.exam_cram_detail.entry_index = entry_index;
}

static void enter_reference_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_REFERENCE_LIST);
    if(view) ensure_list_window(&view->screen.reference_list.cursor, REFERENCE_COUNT, 9);
}

static void enter_reference_detail_view(int entry_index)
{
    ViewState *view = push_empty_view(VIEW_REFERENCE_DETAIL);
    if(!view) return;
    view->screen.reference_detail.entry_index = entry_index;
}

static void enter_recent_list_view(void)
{
    ViewState *view = push_empty_view(VIEW_RECENT_LIST);
    if(view) ensure_list_window(&view->screen.recent_list.cursor, g_app.recent_count, 9);
}

static void enter_about_view(void)
{
    push_empty_view(VIEW_ABOUT);
}

static void enter_audit_view(void)
{
    push_empty_view(VIEW_AUDIT);
}

static void enter_result_detail_view(void)
{
    push_empty_view(VIEW_RESULT_DETAIL);
}

static void append_text(char *buffer, size_t size, const char *text)
{
    size_t len = 0;
    size_t remaining = 0;
    if(!buffer || !text || size == 0) return;
    len = strlen(buffer);
    if(len >= size - 1) return;
    remaining = size - len - 1;
    strncat(buffer, text, remaining);
}

static void append_hint_piece(char *buffer, size_t size, const char *text)
{
    if(!buffer || !text || !text[0]) return;
    if(buffer[0]) append_text(buffer, size, "  ");
    append_text(buffer, size, text);
}

static void append_section(char *buffer, size_t size, const char *title, const char *value)
{
    if(!value || !value[0]) return;
    if(buffer[0]) append_text(buffer, size, "\n\n");
    if(title && title[0]) {
        append_text(buffer, size, title);
        append_text(buffer, size, "\n");
    }
    append_text(buffer, size, value);
}

static void first_csv_item(const char *source, char *buffer, size_t size)
{
    size_t length = 0;
    if(size == 0) return;
    buffer[0] = 0;
    if(!source || !source[0]) return;
    while(*source == ' ') source++;
    while(source[length] && source[length] != ',' && length < size - 1) {
        buffer[length] = source[length];
        length++;
    }
    while(length > 0 && buffer[length - 1] == ' ') length--;
    buffer[length] = 0;
}

static bool csv_contains_token(const char *csv, const char *token)
{
    size_t token_len = 0;
    const char *cursor = csv;
    if(!csv || !token || !token[0]) return false;
    token_len = strlen(token);
    while(*cursor) {
        while(*cursor == ' ' || *cursor == ',') cursor++;
        if(strncmp(cursor, token, token_len) == 0) {
            char end = cursor[token_len];
            if(end == 0 || end == ',') return true;
        }
        while(*cursor && *cursor != ',') cursor++;
    }
    return false;
}

static bool string_flag_enabled(const char *value)
{
    return value && value[0] && strcmp(value, "0") != 0 && strcmp(value, "false") != 0;
}

static void first_focus_id(const char *graph_element_id, const char *point_ids_csv, const char *curve_ids_csv, const char *region_ids_csv, char *buffer, size_t size)
{
    if(size == 0) return;
    buffer[0] = 0;
    if(graph_element_id && graph_element_id[0]) {
        snprintf(buffer, size, "%s", graph_element_id);
        return;
    }
    first_csv_item(point_ids_csv, buffer, size);
    if(buffer[0]) return;
    first_csv_item(curve_ids_csv, buffer, size);
    if(buffer[0]) return;
    first_csv_item(region_ids_csv, buffer, size);
}

static int find_unit_index_by_id(const char *id)
{
    int i = 0;
    if(!id || !id[0]) return -1;
    for(i = 0; i < UNIT_COUNT; i++) {
        if(strcmp(g_units[i].id, id) == 0) return i;
    }
    return -1;
}

static const char *unit_title_from_id(const char *id)
{
    int index = find_unit_index_by_id(id);
    return (index >= 0) ? g_units[index].title : "AP Microeconomics";
}

static const char *topic_title_from_id(const char *id)
{
    int i = 0;
    if(!id || !id[0]) return "";
    for(i = 0; i < TOPIC_COUNT; i++) {
        if(strcmp(g_topics[i].id, id) == 0) return g_topics[i].title;
    }
    return "";
}

static void append_csv_title(char *buffer, size_t size, const char *title)
{
    if(!title || !title[0]) return;
    if(buffer[0]) append_text(buffer, size, ", ");
    append_text(buffer, size, title);
}

static void unit_titles_from_csv(const char *csv, char *buffer, size_t size)
{
    const char *cursor = csv;
    buffer[0] = 0;
    while(cursor && *cursor) {
        char item[64];
        size_t length = 0;
        while(*cursor == ' ' || *cursor == ',') cursor++;
        while(cursor[length] && cursor[length] != ',' && length < sizeof item - 1) {
            item[length] = cursor[length];
            length++;
        }
        item[length] = 0;
        while(length > 0 && item[length - 1] == ' ') item[--length] = 0;
        if(item[0]) append_csv_title(buffer, size, unit_title_from_id(item));
        cursor += length;
        while(*cursor && *cursor != ',') cursor++;
    }
}

static void topic_titles_from_csv(const char *csv, char *buffer, size_t size)
{
    const char *cursor = csv;
    buffer[0] = 0;
    while(cursor && *cursor) {
        char item[96];
        size_t length = 0;
        while(*cursor == ' ' || *cursor == ',') cursor++;
        while(cursor[length] && cursor[length] != ',' && length < sizeof item - 1) {
            item[length] = cursor[length];
            length++;
        }
        item[length] = 0;
        while(length > 0 && item[length - 1] == ' ') item[--length] = 0;
        if(item[0]) append_csv_title(buffer, size, topic_title_from_id(item));
        cursor += length;
        while(*cursor && *cursor != ',') cursor++;
    }
}

static int find_vocab_index_by_term(const char *term)
{
    int i = 0;
    if(!term || !term[0]) return -1;
    for(i = 0; i < VOCAB_COUNT; i++) {
        if(strcmp(g_vocabulary[i].term, term) == 0) return i;
        if(strcmp(g_vocabulary[i].canonical_term, term) == 0) return i;
        if(csv_contains_token(g_vocabulary[i].aliases_csv, term)) return i;
    }
    return -1;
}

static int find_graph_index_by_id(const char *id)
{
    int i = 0;
    if(!id || !id[0]) return -1;
    for(i = 0; i < GRAPH_COUNT; i++) {
        if(strcmp(g_graphs[i].id, id) == 0) return i;
    }
    return -1;
}

static int find_graph_index_by_title(const char *title)
{
    int i = 0;
    for(i = 0; i < GRAPH_COUNT; i++) {
        if(strcmp(g_graphs[i].title, title) == 0) return i;
    }
    return -1;
}

static int find_formula_index_by_title(const char *title)
{
    int i = 0;
    for(i = 0; i < FORMULA_COUNT; i++) {
        if(strcmp(g_formulas[i].title, title) == 0) return i;
    }
    return -1;
}

static void add_recent(ViewType type, int index, int page, int focus_index, int mode_flags, const char *title)
{
    int i = 0;
    int insert = 0;
    for(i = 0; i < g_app.recent_count; i++) {
        if(g_app.recent[i].type == type && g_app.recent[i].index == index) {
            RecentEntry entry = g_app.recent[i];
            entry.page = page;
            entry.focus_index = focus_index;
            entry.mode_flags = mode_flags;
            snprintf(entry.title, sizeof entry.title, "%s", title ? title : "Recent item");
            for(; i > 0; i--) g_app.recent[i] = g_app.recent[i - 1];
            g_app.recent[0] = entry;
            return;
        }
    }
    if(g_app.recent_count < RECENT_CAPACITY) g_app.recent_count++;
    insert = g_app.recent_count - 1;
    for(i = insert; i > 0; i--) g_app.recent[i] = g_app.recent[i - 1];
    g_app.recent[0].type = type;
    g_app.recent[0].index = index;
    g_app.recent[0].page = page;
    g_app.recent[0].focus_index = focus_index;
    g_app.recent[0].mode_flags = mode_flags;
    snprintf(g_app.recent[0].title, sizeof g_app.recent[0].title, "%s", title ? title : "Recent item");
}

static void open_unit_detail(int index)
{
    const UnitEntry *entry = unit_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That unit page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_UNIT_DETAIL, index, 0, 0, 0, entry->title);
    enter_unit_detail_view(index);
}

static void open_topic_detail(int index)
{
    const TopicEntry *entry = topic_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That topic page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_TOPIC_DETAIL, index, 0, 0, 0, entry->title);
    enter_topic_detail_view(index);
}

static void open_concept_detail(int index)
{
    const ConceptEntry *entry = concept_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That concept page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_CONCEPT_DETAIL, index, 0, 0, 0, entry->title);
    enter_concept_detail_view(index);
}

static void open_vocab_detail(int index)
{
    const VocabularyEntry *entry = vocab_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That vocabulary page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_VOCAB_DETAIL, index, 0, 0, 0, entry->term);
    enter_vocab_detail_view(index);
}

static void open_graph_detail_focused(int index, int page, int focus_index, int mode_flags)
{
    const GraphEntry *entry = graph_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That graph page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_GRAPH_DETAIL, index, page, focus_index, mode_flags, entry->title);
    enter_graph_detail_view(index, page, focus_index, mode_flags);
}

static void open_graph_detail(int index, int page)
{
    const GraphEntry *entry = graph_entry_at(index);
    open_graph_detail_focused(index, page, 0, graph_default_mode_flags(entry));
}

static GraphRenderOptions graph_options_from_flags(int flags)
{
    GraphRenderOptions options;
    options.show_labels = (flags & GRAPH_MODE_LABELS) != 0;
    options.show_points = (flags & GRAPH_MODE_POINTS) != 0;
    options.show_area = (flags & GRAPH_MODE_AREA) != 0;
    options.show_concepts = (flags & GRAPH_MODE_CONCEPTS) != 0;
    options.show_shift = (flags & GRAPH_MODE_SHIFT) != 0;
    options.show_info = (flags & GRAPH_MODE_INFO) != 0;
    return options;
}

static int graph_default_mode_flags(const GraphEntry *graph)
{
    const GraphCapabilities *capabilities = graphs_get_capabilities(graph);
    if(capabilities) return capabilities->default_flags;
    return GRAPH_MODE_LABELS | GRAPH_MODE_POINTS | GRAPH_MODE_INFO;
}

static int graph_supported_mode_flags(const GraphEntry *graph)
{
    const GraphCapabilities *capabilities = graphs_get_capabilities(graph);
    if(capabilities) return capabilities->supported_flags;
    return GRAPH_MODE_LABELS | GRAPH_MODE_POINTS | GRAPH_MODE_AREA | GRAPH_MODE_CONCEPTS | GRAPH_MODE_SHIFT | GRAPH_MODE_INFO;
}

static const char *graph_toggle_label(const GraphEntry *graph, int slot)
{
    const GraphCapabilities *capabilities = graphs_get_capabilities(graph);
    if(!capabilities || slot < 0 || slot >= 3 || capabilities->toggle_flags[slot] == 0) return "";
    return safe_str(capabilities->toggle_labels[slot]);
}

static int graph_toggle_flag(const GraphEntry *graph, int slot)
{
    const GraphCapabilities *capabilities = graphs_get_capabilities(graph);
    if(!capabilities || slot < 0 || slot >= 3) return 0;
    return capabilities->toggle_flags[slot];
}

static int sanitize_graph_mode_flags(const GraphEntry *graph, int flags)
{
    int supported = graph_supported_mode_flags(graph);
    int sanitized = flags & supported;
    if((supported & GRAPH_MODE_INFO) != 0) sanitized |= GRAPH_MODE_INFO;
    return sanitized;
}

static int graph_focus_overlay_flags(const GraphEntry *graph, const GraphElementEntry *focus, const char *highlight_mode)
{
    int flags = graph_default_mode_flags(graph);
    bool is_region = false;
    bool is_point = false;
    bool is_concept = false;
    if(highlight_mode && highlight_mode[0]) {
        if(csv_contains_token(highlight_mode, "area")) flags |= GRAPH_MODE_AREA;
        if(csv_contains_token(highlight_mode, "concept")) flags |= GRAPH_MODE_CONCEPTS;
        if(csv_contains_token(highlight_mode, "shift")) flags |= GRAPH_MODE_SHIFT;
        if(csv_contains_token(highlight_mode, "point")) flags |= GRAPH_MODE_POINTS;
        return sanitize_graph_mode_flags(graph, flags);
    }
    if(!focus) return sanitize_graph_mode_flags(graph, flags);

    is_region = focus->area_capable || strcmp(focus->type, "region") == 0 || strcmp(focus->type, "gap") == 0 || strcmp(focus->type, "matrix cell") == 0;
    is_point = strcmp(focus->type, "point") == 0 || strcmp(focus->type, "line") == 0 || strcmp(focus->type, "curve") == 0;
    is_concept =
        strcmp(focus->type, "policy") == 0 ||
        strcmp(focus->type, "movement") == 0 ||
        strcmp(focus->type, "behavior") == 0 ||
        strcmp(focus->type, "market failure") == 0 ||
        strcmp(focus->type, "classification") == 0 ||
        strcmp(focus->type, "overuse") == 0 ||
        strcmp(focus->type, "decision outcome") == 0 ||
        strcmp(focus->type, "table comparison") == 0 ||
        strcmp(focus->type, "matrix logic") == 0 ||
        strcmp(focus->type, "matrix strategy") == 0 ||
        strcmp(focus->type, "outcome note") == 0;

    flags = GRAPH_MODE_LABELS | GRAPH_MODE_INFO;
    if(is_point || is_region) flags |= GRAPH_MODE_POINTS;
    if(is_region) {
        flags |= GRAPH_MODE_AREA;
    }
    if(is_concept) {
        flags |= GRAPH_MODE_CONCEPTS;
    }
    if(focus->shift_capable || strcmp(focus->type, "policy") == 0 || strcmp(focus->id, "corrective-tax") == 0 || strcmp(focus->id, "subsidy") == 0) {
        flags |= GRAPH_MODE_SHIFT;
    }
    return sanitize_graph_mode_flags(graph, flags);
}

static const char *graph_view_group_name(const GraphEntry *graph, const GraphElementEntry *focus, int flags)
{
    int supported = graph_supported_mode_flags(graph);
    (void)focus;
    if((flags & GRAPH_MODE_SHIFT) != 0 && (supported & GRAPH_MODE_SHIFT) != 0) return "After";
    if((flags & GRAPH_MODE_AREA) != 0 && (supported & GRAPH_MODE_AREA) != 0) return "Areas";
    if((flags & GRAPH_MODE_CONCEPTS) != 0 && (supported & GRAPH_MODE_CONCEPTS) != 0) return "Concepts";
    if((flags & GRAPH_MODE_POINTS) != 0 && (supported & GRAPH_MODE_POINTS) != 0) return "Points";
    if((flags & GRAPH_MODE_LABELS) != 0 && (supported & GRAPH_MODE_LABELS) != 0) return "Labels";
    return "Base";
}

static int resolve_graph_focus_index(const GraphEntry *graph, const char *graph_element_id, const char *related_point_ids_csv, const char *related_curve_ids_csv, const char *related_region_ids_csv, const char *term)
{
    char preferred_id[64];
    int focus_index = -1;
    if(!graph) return 0;
    first_focus_id(graph_element_id, related_point_ids_csv, related_curve_ids_csv, related_region_ids_csv, preferred_id, sizeof preferred_id);
    if(preferred_id[0]) focus_index = graphs_find_element_by_id(graph, preferred_id);
    if(focus_index < 0 && term && term[0]) focus_index = graphs_find_element_by_term(graph, term);
    if(focus_index < 0) focus_index = 0;
    return focus_index;
}

static int resolve_graph_index(const char *csv, bool ids_allowed)
{
    char item[128];
    int index = -1;
    first_csv_item(csv, item, sizeof item);
    if(!item[0]) return -1;
    index = find_graph_index_by_title(item);
    if(index < 0 || ids_allowed) {
        int id_index = find_graph_index_by_id(item);
        if(id_index >= 0) index = id_index;
    }
    return index;
}

static void open_formula_detail(int index)
{
    const FormulaEntry *entry = formula_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That formula card is not available in the current data set.");
        return;
    }
    add_recent(VIEW_FORMULA_DETAIL, index, 0, 0, 0, entry->title);
    enter_formula_detail_view(index);
}

static void open_structure_detail(int index)
{
    const StructureEntry *entry = structure_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That structure page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_STRUCTURE_DETAIL, index, 0, 0, 0, entry->title);
    enter_structure_detail_view(index);
}

static void open_quick_detail(int index)
{
    const TextSheetEntry *entry = quick_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That quick review sheet is not available in the current data set.");
        return;
    }
    add_recent(VIEW_QUICK_DETAIL, index, 0, 0, 0, entry->title);
    enter_quick_detail_view(index);
}

static void open_exam_cram_detail(int index)
{
    const TextSheetEntry *entry = exam_cram_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That exam cram page is not available in the current data set.");
        return;
    }
    add_recent(VIEW_EXAM_CRAM_DETAIL, index, 0, 0, 0, entry->title);
    enter_exam_cram_detail_view(index);
}

static void open_reference_detail(int index)
{
    const TextSheetEntry *entry = reference_entry_at(index);
    if(!entry) {
        set_result_text("Unavailable", "That reference sheet is not available in the current data set.");
        return;
    }
    add_recent(VIEW_REFERENCE_DETAIL, index, 0, 0, 0, entry->title);
    enter_reference_detail_view(index);
}

static void open_recent_entry(const RecentEntry *entry)
{
    ViewState *view = NULL;

    if(!entry) {
        set_result_text("Unavailable", "That recent page could not be reopened.");
        return;
    }
    switch(entry->type) {
        case VIEW_UNIT_DETAIL:
            open_unit_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_UNIT_DETAIL) {
                sanitize_view_state(view);
                view->screen.unit_detail.text.page = clamp_page_or_zero(entry->page, UNIT_PAGE_COUNT);
                reset_text_viewport(&view->screen.unit_detail.text);
            }
            break;
        case VIEW_TOPIC_DETAIL:
            open_topic_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_TOPIC_DETAIL) {
                sanitize_view_state(view);
                view->screen.topic_detail.text.page = clamp_page_or_zero(entry->page, TOPIC_PAGE_COUNT);
                reset_text_viewport(&view->screen.topic_detail.text);
            }
            break;
        case VIEW_CONCEPT_DETAIL:
            open_concept_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_CONCEPT_DETAIL) {
                sanitize_view_state(view);
                view->screen.concept_detail.detail.page = clamp_page_or_zero(entry->page, CONCEPT_PAGE_COUNT);
                reset_paged_viewport(&view->screen.concept_detail.detail);
            }
            break;
        case VIEW_VOCAB_DETAIL:
            open_vocab_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_VOCAB_DETAIL) {
                sanitize_view_state(view);
                view->screen.vocab_detail.detail.page = clamp_page_or_zero(entry->page, VOCAB_PAGE_COUNT);
                reset_paged_viewport(&view->screen.vocab_detail.detail);
            }
            break;
        case VIEW_GRAPH_DETAIL: open_graph_detail_focused(entry->index, entry->page, entry->focus_index, entry->mode_flags); break;
        case VIEW_FORMULA_DETAIL:
            open_formula_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_FORMULA_DETAIL) {
                sanitize_view_state(view);
                view->screen.formula_detail.text.page = clamp_page_or_zero(entry->page, FORMULA_PAGE_COUNT);
                reset_text_viewport(&view->screen.formula_detail.text);
            }
            break;
        case VIEW_STRUCTURE_DETAIL:
            open_structure_detail(entry->index);
            view = current_view();
            if(view && view->type == VIEW_STRUCTURE_DETAIL) {
                sanitize_view_state(view);
                view->screen.structure_detail.text.page = clamp_page_or_zero(entry->page, STRUCTURE_PAGE_COUNT);
                reset_text_viewport(&view->screen.structure_detail.text);
            }
            break;
        case VIEW_QUICK_DETAIL: open_quick_detail(entry->index); break;
        case VIEW_EXAM_CRAM_DETAIL: open_exam_cram_detail(entry->index); break;
        case VIEW_REFERENCE_DETAIL: open_reference_detail(entry->index); break;
        default: break;
    }
}

static void open_first_graph(const char *csv, bool ids_allowed)
{
    int index = resolve_graph_index(csv, ids_allowed);
    if(index >= 0) open_graph_detail(index, 0);
    else if(!csv || !csv[0]) set_result_text("Missing Link", "No related graph is attached to this page.");
    else set_result_text("Missing Link", "The related graph link could not be resolved in the current data set.");
}

static void open_first_graph_with_focus(const char *csv, bool ids_allowed, const char *graph_element_id, const char *related_point_ids_csv, const char *related_curve_ids_csv, const char *related_region_ids_csv, const char *term, const char *highlight_mode)
{
    int index = resolve_graph_index(csv, ids_allowed);
    const GraphEntry *graph = graph_entry_at(index);
    int focus_index = 0;
    const GraphElementEntry *focus = NULL;
    if(!graph) {
        if(!csv || !csv[0]) set_result_text("Missing Link", "No related graph is attached to this page.");
        else set_result_text("Missing Link", "The related graph link could not be resolved in the current data set.");
        return;
    }
    focus_index = resolve_graph_focus_index(graph, graph_element_id, related_point_ids_csv, related_curve_ids_csv, related_region_ids_csv, term);
    focus = graphs_get_element(graph, focus_index);
    open_graph_detail_focused(index, 0, focus_index, graph_focus_overlay_flags(graph, focus, highlight_mode));
}

static int resolve_vocab_graph_index(const VocabularyEntry *entry)
{
    if(!entry) return -1;
    if(entry->graph_id[0]) {
        int graph_index = find_graph_index_by_id(entry->graph_id);
        if(graph_index >= 0) return graph_index;
    }
    if(entry->related_graph_ids_csv[0]) {
        int graph_index = resolve_graph_index(entry->related_graph_ids_csv, true);
        if(graph_index >= 0) return graph_index;
    }
    if(entry->graph_name[0]) {
        int graph_index = resolve_graph_index(entry->graph_name, true);
        if(graph_index >= 0) return graph_index;
    }
    return resolve_graph_index(entry->related_graphs, false);
}

static const GraphEntry *resolve_vocab_graph(const VocabularyEntry *entry, int *graph_index_out)
{
    int graph_index = resolve_vocab_graph_index(entry);
    if(graph_index_out) *graph_index_out = graph_index;
    return graph_entry_at(graph_index);
}

static const GraphElementEntry *resolve_vocab_focus(const VocabularyEntry *entry, const GraphEntry *graph, int *focus_index_out)
{
    int focus_index = 0;
    if(!entry || !graph) {
        if(focus_index_out) *focus_index_out = 0;
        return NULL;
    }
    focus_index = resolve_graph_focus_index(
        graph,
        entry->graph_element_id,
        entry->related_point_ids_csv,
        entry->related_curve_ids_csv,
        entry->related_region_ids_csv,
        entry->term
    );
    if(focus_index_out) *focus_index_out = focus_index;
    return graphs_get_element(graph, focus_index);
}

static void open_first_formula(const char *csv)
{
    char item[128];
    int index = -1;
    first_csv_item(csv, item, sizeof item);
    if(!item[0]) {
        set_result_text("Missing Link", "No related formula is attached to this page.");
        return;
    }
    index = find_formula_index_by_title(item);
    if(index >= 0) open_formula_detail(index);
    else set_result_text("Missing Link", "The related formula link could not be resolved in the current data set.");
}

static void open_first_vocab(const char *csv)
{
    char item[128];
    int index = -1;
    first_csv_item(csv, item, sizeof item);
    if(!item[0]) {
        set_result_text("Missing Link", "No related vocabulary term is attached to this page.");
        return;
    }
    index = find_vocab_index_by_term(item);
    if(index >= 0) open_vocab_detail(index);
    else set_result_text("Missing Link", "The related vocabulary link could not be resolved in the current data set.");
}

static void open_graph_focus_term(const GraphEntry *graph, const GraphElementEntry *focus)
{
    if(focus && focus->related_terms && focus->related_terms[0]) {
        open_first_vocab(focus->related_terms);
        return;
    }
    if(graph) open_first_vocab(graph->related_terms);
}

static void open_graph_focus_formula(const GraphEntry *graph, const GraphElementEntry *focus)
{
    if(focus && focus->related_formulas && focus->related_formulas[0]) {
        open_first_formula(focus->related_formulas);
        return;
    }
    if(graph) open_first_formula(graph->related_formulas);
}

static void sort_vocab_indices_alpha(int indices[MAX_FILTERED_RESULTS], int count)
{
    int i = 0;
    int j = 0;
    if(!indices || count <= 1) return;
    for(i = 0; i < count; i++) {
        for(j = i + 1; j < count; j++) {
            const VocabularyEntry *left = vocab_entry_at(indices[i]);
            const VocabularyEntry *right = vocab_entry_at(indices[j]);
            const char *left_term = left ? safe_str(left->term) : "";
            const char *right_term = right ? safe_str(right->term) : "";
            if(strcmp(left_term, right_term) > 0) {
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }
}

static int filtered_topics(int unit_index, int out_indices[MAX_FILTERED_RESULTS])
{
    int count = 0;
    const UnitEntry *unit = unit_entry_at(unit_index);
    const char *unit_id = unit ? unit->id : NULL;
    int i = 0;
    if(!out_indices) return 0;
    for(i = 0; i < TOPIC_COUNT && count < MAX_FILTERED_RESULTS; i++) {
        const TopicEntry *topic = topic_entry_at(i);
        if(topic && (!unit_id || csv_contains_token(topic->unit_ids_csv, unit_id))) out_indices[count++] = i;
    }
    return count;
}

static int filtered_vocab(int mode, int filter, int out_indices[MAX_FILTERED_RESULTS])
{
    int count = 0;
    int i = 0;
    if(!out_indices) return 0;
    for(i = 0; i < VOCAB_COUNT && count < MAX_FILTERED_RESULTS; i++) {
        const VocabularyEntry *entry = vocab_entry_at(i);
        bool keep = false;
        const UnitEntry *unit = unit_entry_at(filter);
        if(!entry) continue;
        if(mode == 0) keep = unit && csv_contains_token(entry->unit_ids_csv, unit->id);
        else if(mode == 1) keep = true;
        else if(mode == 2) keep = (filter >= 0 && filter < CATEGORY_COUNT) && strcmp(safe_str(entry->category), k_categories[filter]) == 0;
        if(keep) out_indices[count++] = i;
    }
    sort_vocab_indices_alpha(out_indices, count);
    return count;
}

static void ensure_list_window(ListCursorState *cursor, int count, int visible_rows)
{
    if(!cursor) return;
    if(visible_rows <= 0) visible_rows = 1;
    if(cursor->selected < 0) cursor->selected = 0;
    if(count <= 0) {
        cursor->selected = 0;
        cursor->top = 0;
        return;
    }
    if(cursor->selected >= count) cursor->selected = count - 1;
    if(cursor->selected < cursor->top) cursor->top = cursor->selected;
    if(cursor->selected >= cursor->top + visible_rows) cursor->top = cursor->selected - visible_rows + 1;
    if(cursor->top < 0) cursor->top = 0;
    if(cursor->top > count - 1) cursor->top = count - 1;
    if(cursor->top > cursor->selected) cursor->top = cursor->selected;
}

static void reset_text_viewport(TextScrollState *text)
{
    if(!text) return;
    text->scroll = 0;
    text->x_offset = 0;
}

static void reset_paged_viewport(PagedTextState *detail)
{
    if(!detail) return;
    detail->scroll = 0;
    detail->x_offset = 0;
}

static bool change_text_section(TextScrollState *text, int page_count, int delta)
{
    int next_page = 0;
    if(!text || page_count <= 0 || delta == 0) return false;
    next_page = text->page + delta;
    if(next_page < 0 || next_page >= page_count) return false;
    text->page = next_page;
    reset_text_viewport(text);
    return true;
}

static bool change_paged_section(PagedTextState *detail, int page_count, int delta)
{
    int next_page = 0;
    if(!detail || page_count <= 0 || delta == 0) return false;
    next_page = detail->page + delta;
    if(next_page < 0 || next_page >= page_count) return false;
    detail->page = next_page;
    reset_paged_viewport(detail);
    return true;
}

static void sanitize_view_state(ViewState *view)
{
    if(!view) return;

    switch(view->type) {
        case VIEW_HOME:
            ensure_list_window(&view->screen.home.cursor, HOME_COUNT, 8);
            break;
        case VIEW_UNITS:
            ensure_list_window(&view->screen.units.cursor, UNIT_COUNT, 9);
            break;
        case VIEW_UNIT_DETAIL:
            view->screen.unit_detail.unit_index = clamp_index_or_zero(view->screen.unit_detail.unit_index, UNIT_COUNT);
            view->screen.unit_detail.text.page = clamp_page_or_zero(view->screen.unit_detail.text.page, UNIT_PAGE_COUNT);
            if(view->screen.unit_detail.text.scroll < 0) view->screen.unit_detail.text.scroll = 0;
            if(view->screen.unit_detail.text.x_offset < 0) view->screen.unit_detail.text.x_offset = 0;
            break;
        case VIEW_TOPIC_LIST:
            if(view->screen.topic_list.unit_index < -1 || view->screen.topic_list.unit_index >= UNIT_COUNT) view->screen.topic_list.unit_index = -1;
            ensure_list_window(&view->screen.topic_list.cursor, filtered_topics(view->screen.topic_list.unit_index, (int [MAX_FILTERED_RESULTS]){0}), 8);
            break;
        case VIEW_TOPIC_DETAIL:
            view->screen.topic_detail.topic_index = clamp_index_or_zero(view->screen.topic_detail.topic_index, TOPIC_COUNT);
            view->screen.topic_detail.text.page = clamp_page_or_zero(view->screen.topic_detail.text.page, TOPIC_PAGE_COUNT);
            if(view->screen.topic_detail.text.scroll < 0) view->screen.topic_detail.text.scroll = 0;
            if(view->screen.topic_detail.text.x_offset < 0) view->screen.topic_detail.text.x_offset = 0;
            break;
        case VIEW_CONCEPT_LIST:
            ensure_list_window(&view->screen.concept_list.cursor, CONCEPT_COUNT, 8);
            break;
        case VIEW_CONCEPT_DETAIL:
            view->screen.concept_detail.concept_index = clamp_index_or_zero(view->screen.concept_detail.concept_index, CONCEPT_COUNT);
            view->screen.concept_detail.detail.page = clamp_page_or_zero(view->screen.concept_detail.detail.page, CONCEPT_PAGE_COUNT);
            if(view->screen.concept_detail.detail.scroll < 0) view->screen.concept_detail.detail.scroll = 0;
            if(view->screen.concept_detail.detail.x_offset < 0) view->screen.concept_detail.detail.x_offset = 0;
            break;
        case VIEW_VOCAB_MODE:
            ensure_list_window(&view->screen.vocab_mode.cursor, VOCAB_MODE_COUNT, 8);
            break;
        case VIEW_VOCAB_UNIT_LIST:
            ensure_list_window(&view->screen.vocab_unit_list.cursor, UNIT_COUNT, 9);
            break;
        case VIEW_VOCAB_CATEGORY_LIST:
            ensure_list_window(&view->screen.vocab_category_list.cursor, CATEGORY_COUNT, 9);
            break;
        case VIEW_VOCAB_LIST: {
            int count = 0;
            int active_filter = 0;
            if(view->screen.vocab_list.group_mode < 0 || view->screen.vocab_list.group_mode > 2) view->screen.vocab_list.group_mode = 1;
            if(view->screen.vocab_list.unit_filter_index < -1 || view->screen.vocab_list.unit_filter_index >= UNIT_COUNT) view->screen.vocab_list.unit_filter_index = -1;
            if(view->screen.vocab_list.category_filter_index < -1 || view->screen.vocab_list.category_filter_index >= CATEGORY_COUNT) view->screen.vocab_list.category_filter_index = -1;
            active_filter = (view->screen.vocab_list.group_mode == 0) ? view->screen.vocab_list.unit_filter_index : view->screen.vocab_list.category_filter_index;
            count = filtered_vocab(view->screen.vocab_list.group_mode, active_filter, (int [MAX_FILTERED_RESULTS]){0});
            ensure_list_window(&view->screen.vocab_list.cursor, count, 8);
            break;
        }
        case VIEW_VOCAB_DETAIL:
            view->screen.vocab_detail.vocab_index = clamp_index_or_zero(view->screen.vocab_detail.vocab_index, VOCAB_COUNT);
            view->screen.vocab_detail.detail.page = clamp_page_or_zero(view->screen.vocab_detail.detail.page, VOCAB_PAGE_COUNT);
            if(view->screen.vocab_detail.detail.scroll < 0) view->screen.vocab_detail.detail.scroll = 0;
            if(view->screen.vocab_detail.detail.x_offset < 0) view->screen.vocab_detail.detail.x_offset = 0;
            break;
        case VIEW_GRAPH_LIST:
            ensure_list_window(&view->screen.graph_list.cursor, GRAPH_COUNT, 8);
            break;
        case VIEW_GRAPH_DETAIL: {
            const GraphEntry *graph;
            int element_count;
            view->screen.graph_detail.graph_index = clamp_index_or_zero(view->screen.graph_detail.graph_index, GRAPH_COUNT);
            view->screen.graph_detail.detail.page = clamp_page_or_zero(view->screen.graph_detail.detail.page, GRAPH_PAGE_COUNT);
            if(view->screen.graph_detail.detail.scroll < 0) view->screen.graph_detail.detail.scroll = 0;
            if(view->screen.graph_detail.detail.x_offset < 0) view->screen.graph_detail.detail.x_offset = 0;
            graph = graph_entry_at(view->screen.graph_detail.graph_index);
            if(view->screen.graph_detail.mode_flags < 0) view->screen.graph_detail.mode_flags = graph_default_mode_flags(graph);
            view->screen.graph_detail.mode_flags = sanitize_graph_mode_flags(graph, view->screen.graph_detail.mode_flags);
            element_count = graphs_get_element_count(graph);
            view->screen.graph_detail.focus_index = clamp_index_or_zero(view->screen.graph_detail.focus_index, element_count);
            break;
        }
        case VIEW_FORMULA_LIST:
            ensure_list_window(&view->screen.formula_list.cursor, FORMULA_COUNT, 8);
            break;
        case VIEW_FORMULA_DETAIL:
            view->screen.formula_detail.formula_index = clamp_index_or_zero(view->screen.formula_detail.formula_index, FORMULA_COUNT);
            view->screen.formula_detail.text.page = clamp_page_or_zero(view->screen.formula_detail.text.page, FORMULA_PAGE_COUNT);
            if(view->screen.formula_detail.text.scroll < 0) view->screen.formula_detail.text.scroll = 0;
            if(view->screen.formula_detail.text.x_offset < 0) view->screen.formula_detail.text.x_offset = 0;
            break;
        case VIEW_STRUCTURE_LIST:
            ensure_list_window(&view->screen.structure_list.cursor, STRUCTURE_COUNT, 8);
            break;
        case VIEW_STRUCTURE_DETAIL:
            view->screen.structure_detail.structure_index = clamp_index_or_zero(view->screen.structure_detail.structure_index, STRUCTURE_COUNT);
            view->screen.structure_detail.text.page = clamp_page_or_zero(view->screen.structure_detail.text.page, STRUCTURE_PAGE_COUNT);
            if(view->screen.structure_detail.text.scroll < 0) view->screen.structure_detail.text.scroll = 0;
            if(view->screen.structure_detail.text.x_offset < 0) view->screen.structure_detail.text.x_offset = 0;
            break;
        case VIEW_REVISION_MENU:
            ensure_list_window(&view->screen.revision_menu.cursor, REVISION_ITEM_COUNT, 8);
            break;
        case VIEW_QUICK_LIST:
            ensure_list_window(&view->screen.quick_list.cursor, QUICK_REVIEW_COUNT, 9);
            break;
        case VIEW_QUICK_DETAIL:
            view->screen.quick_detail.entry_index = clamp_index_or_zero(view->screen.quick_detail.entry_index, QUICK_REVIEW_COUNT);
            if(view->screen.quick_detail.text.scroll < 0) view->screen.quick_detail.text.scroll = 0;
            if(view->screen.quick_detail.text.x_offset < 0) view->screen.quick_detail.text.x_offset = 0;
            break;
        case VIEW_EXAM_CRAM_LIST:
            ensure_list_window(&view->screen.exam_cram_list.cursor, EXAM_CRAM_COUNT, 9);
            break;
        case VIEW_EXAM_CRAM_DETAIL:
            view->screen.exam_cram_detail.entry_index = clamp_index_or_zero(view->screen.exam_cram_detail.entry_index, EXAM_CRAM_COUNT);
            if(view->screen.exam_cram_detail.text.scroll < 0) view->screen.exam_cram_detail.text.scroll = 0;
            if(view->screen.exam_cram_detail.text.x_offset < 0) view->screen.exam_cram_detail.text.x_offset = 0;
            break;
        case VIEW_REFERENCE_LIST:
            ensure_list_window(&view->screen.reference_list.cursor, REFERENCE_COUNT, 9);
            break;
        case VIEW_REFERENCE_DETAIL:
            view->screen.reference_detail.entry_index = clamp_index_or_zero(view->screen.reference_detail.entry_index, REFERENCE_COUNT);
            if(view->screen.reference_detail.text.scroll < 0) view->screen.reference_detail.text.scroll = 0;
            if(view->screen.reference_detail.text.x_offset < 0) view->screen.reference_detail.text.x_offset = 0;
            break;
        case VIEW_RECENT_LIST:
            ensure_list_window(&view->screen.recent_list.cursor, g_app.recent_count, 9);
            break;
        case VIEW_ABOUT:
            if(view->screen.about.text.scroll < 0) view->screen.about.text.scroll = 0;
            if(view->screen.about.text.x_offset < 0) view->screen.about.text.x_offset = 0;
            break;
        case VIEW_AUDIT:
            if(view->screen.audit.text.scroll < 0) view->screen.audit.text.scroll = 0;
            if(view->screen.audit.text.x_offset < 0) view->screen.audit.text.x_offset = 0;
            break;
        case VIEW_RESULT_DETAIL:
            if(view->screen.result_detail.text.scroll < 0) view->screen.result_detail.text.scroll = 0;
            if(view->screen.result_detail.text.x_offset < 0) view->screen.result_detail.text.x_offset = 0;
            break;
    }
}

static void scroll_text(TextScrollState *text, const char *body, int key)
{
    if(!text) return;
    ui_handle_text_view_input(body, &text->scroll, &text->x_offset, key);
}

static void scroll_paged_text(PagedTextState *detail, const char *body, int key)
{
    if(!detail) return;
    ui_handle_text_view_input(body, &detail->scroll, &detail->x_offset, key);
}

static void render_home(HomeScreenState *state)
{
    int visible = 8;
    int i = 0;
    char counts[128];
    ListCursorState *cursor = &state->cursor;
    ensure_list_window(cursor, HOME_COUNT, visible);
    ui_clear();
    ui_draw_header("AP Microeconomics", "Casio fx-CG50 native add-in");
    snprintf(counts, sizeof counts, "%d units | %d concepts | %d topics | %d terms | %d graphs", UNIT_COUNT, CONCEPT_COUNT, TOPIC_COUNT, VOCAB_COUNT, GRAPH_COUNT);
    ui_draw_home_card("Study + cram + graph lookup", counts);
    for(i = 0; i < visible && cursor->top + i < HOME_COUNT; i++) {
        int index = cursor->top + i;
        ui_draw_list_item(82 + i * 18, k_home_items[index].label, index == cursor->selected, "");
    }
    dtext(16, 208, COLOR_MUTED, "Cross-links, recent pages, graph locations, and exam-focused helpers");
    ui_draw_footer("EXE Open", "EXIT Quit");
    dupdate();
}

static void render_unit_cursor_list(ListCursorState *cursor, const char *title, const char *subtitle)
{
    int visible = 9;
    int i = 0;
    ensure_list_window(cursor, UNIT_COUNT, visible);
    ui_clear();
    ui_draw_header(title, subtitle);
    if(UNIT_COUNT <= 0) {
        dtext(18, 56, COLOR_TEXT, "No unit pages are available.");
    }
    for(i = 0; i < visible && cursor->top + i < UNIT_COUNT; i++) {
        int index = cursor->top + i;
        ui_draw_list_item(38 + i * 18, unit_title_at(index), index == cursor->selected, "");
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_units(UnitsScreenState *state, const char *title)
{
    render_unit_cursor_list(&state->cursor, title, "AP unit navigation");
}

static void render_vocab_unit_list(VocabUnitListScreenState *state)
{
    render_unit_cursor_list(&state->cursor, "Vocabulary By Unit", "Choose a unit filter");
}

static void render_revision_menu(RevisionMenuScreenState *state)
{
    render_simple_menu(&state->cursor, "Revision Hub", "Fast review, cram, reference, graphs, and formulas", k_revision_items, REVISION_ITEM_COUNT);
}

static void render_simple_menu(ListCursorState *cursor, const char *title, const char *subtitle, const MenuItem *items, int count)
{
    int visible = 8;
    int i = 0;
    char sub[52];
    ensure_list_window(cursor, count, visible);
    ui_clear();
    ui_draw_header(title, subtitle);
    if(!items || count <= 0) {
        dtext(18, 56, COLOR_TEXT, "No menu items are available.");
    }
    for(i = 0; i < visible && cursor->top + i < count; i++) {
        int index = cursor->top + i;
        ui_draw_list_item(56 + i * 20, items[index].label, index == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, items[index].subtitle);
        dtext(22, 68 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_text_sheet_list(ListCursorState *cursor, const char *title, const char *subtitle, const TextSheetEntry *items, int count)
{
    int visible = 9;
    int i = 0;
    ensure_list_window(cursor, count, visible);
    ui_clear();
    ui_draw_header(title, subtitle);
    if(!items || count <= 0) {
        dtext(18, 56, COLOR_TEXT, "No sheets are available in this section.");
    }
    for(i = 0; i < visible && cursor->top + i < count; i++) {
        int index = cursor->top + i;
        ui_draw_list_item(38 + i * 18, items[index].title, index == cursor->selected, "");
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void draw_wrapped_block(int x, int y, int width, int max_lines, uint16_t color, const char *text)
{
    WrappedText wrapped;
    int columns = width / 6;
    int i = 0;
    if(columns < 12) columns = 12;
    if(columns > MAX_LINE_LEN) columns = MAX_LINE_LEN;
    ui_wrap_text_for_width(text ? text : "", &wrapped, columns);
    for(i = 0; i < max_lines && i < wrapped.line_count; i++) {
        dtext(x, y + i * 12, color, wrapped.lines[i]);
    }
}

static const char *build_unit_body(int index, int page)
{
    const UnitEntry *entry = unit_entry_at(index);
    if(!entry) return build_unavailable_body("This unit overview is missing.");
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Overview", entry->body);
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Study actions", "EXE opens the topic list for this unit.\nF6 opens the vocabulary list for this unit.");
        append_section(g_page_body, sizeof g_page_body, "Section controls", "F1 and F2 move between unit sections. UP and DOWN scroll. LEFT and RIGHT pan sideways when a line is wider than the screen.");
    }
    return g_page_body;
}

static const char *build_topic_body(int index, int page)
{
    const TopicEntry *entry = topic_entry_at(index);
    char units[192];
    if(!entry) return build_unavailable_body("This topic page is missing.");
    unit_titles_from_csv(entry->unit_ids_csv, units, sizeof units);
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Overview", entry->body);
        append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(entry->primary_unit_id));
        append_section(g_page_body, sizeof g_page_body, "All units", units);
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Related graphs", entry->related_graphs);
        append_section(g_page_body, sizeof g_page_body, "Related formulas", entry->related_formulas);
        append_section(g_page_body, sizeof g_page_body, "Study actions", "EXE opens the first related graph.\nF6 opens the first related formula.");
    }
    return g_page_body;
}

static const char *build_vocab_body(int index, int page)
{
    const VocabularyEntry *entry = vocab_entry_at(index);
    bool has_graph_path = false;
    char units[192];
    char topics[256];
    if(!entry) return build_unavailable_body("This vocabulary page is missing.");
    has_graph_path = (entry->graph_id[0] || entry->related_graph_ids_csv[0] || entry->graph_name[0] || entry->related_graphs[0]) && (entry->graph_location_text[0] || entry->graph_where[0]);
    unit_titles_from_csv(entry->unit_ids_csv, units, sizeof units);
    topic_titles_from_csv(entry->topic_ids_csv, topics, sizeof topics);
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Quick definition", entry->short_definition);
        append_section(g_page_body, sizeof g_page_body, "Used for", entry->used_for);
        append_section(g_page_body, sizeof g_page_body, "Question types", entry->question_types);
        append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(entry->primary_unit_id));
        append_section(g_page_body, sizeof g_page_body, "All units", units);
        append_section(g_page_body, sizeof g_page_body, "Primary topic", topic_title_from_id(entry->primary_topic_id));
        append_section(g_page_body, sizeof g_page_body, "All topics", topics);
        append_section(g_page_body, sizeof g_page_body, "Category", entry->category);
        append_section(g_page_body, sizeof g_page_body, "Visual understanding", entry->visual_summary);
    }
    else if(page == 1) {
        append_section(g_page_body, sizeof g_page_body, "Full explanation", entry->long_definition);
        if(has_graph_path) {
            append_section(g_page_body, sizeof g_page_body, "Graph connection", entry->graph_name[0] ? entry->graph_name : entry->related_graphs);
            append_section(g_page_body, sizeof g_page_body, "Graph position type", entry->graph_element_type[0] ? entry->graph_element_type : entry->graph_kind);
            append_section(g_page_body, sizeof g_page_body, "Exact graph location", entry->graph_location_text[0] ? entry->graph_location_text : entry->graph_where);
            append_section(g_page_body, sizeof g_page_body, "Graph element id", entry->graph_element_id);
            append_section(g_page_body, sizeof g_page_body, "What that position means", entry->graph_meaning);
            append_section(g_page_body, sizeof g_page_body, "What changing it does", entry->graph_effect[0] ? entry->graph_effect : entry->example_graph_effect);
            if(entry->highlight_mode[0]) append_section(g_page_body, sizeof g_page_body, "Highlight mode", entry->highlight_mode);
            if(entry->real_example[0]) append_section(g_page_body, sizeof g_page_body, "Real-world example", entry->real_example);
        }
        else {
            append_section(g_page_body, sizeof g_page_body, "Real-world example", entry->real_example);
            append_section(g_page_body, sizeof g_page_body, "Graph effect", entry->example_graph_effect);
        }
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Related formulas", entry->related_formulas);
        append_section(g_page_body, sizeof g_page_body, "Related terms", entry->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Related graph curves", entry->related_curve_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Related graph points", entry->related_point_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Related graph regions", entry->related_region_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Common confusion", entry->confusion);
        append_section(g_page_body, sizeof g_page_body, "AP exam tip", entry->exam_tip);
        append_section(g_page_body, sizeof g_page_body, "Market structure", entry->market_structure);
        append_section(g_page_body, sizeof g_page_body, "Study actions", "EXE opens the related graph with this term highlighted. F5 opens the first related formula. F6 opens the first related term.");
    }
    return g_page_body;
}

static const char *build_concept_body(int index, int page)
{
    const ConceptEntry *entry = concept_entry_at(index);
    char units[192];
    char topics[256];
    if(!entry) return build_unavailable_body("This concept page is missing.");
    unit_titles_from_csv(entry->unit_ids_csv, units, sizeof units);
    topic_titles_from_csv(entry->topic_ids_csv, topics, sizeof topics);
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Short definition", entry->short_definition);
        append_section(g_page_body, sizeof g_page_body, "Why it matters", entry->why_it_matters);
        append_section(g_page_body, sizeof g_page_body, "Exam use", entry->exam_use);
        append_section(g_page_body, sizeof g_page_body, "Quick recall", entry->quick_recall);
        append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(entry->primary_unit_id));
        append_section(g_page_body, sizeof g_page_body, "All units", units);
        append_section(g_page_body, sizeof g_page_body, "Primary topic", topic_title_from_id(entry->primary_topic_id));
        append_section(g_page_body, sizeof g_page_body, "All topics", topics);
    }
    else if(page == 1) {
        append_section(g_page_body, sizeof g_page_body, "Full explanation", entry->full_explanation);
        append_section(g_page_body, sizeof g_page_body, "Real-world example", entry->real_world_example);
        append_section(g_page_body, sizeof g_page_body, "Graph connection", entry->graph_connection);
        append_section(g_page_body, sizeof g_page_body, "Exact graph location", entry->graph_location_text);
        append_section(g_page_body, sizeof g_page_body, "Graph position type", entry->graph_element_type);
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Graph element id", entry->graph_element_id);
        append_section(g_page_body, sizeof g_page_body, "Related graph curves", entry->related_curve_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Related graph points", entry->related_point_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Related graph regions", entry->related_region_ids_csv);
        append_section(g_page_body, sizeof g_page_body, "Related terms", entry->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Related formulas", entry->related_formulas);
        append_section(g_page_body, sizeof g_page_body, "Study actions", "EXE opens the linked graph. F5 opens the first related vocabulary term. F6 opens the first related formula.");
    }
    return g_page_body;
}

static const char *build_graph_text_page(int index, int page, int focus_index)
{
    const GraphEntry *graph = graph_entry_at(index);
    const GraphElementEntry *focus = graphs_get_element(graph, focus_index);
    char units[192];
    char topics[256];
    if(!graph) return build_unavailable_body("This graph page is missing.");
    unit_titles_from_csv(graph->unit_ids_csv, units, sizeof units);
    topic_titles_from_csv(graph->topic_ids_csv, topics, sizeof topics);
    g_page_body[0] = 0;
    if(page == 1) {
        append_section(g_page_body, sizeof g_page_body, "Overview", graph->overview_page);
        append_section(g_page_body, sizeof g_page_body, "Labels", graph->labels_page);
    }
    else if(page == 2) append_section(g_page_body, sizeof g_page_body, "", graph->shifts_page);
    else if(page == 3) append_section(g_page_body, sizeof g_page_body, "", graph->guide_page);
    else if(page == 4) append_section(g_page_body, sizeof g_page_body, "", graph->questions_page);
    else if(page == 5) append_section(g_page_body, sizeof g_page_body, "", graph->mistakes_page);
    if(focus) {
        append_section(g_page_body, sizeof g_page_body, "Current focus", focus->name);
        append_section(g_page_body, sizeof g_page_body, "Element type", focus->type);
        append_section(g_page_body, sizeof g_page_body, "Where it is", focus->location);
        append_section(g_page_body, sizeof g_page_body, "What it means", focus->meaning);
        append_section(g_page_body, sizeof g_page_body, "AP use", focus->used_for);
    }
    append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(graph->primary_unit_id));
    append_section(g_page_body, sizeof g_page_body, "All units", units);
    append_section(g_page_body, sizeof g_page_body, "Primary topic", topic_title_from_id(graph->primary_topic_id));
    append_section(g_page_body, sizeof g_page_body, "All topics", topics);
    append_section(g_page_body, sizeof g_page_body, "Related vocabulary", graph->related_terms);
    append_section(g_page_body, sizeof g_page_body, "Related formulas", graph->related_formulas);
    if(focus) {
        append_section(g_page_body, sizeof g_page_body, "Focus term links", focus->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Focus formula links", focus->related_formulas);
    }
    return g_page_body;
}

static const char *build_graph_visual_body(int index, int focus_index, int mode_flags)
{
    const GraphEntry *graph = graph_entry_at(index);
    const GraphElementEntry *focus = graphs_get_element(graph, focus_index);
    const char *view_group = graph_view_group_name(graph, focus, mode_flags);
    char units[192];
    char topics[256];
    char overlays[128];
    int slot = 0;

    if(!graph) return build_unavailable_body("This graph page is missing.");

    unit_titles_from_csv(graph->unit_ids_csv, units, sizeof units);
    topic_titles_from_csv(graph->topic_ids_csv, topics, sizeof topics);
    g_page_body[0] = 0;

    if(focus) {
        append_section(g_page_body, sizeof g_page_body, "Selected element", focus->name);
        append_section(g_page_body, sizeof g_page_body, "Element type", focus->type);
        append_section(g_page_body, sizeof g_page_body, "Where it is", focus->location);
        append_section(g_page_body, sizeof g_page_body, "What it means", focus->meaning);
        append_section(g_page_body, sizeof g_page_body, "AP use", focus->used_for);
        append_section(g_page_body, sizeof g_page_body, "Focus term links", focus->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Focus formula links", focus->related_formulas);
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Selected element", "No graph element is focused right now.");
        append_section(g_page_body, sizeof g_page_body, "How to study it", "Use F3 to cycle through curves, points, shaded regions, and graph concepts. The info panel below the graph scrolls with UP and DOWN and pans sideways with LEFT and RIGHT.");
    }

    overlays[0] = 0;
    append_hint_piece(overlays, sizeof overlays, view_group);
    for(slot = 0; slot < 3; slot++) {
        const char *label = graph_toggle_label(graph, slot);
        int flag = graph_toggle_flag(graph, slot);
        char piece[32];
        if(!label[0] || !flag) continue;
        snprintf(piece, sizeof piece, "%s:%s", label, (mode_flags & flag) ? "On" : "Off");
        append_hint_piece(overlays, sizeof overlays, piece);
    }
    append_section(g_page_body, sizeof g_page_body, "Current view", overlays);
    append_section(g_page_body, sizeof g_page_body, "Graph overview", graph->overview_page);
    append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(graph->primary_unit_id));
    append_section(g_page_body, sizeof g_page_body, "All units", units);
    append_section(g_page_body, sizeof g_page_body, "Primary topic", topic_title_from_id(graph->primary_topic_id));
    append_section(g_page_body, sizeof g_page_body, "All topics", topics);
    append_section(g_page_body, sizeof g_page_body, "Related vocabulary", graph->related_terms);
    append_section(g_page_body, sizeof g_page_body, "Related formulas", graph->related_formulas);
    append_section(g_page_body, sizeof g_page_body, "Controls", "UP and DOWN scroll this graph info panel. LEFT and RIGHT pan across long lines. F3 changes the focused graph element. F4 to F6 toggle graph overlays when the current graph supports them. EXE opens the linked term for the selected graph element.");
    return g_page_body;
}

static const char *build_formula_body(int index, int page)
{
    const FormulaEntry *entry = formula_entry_at(index);
    char units[192];
    char topics[256];
    if(!entry) return build_unavailable_body("This formula card is missing.");
    unit_titles_from_csv(entry->unit_ids_csv, units, sizeof units);
    topic_titles_from_csv(entry->topic_ids_csv, topics, sizeof topics);
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Formula", entry->formula);
        append_section(g_page_body, sizeof g_page_body, "Variables", entry->variables);
        append_section(g_page_body, sizeof g_page_body, "When to use it", entry->when_to_use);
        append_section(g_page_body, sizeof g_page_body, "How to interpret it", entry->interpretation);
        append_section(g_page_body, sizeof g_page_body, "Common trap", entry->trap);
        if(entry->helper[0]) append_section(g_page_body, sizeof g_page_body, "Calculator helper", "Press EXE to open the interactive helper.");
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Primary unit", unit_title_from_id(entry->primary_unit_id));
        append_section(g_page_body, sizeof g_page_body, "All units", units);
        append_section(g_page_body, sizeof g_page_body, "Primary topic", topic_title_from_id(entry->primary_topic_id));
        append_section(g_page_body, sizeof g_page_body, "All topics", topics);
        append_section(g_page_body, sizeof g_page_body, "Related terms", entry->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Related graphs", entry->related_graphs);
        append_section(g_page_body, sizeof g_page_body, "Full card", entry->body);
        append_section(g_page_body, sizeof g_page_body, "Study actions", entry->helper[0] ? "EXE opens the calculator helper. F6 opens the first related graph." : "F6 opens the first related graph.");
    }
    return g_page_body;
}

static const char *build_structure_body(int index, int page)
{
    const StructureEntry *entry = structure_entry_at(index);
    if(!entry) return build_unavailable_body("This market structure page is missing.");
    g_page_body[0] = 0;
    if(page <= 0) {
        append_section(g_page_body, sizeof g_page_body, "Snapshot", entry->summary);
        append_section(g_page_body, sizeof g_page_body, "Comparison", entry->body);
    }
    else {
        append_section(g_page_body, sizeof g_page_body, "Related graphs", entry->related_graphs);
        append_section(g_page_body, sizeof g_page_body, "Related terms", entry->related_terms);
        append_section(g_page_body, sizeof g_page_body, "Study actions", "EXE opens the first related graph.");
    }
    return g_page_body;
}

static void set_result_text(const char *title, const char *body)
{
    snprintf(g_result_title, sizeof g_result_title, "%s", title ? title : "Result");
    snprintf(g_result_body, sizeof g_result_body, "%s", body ? body : "");
    enter_result_detail_view();
}

static void render_topic_list(TopicListScreenState *state)
{
    int indices[MAX_FILTERED_RESULTS];
    int count = filtered_topics(state->unit_index, indices);
    int visible = 8;
    int i = 0;
    char sub[52];
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, count, visible);
    ui_clear();
    ui_draw_header("Topics", unit_entry_at(state->unit_index) ? unit_title_at(state->unit_index) : "All AP Micro topics");
    if(count <= 0) {
        dtext(18, 56, COLOR_TEXT, "No topics match this view.");
        dtext(18, 72, COLOR_MUTED, "Try a different unit or return to the main menu.");
    }
    for(i = 0; i < visible && cursor->top + i < count; i++) {
        const TopicEntry *topic = topic_entry_at(indices[cursor->top + i]);
        if(!topic) continue;
        ui_draw_list_item(44 + i * 20, safe_str(topic->title), cursor->top + i == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, unit_title_from_id(topic->primary_unit_id));
        dtext(22, 56 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_category_list(VocabCategoryListScreenState *state)
{
    int visible = 9;
    int i = 0;
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, CATEGORY_COUNT, visible);
    ui_clear();
    ui_draw_header("Vocabulary Categories", "Graph labels, welfare, labor, and more");
    for(i = 0; i < visible && cursor->top + i < CATEGORY_COUNT; i++) {
        int index = cursor->top + i;
        ui_draw_list_item(38 + i * 18, k_categories[index], index == cursor->selected, "");
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_concept_list(ConceptListScreenState *state)
{
    int visible = 8;
    int i = 0;
    char sub[60];
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, CONCEPT_COUNT, visible);
    ui_clear();
    ui_draw_header("Concepts", "Big AP ideas with graph previews");
    if(CONCEPT_COUNT <= 0) {
        dtext(18, 56, COLOR_TEXT, "No concepts are available.");
    }
    for(i = 0; i < visible && cursor->top + i < CONCEPT_COUNT; i++) {
        int index = cursor->top + i;
        const ConceptEntry *entry = concept_entry_at(index);
        if(!entry) continue;
        ui_draw_list_item(44 + i * 20, safe_str(entry->title), index == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, safe_str(entry->short_definition));
        dtext(22, 56 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_vocab_list(VocabListScreenState *state)
{
    int indices[MAX_FILTERED_RESULTS];
    int active_filter = (state->group_mode == 0) ? state->unit_filter_index : state->category_filter_index;
    int count = filtered_vocab(state->group_mode, active_filter, indices);
    int visible = 8;
    int i = 0;
    char subtitle[64];
    char sub[52];
    ListCursorState *cursor = &state->cursor;

    if(state->group_mode == 0 && unit_entry_at(state->unit_filter_index)) snprintf(subtitle, sizeof subtitle, "%s", unit_title_at(state->unit_filter_index));
    else if(state->group_mode == 1) snprintf(subtitle, sizeof subtitle, "Alphabetical master list");
    else if(state->group_mode == 2 && state->category_filter_index >= 0) snprintf(subtitle, sizeof subtitle, "%s", k_categories[state->category_filter_index]);
    else snprintf(subtitle, sizeof subtitle, "Vocabulary");

    ensure_list_window(cursor, count, visible);
    ui_clear();
    ui_draw_header("Vocabulary", subtitle);
    if(count <= 0) {
        dtext(18, 56, COLOR_TEXT, "No vocabulary terms match this filter.");
        dtext(18, 72, COLOR_MUTED, "Try another unit, category, or the A-Z list.");
    }
    for(i = 0; i < visible && cursor->top + i < count; i++) {
        const VocabularyEntry *entry = vocab_entry_at(indices[cursor->top + i]);
        if(!entry) continue;
        ui_draw_list_item(44 + i * 20, safe_str(entry->term), cursor->top + i == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, safe_str(entry->category));
        dtext(22, 56 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_vocab_visual_page(VocabDetailScreenState *state)
{
    const VocabularyEntry *entry = vocab_entry_at(state->vocab_index);
    int graph_index = -1;
    const GraphEntry *graph;
    int focus_index = 0;
    const GraphElementEntry *focus;
    GraphRenderOptions options = {
        false,
        false,
        false,
        false,
        false,
        false
    };
    char summary[900];

    if(!entry) {
        ui_draw_text_page("Vocabulary", "Visual", "This vocabulary page is unavailable.", &state->detail.scroll, &state->detail.x_offset, "EXE Graph", "F6 Term");
        return;
    }
    graph = resolve_vocab_graph(entry, &graph_index);
    focus = resolve_vocab_focus(entry, graph, &focus_index);
    options.show_labels = (graph != NULL);
    options.show_points = (graph != NULL);
    options.show_area = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "area") && focus && focus->area_capable;
    options.show_concepts = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "concept");
    options.show_shift = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "shift");

    summary[0] = 0;
    if(graph) {
        append_section(summary, sizeof summary, "Linked graph", graph->title);
        append_section(summary, sizeof summary, "Focused element", focus ? focus->name : entry->term);
        append_section(summary, sizeof summary, "Element type", entry->graph_element_type[0] ? entry->graph_element_type : (focus ? focus->type : ""));
        append_section(summary, sizeof summary, "Where it is", entry->graph_location_text[0] ? entry->graph_location_text : (entry->graph_where[0] ? entry->graph_where : (focus ? focus->location : "")));
        append_section(summary, sizeof summary, "What it means", entry->graph_meaning[0] ? entry->graph_meaning : (focus ? focus->meaning : ""));
        append_section(summary, sizeof summary, "What it does", entry->graph_effect[0] ? entry->graph_effect : (focus ? focus->used_for : entry->example_graph_effect));
        if(entry->graph_element_id[0]) append_section(summary, sizeof summary, "Element id", entry->graph_element_id);
        if(entry->highlight_mode[0]) append_section(summary, sizeof summary, "Highlight mode", entry->highlight_mode);
    }
    else {
        append_section(summary, sizeof summary, "No direct graph location", "This term is better remembered through its example and graph effect than through a single labeled graph element.");
        append_section(summary, sizeof summary, "Real-world example", entry->real_example);
        append_section(summary, sizeof summary, "Graph effect", entry->example_graph_effect);
    }

    ui_clear();
    ui_draw_header(entry->term, k_vocab_pages[clamp_page_or_zero(state->detail.page, VOCAB_PAGE_COUNT)]);
    if(graph) graphs_draw_preview(graph, focus, &options, 12, 34, 192, 118);
    else {
        ui_draw_panel(12, 34, 192, 118, COLOR_WHITE, COLOR_LINE);
        draw_wrapped_block(22, 54, 172, 6, COLOR_TEXT, "No direct graph highlight for this term.");
        draw_wrapped_block(22, 118, 172, 2, COLOR_MUTED, entry->visual_summary);
    }
    ui_draw_panel(212, 34, 172, 118, COLOR_PANEL, COLOR_LINE);
    draw_wrapped_block(220, 44, 156, 8, COLOR_TEXT, summary);
    ui_draw_panel(12, 160, 372, 42, COLOR_WHITE, COLOR_LINE);
    draw_wrapped_block(20, 170, 356, 2, COLOR_MUTED, graph ? "F1/F2 switch sections. EXE opens the full graph. F5 opens a related formula. F6 opens a related term." : "F1/F2 switch sections. F5 opens a related formula. F6 opens a related term.");
    ui_draw_status_bar(graph ? "Mini graph linked to vocab focus" : "Example-driven vocab cue", graph ? graph->title : "No direct graph");
    ui_draw_footer("F1<Sec  F2Sec>", graph ? "EXE Graph  F5 Formula" : "F5 Formula  F6 Term");
    dupdate();
}

static void render_graph_list(GraphListScreenState *state)
{
    int visible = 8;
    int i = 0;
    char sub[52];
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, GRAPH_COUNT, visible);
    ui_clear();
    ui_draw_header("Graphs", "Diagram, labels, shifts, guide, questions, mistakes");
    if(GRAPH_COUNT <= 0) {
        dtext(18, 56, COLOR_TEXT, "No graphs are available.");
    }
    for(i = 0; i < visible && cursor->top + i < GRAPH_COUNT; i++) {
        int index = cursor->top + i;
        const GraphEntry *entry = graph_entry_at(index);
        if(!entry) continue;
        ui_draw_list_item(44 + i * 20, safe_str(entry->title), index == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, unit_title_from_id(entry->primary_unit_id));
        dtext(22, 56 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_concept_visual_page(ConceptDetailScreenState *state)
{
    const ConceptEntry *entry = concept_entry_at(state->concept_index);
    int graph_index = -1;
    const GraphEntry *graph;
    int focus_index;
    const GraphElementEntry *focus;
    GraphRenderOptions options = {
        false,
        false,
        false,
        false,
        false,
        false
    };
    char summary[900];

    if(!entry) {
        ui_draw_text_page("Concept", "Visual", "This concept page is unavailable.", &state->detail.scroll, &state->detail.x_offset, "EXE Graph", "F6 Formula");
        return;
    }
    graph_index = find_graph_index_by_id(entry->graph_id);
    graph = graph_entry_at(graph_index);
    focus_index = resolve_graph_focus_index(
        graph,
        entry->graph_element_id,
        entry->related_point_ids_csv,
        entry->related_curve_ids_csv,
        entry->related_region_ids_csv,
        entry->graph_focus_term
    );
    focus = graph ? graphs_get_element(graph, focus_index) : NULL;
    options.show_labels = true;
    options.show_points = true;
    options.show_area = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "area") && focus && focus->area_capable;
    options.show_concepts = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "concept");
    options.show_shift = string_flag_enabled(entry->highlight_supported) && csv_contains_token(entry->highlight_mode, "shift");

    summary[0] = 0;
    append_section(summary, sizeof summary, "Quick recall", entry->quick_recall);
    append_section(summary, sizeof summary, "Graph link", entry->graph_connection);
    append_section(summary, sizeof summary, "Example", entry->real_world_example);
    if(focus) {
        append_section(summary, sizeof summary, "Focus", focus->name);
        append_section(summary, sizeof summary, "Element type", entry->graph_element_type[0] ? entry->graph_element_type : focus->type);
        append_section(summary, sizeof summary, "Where it is", entry->graph_location_text[0] ? entry->graph_location_text : focus->location);
    }
    if(entry->graph_element_id[0]) append_section(summary, sizeof summary, "Element id", entry->graph_element_id);
    if(entry->highlight_mode[0]) append_section(summary, sizeof summary, "Highlight mode", entry->highlight_mode);

    ui_clear();
    ui_draw_header(entry->title, k_concept_pages[clamp_page_or_zero(state->detail.page, CONCEPT_PAGE_COUNT)]);
    if(graph) graphs_draw_preview(graph, focus, &options, 12, 34, 192, 118);
    else {
        ui_draw_panel(12, 34, 192, 118, COLOR_WHITE, COLOR_LINE);
        dtext(24, 88, COLOR_TEXT, "No linked graph preview");
    }
    ui_draw_panel(212, 34, 172, 118, COLOR_PANEL, COLOR_LINE);
    draw_wrapped_block(220, 44, 156, 8, COLOR_TEXT, summary);
    ui_draw_panel(12, 160, 372, 42, COLOR_WHITE, COLOR_LINE);
    draw_wrapped_block(20, 170, 356, 2, COLOR_MUTED, "F1/F2 switch sections. EXE opens the full graph. F5 opens the first related term. F6 opens the first related formula.");
    ui_draw_status_bar("Concept mini graph", graph ? graph->title : "No graph");
    ui_draw_footer("F1<Sec  F2Sec>", "EXE Graph  F5 Term");
    dupdate();
}

static void render_vocab_detail(VocabDetailScreenState *state)
{
    const VocabularyEntry *entry = vocab_entry_at(state->vocab_index);
    if(state->detail.page < 0) state->detail.page = 0;
    if(state->detail.page >= VOCAB_PAGE_COUNT) state->detail.page = VOCAB_PAGE_COUNT - 1;
    if(!entry) {
        ui_draw_text_page("Vocabulary", "Unavailable", "This vocabulary page is not available.", &state->detail.scroll, &state->detail.x_offset, "", "");
        return;
    }
    if(state->detail.page == 2) {
        render_vocab_visual_page(state);
        return;
    }
    ui_draw_text_page(entry->term, k_vocab_pages[state->detail.page], build_vocab_body(state->vocab_index, state->detail.page), &state->detail.scroll, &state->detail.x_offset, "EXE Graph", "F5 Formula  F6 Term");
}

static void render_concept_detail(ConceptDetailScreenState *state)
{
    const ConceptEntry *entry = concept_entry_at(state->concept_index);
    if(state->detail.page < 0) state->detail.page = 0;
    if(state->detail.page >= CONCEPT_PAGE_COUNT) state->detail.page = CONCEPT_PAGE_COUNT - 1;
    if(!entry) {
        ui_draw_text_page("Concept", "Unavailable", "This concept page is not available.", &state->detail.scroll, &state->detail.x_offset, "", "");
        return;
    }
    if(state->detail.page == 2) {
        render_concept_visual_page(state);
        return;
    }
    ui_draw_text_page(entry->title, k_concept_pages[state->detail.page], build_concept_body(state->concept_index, state->detail.page), &state->detail.scroll, &state->detail.x_offset, "EXE Graph", "F5 Term  F6 Formula");
}

static void render_formula_list(FormulaListScreenState *state)
{
    int visible = 8;
    int i = 0;
    char sub[52];
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, FORMULA_COUNT, visible);
    ui_clear();
    ui_draw_header("Formulas", "Cards, variables, traps, and calculators");
    if(FORMULA_COUNT <= 0) {
        dtext(18, 56, COLOR_TEXT, "No formulas are available.");
    }
    for(i = 0; i < visible && cursor->top + i < FORMULA_COUNT; i++) {
        int index = cursor->top + i;
        const FormulaEntry *entry = formula_entry_at(index);
        if(!entry) continue;
        ui_draw_list_item(44 + i * 20, safe_str(entry->title), index == cursor->selected, entry->helper[0] ? "Calc" : "");
        ui_trimmed_copy(sub, sizeof sub, topic_title_from_id(entry->primary_topic_id));
        dtext(22, 56 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_structure_list(StructureListScreenState *state)
{
    int visible = 8;
    int i = 0;
    char sub[56];
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, STRUCTURE_COUNT, visible);
    ui_clear();
    ui_draw_header("Market Structures", "Compare pricing power, entry, efficiency, and output rules");
    if(STRUCTURE_COUNT <= 0) {
        dtext(18, 56, COLOR_TEXT, "No market structure pages are available.");
    }
    for(i = 0; i < visible && cursor->top + i < STRUCTURE_COUNT; i++) {
        int index = cursor->top + i;
        const StructureEntry *entry = structure_entry_at(index);
        if(!entry) continue;
        ui_draw_list_item(52 + i * 20, safe_str(entry->title), index == cursor->selected, "");
        ui_trimmed_copy(sub, sizeof sub, safe_str(entry->summary));
        dtext(22, 64 + i * 20, COLOR_MUTED, sub);
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_recent_list(RecentListScreenState *state)
{
    int visible = 9;
    int i = 0;
    ListCursorState *cursor = &state->cursor;

    ensure_list_window(cursor, g_app.recent_count, visible);
    ui_clear();
    ui_draw_header("Recent Pages", "Jump back to recently opened study pages");
    if(g_app.recent_count == 0) {
        dtext(18, 56, COLOR_TEXT, "No recent pages yet.");
        dtext(18, 72, COLOR_MUTED, "Open a unit, term, graph, or formula detail page.");
    }
    else {
        for(i = 0; i < visible && cursor->top + i < g_app.recent_count; i++) {
            int index = cursor->top + i;
            ui_draw_list_item(40 + i * 18, g_app.recent[index].title, index == cursor->selected, "");
        }
    }
    ui_draw_footer("EXE Open", "EXIT Back");
    dupdate();
}

static void render_graph_detail(GraphDetailScreenState *state)
{
    const GraphEntry *graph = graph_entry_at(state->graph_index);
    int element_count = graphs_get_element_count(graph);
    const GraphElementEntry *focus = NULL;
    const char *view_group = "Base";
    const char *toggle1 = "";
    const char *toggle2 = "";
    const char *toggle3 = "";
    GraphRenderOptions options;
    GraphRenderOptions graph_options;
    char status_left[64];
    char status_right[28];
    char footer_left[44];
    char footer_right[44];
    const char *graph_body = NULL;

    if(state->detail.page < 0) state->detail.page = 0;
    if(state->detail.page >= GRAPH_PAGE_COUNT) state->detail.page = GRAPH_PAGE_COUNT - 1;
    if(state->mode_flags < 0) state->mode_flags = graph_default_mode_flags(graph);
    if(state->focus_index < 0) state->focus_index = 0;
    if(element_count > 0 && state->focus_index >= element_count) state->focus_index = element_count - 1;
    state->mode_flags = sanitize_graph_mode_flags(graph, state->mode_flags);
    focus = graphs_get_element(graph, state->focus_index);
    options = graph_options_from_flags(state->mode_flags);
    graph_options = options;
    graph_options.show_info = false;
    view_group = graph_view_group_name(graph, focus, state->mode_flags);

    if(!graph) {
        ui_draw_text_page("Graph", "Unavailable", "This graph page is not available.", &state->detail.scroll, &state->detail.x_offset, "", "");
        return;
    }

    if(state->detail.page == 0) {
        toggle1 = graph_toggle_label(graph, 0);
        toggle2 = graph_toggle_label(graph, 1);
        toggle3 = graph_toggle_label(graph, 2);
        ui_clear();
        ui_draw_header(graph->title, k_graph_pages[state->detail.page]);
        status_left[0] = 0;
        append_hint_piece(status_left, sizeof status_left, "F1/F2 sections");
        append_hint_piece(status_left, sizeof status_left, "F3 focus");
        if(toggle1[0]) {
            char piece[20];
            snprintf(piece, sizeof piece, "F4 %s", toggle1);
            append_hint_piece(status_left, sizeof status_left, piece);
        }
        if(toggle2[0]) {
            char piece[20];
            snprintf(piece, sizeof piece, "F5 %s", toggle2);
            append_hint_piece(status_left, sizeof status_left, piece);
        }
        if(toggle3[0]) {
            char piece[20];
            snprintf(piece, sizeof piece, "F6 %s", toggle3);
            append_hint_piece(status_left, sizeof status_left, piece);
        }
        append_hint_piece(status_left, sizeof status_left, "Arrows read");
        snprintf(status_right, sizeof status_right, "%s view", view_group);
        graph_body = build_graph_visual_body(state->graph_index, state->focus_index, state->mode_flags);
        snprintf(footer_left, sizeof footer_left, "F1<Sec  F2Sec>");
        snprintf(footer_right, sizeof footer_right, toggle1[0] || toggle2[0] || toggle3[0] ? "EXE Term  F3/F4-6" : "EXE Term  F3 Focus");
        graphs_draw_diagram(graph, focus, &graph_options);
        ui_draw_text_viewport(12, 142, SCREEN_W - 24, 56, focus ? focus->name : "Graph focus", graph_body, &state->detail.scroll, &state->detail.x_offset);
        ui_draw_status_bar(status_left, status_right);
        ui_draw_footer(footer_left, footer_right);
        dupdate();
        return;
    }

    ui_draw_text_page(graph->title, k_graph_pages[state->detail.page], build_graph_text_page(state->graph_index, state->detail.page, state->focus_index), &state->detail.scroll, &state->detail.x_offset, "EXE Term", "F6 Formula");
}

static void render_current_view(void)
{
    ViewState *view = current_view();
    const TextSheetEntry *audit = audit_entry_at(0);

    sanitize_view_state(view);

    switch(view->type) {
        case VIEW_HOME: render_home(&view->screen.home); break;
        case VIEW_UNITS: render_units(&view->screen.units, "Units"); break;
        case VIEW_UNIT_DETAIL: ui_draw_text_page(unit_title_at(view->screen.unit_detail.unit_index), k_unit_pages[clamp_page_or_zero(view->screen.unit_detail.text.page, UNIT_PAGE_COUNT)], build_unit_body(view->screen.unit_detail.unit_index, view->screen.unit_detail.text.page), &view->screen.unit_detail.text.scroll, &view->screen.unit_detail.text.x_offset, "EXE Topics", "F6 Terms"); break;
        case VIEW_TOPIC_LIST: render_topic_list(&view->screen.topic_list); break;
        case VIEW_TOPIC_DETAIL: ui_draw_text_page(topic_title_at(view->screen.topic_detail.topic_index), k_topic_pages[clamp_page_or_zero(view->screen.topic_detail.text.page, TOPIC_PAGE_COUNT)], build_topic_body(view->screen.topic_detail.topic_index, view->screen.topic_detail.text.page), &view->screen.topic_detail.text.scroll, &view->screen.topic_detail.text.x_offset, "EXE Graph", "F6 Formula"); break;
        case VIEW_CONCEPT_LIST: render_concept_list(&view->screen.concept_list); break;
        case VIEW_CONCEPT_DETAIL: render_concept_detail(&view->screen.concept_detail); break;
        case VIEW_VOCAB_MODE: render_simple_menu(&view->screen.vocab_mode.cursor, "Vocabulary", "Choose a lookup path", k_vocab_modes, VOCAB_MODE_COUNT); break;
        case VIEW_VOCAB_UNIT_LIST: render_vocab_unit_list(&view->screen.vocab_unit_list); break;
        case VIEW_VOCAB_CATEGORY_LIST: render_category_list(&view->screen.vocab_category_list); break;
        case VIEW_VOCAB_LIST: render_vocab_list(&view->screen.vocab_list); break;
        case VIEW_VOCAB_DETAIL: render_vocab_detail(&view->screen.vocab_detail); break;
        case VIEW_GRAPH_LIST: render_graph_list(&view->screen.graph_list); break;
        case VIEW_GRAPH_DETAIL: render_graph_detail(&view->screen.graph_detail); break;
        case VIEW_FORMULA_LIST: render_formula_list(&view->screen.formula_list); break;
        case VIEW_FORMULA_DETAIL: ui_draw_text_page(formula_title_at(view->screen.formula_detail.formula_index), k_formula_pages[clamp_page_or_zero(view->screen.formula_detail.text.page, FORMULA_PAGE_COUNT)], build_formula_body(view->screen.formula_detail.formula_index, view->screen.formula_detail.text.page), &view->screen.formula_detail.text.scroll, &view->screen.formula_detail.text.x_offset, (formula_entry_at(view->screen.formula_detail.formula_index) && formula_entry_at(view->screen.formula_detail.formula_index)->helper[0]) ? "EXE Calc" : "", "F6 Graph"); break;
        case VIEW_STRUCTURE_LIST: render_structure_list(&view->screen.structure_list); break;
        case VIEW_STRUCTURE_DETAIL: ui_draw_text_page(structure_title_at(view->screen.structure_detail.structure_index), k_structure_pages[clamp_page_or_zero(view->screen.structure_detail.text.page, STRUCTURE_PAGE_COUNT)], build_structure_body(view->screen.structure_detail.structure_index, view->screen.structure_detail.text.page), &view->screen.structure_detail.text.scroll, &view->screen.structure_detail.text.x_offset, "EXE Graph", ""); break;
        case VIEW_REVISION_MENU: render_revision_menu(&view->screen.revision_menu); break;
        case VIEW_QUICK_LIST: render_text_sheet_list(&view->screen.quick_list.cursor, "Quick Review", "Compact AP Micro refreshers", g_quick_review, QUICK_REVIEW_COUNT); break;
        case VIEW_QUICK_DETAIL: ui_draw_text_page(sheet_title_or(quick_entry_at(view->screen.quick_detail.entry_index), "Quick Review"), "Quick review sheet", sheet_body_or(quick_entry_at(view->screen.quick_detail.entry_index)), &view->screen.quick_detail.text.scroll, &view->screen.quick_detail.text.x_offset, "", ""); break;
        case VIEW_EXAM_CRAM_LIST: render_text_sheet_list(&view->screen.exam_cram_list.cursor, "Exam Cram", "Fast high-yield pages", g_exam_cram, EXAM_CRAM_COUNT); break;
        case VIEW_EXAM_CRAM_DETAIL: ui_draw_text_page(sheet_title_or(exam_cram_entry_at(view->screen.exam_cram_detail.entry_index), "Exam Cram"), "High-yield cram page", sheet_body_or(exam_cram_entry_at(view->screen.exam_cram_detail.entry_index)), &view->screen.exam_cram_detail.text.scroll, &view->screen.exam_cram_detail.text.x_offset, "", ""); break;
        case VIEW_REFERENCE_LIST: render_text_sheet_list(&view->screen.reference_list.cursor, "Reference", "Shifts, policy, and rules", g_reference, REFERENCE_COUNT); break;
        case VIEW_REFERENCE_DETAIL: ui_draw_text_page(sheet_title_or(reference_entry_at(view->screen.reference_detail.entry_index), "Reference"), "Cheat sheet", sheet_body_or(reference_entry_at(view->screen.reference_detail.entry_index)), &view->screen.reference_detail.text.scroll, &view->screen.reference_detail.text.x_offset, "", ""); break;
        case VIEW_RECENT_LIST: render_recent_list(&view->screen.recent_list); break;
        case VIEW_ABOUT: ui_draw_text_page("About / Help", "Native fx-CG50 AP Micro add-in", k_about_text, &view->screen.about.text.scroll, &view->screen.about.text.x_offset, "EXE Audit", ""); break;
        case VIEW_AUDIT: ui_draw_text_page(sheet_title_or(audit, "Source Audit"), "Desktop source coverage", sheet_body_or(audit), &view->screen.audit.text.scroll, &view->screen.audit.text.x_offset, "", ""); break;
        case VIEW_RESULT_DETAIL: ui_draw_text_page(g_result_title, "Calculator result", g_result_body, &view->screen.result_detail.text.scroll, &view->screen.result_detail.text.x_offset, "", ""); break;
    }
}

static bool prompt_number(const char *title, const char *label, double *out)
{
    return ui_prompt_number(title, label, out);
}

static bool require_nonnegative(double value, const char *label)
{
    char body[196];
    if(isfinite(value) && value >= 0.0) return true;
    snprintf(body, sizeof body, "%s must be a finite non-negative value.", label);
    set_result_text("Input Error", body);
    return false;
}

static bool require_positive(double value, const char *label)
{
    char body[196];
    if(isfinite(value) && value > 0.0) return true;
    snprintf(body, sizeof body, "%s must be a finite value greater than zero.", label);
    set_result_text("Input Error", body);
    return false;
}

static bool require_finite_result(double value, const char *label)
{
    char body[196];
    if(isfinite(value)) return true;
    snprintf(body, sizeof body, "%s overflowed or became invalid. Try smaller values.", label);
    set_result_text("Calculation Error", body);
    return false;
}

static void run_formula_helper(const FormulaEntry *formula)
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
    double result = 0.0;
    char body[1200];

    body[0] = 0;
    if(!formula || !formula->helper[0]) {
        set_result_text("Helper Unavailable", "No interactive helper is configured for this formula.");
        return;
    }

    if(strcmp(formula->helper, "ped") == 0 || strcmp(formula->helper, "pes") == 0 || strcmp(formula->helper, "midpoint") == 0) {
        bool demand = strcmp(formula->helper, "ped") == 0;
        bool supply = strcmp(formula->helper, "pes") == 0;

        if(!prompt_number(formula->title, "Start quantity", &a)) return;
        if(!prompt_number(formula->title, "End quantity", &b)) return;
        if(!prompt_number(formula->title, "Start price", &c)) return;
        if(!prompt_number(formula->title, "End price", &d)) return;
        if(!require_nonnegative(a, "Start quantity") || !require_nonnegative(b, "End quantity") ||
            !require_nonnegative(c, "Start price") || !require_nonnegative(d, "End price")) return;
        if(((a + b) / 2.0) == 0.0 || ((c + d) / 2.0) == 0.0 || (d - c) == 0.0) {
            set_result_text("Elasticity Error", "Invalid inputs. Midpoint averages and the price change must not produce a zero denominator.");
            return;
        }

        result = ((b - a) / ((a + b) / 2.0)) / ((d - c) / ((c + d) / 2.0));
        if(!require_finite_result(result, "Elasticity")) return;
        snprintf(body, sizeof body,
            "Formula used\n%s\n\n"
            "Result\nElasticity = %.4f\nAbsolute value = %.4f\n\n"
            "Interpretation\n%s",
            formula->formula, result, fabs(result),
            demand ? ((fabs(result) > 1.0) ? "Demand is elastic." : ((fabs(result) < 1.0) ? "Demand is inelastic." : "Demand is unit elastic.")) :
            (supply ? ((fabs(result) > 1.0) ? "Supply is elastic." : ((fabs(result) < 1.0) ? "Supply is inelastic." : "Supply is unit elastic.")) :
            "Use the absolute value for classification on most AP questions."));
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "cross") == 0) {
        if(!prompt_number(formula->title, "Start qty of good A", &a)) return;
        if(!prompt_number(formula->title, "End qty of good A", &b)) return;
        if(!prompt_number(formula->title, "Start price of good B", &c)) return;
        if(!prompt_number(formula->title, "End price of good B", &d)) return;
        if(!require_nonnegative(a, "Start quantity of good A") || !require_nonnegative(b, "End quantity of good A") ||
            !require_nonnegative(c, "Start price of good B") || !require_nonnegative(d, "End price of good B")) return;
        if(((a + b) / 2.0) == 0.0 || ((c + d) / 2.0) == 0.0 || (d - c) == 0.0) {
            set_result_text("Cross Elasticity Error", "Invalid inputs. Midpoint averages and the price change must not produce a zero denominator.");
            return;
        }
        result = ((b - a) / ((a + b) / 2.0)) / ((d - c) / ((c + d) / 2.0));
        if(!require_finite_result(result, "Cross-price elasticity")) return;
        snprintf(body, sizeof body,
            "Formula used\n%s\n\n"
            "Result\nXED = %.4f\n\n"
            "Interpretation\n%s",
            formula->formula, result,
            (result > 0.0) ? "Positive result suggests substitutes." : ((result < 0.0) ? "Negative result suggests complements." : "A near-zero result suggests a weak relation."));
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "income") == 0) {
        if(!prompt_number(formula->title, "Start quantity", &a)) return;
        if(!prompt_number(formula->title, "End quantity", &b)) return;
        if(!prompt_number(formula->title, "Start income", &c)) return;
        if(!prompt_number(formula->title, "End income", &d)) return;
        if(!require_nonnegative(a, "Start quantity") || !require_nonnegative(b, "End quantity") ||
            !require_nonnegative(c, "Start income") || !require_nonnegative(d, "End income")) return;
        if(((a + b) / 2.0) == 0.0 || ((c + d) / 2.0) == 0.0 || (d - c) == 0.0) {
            set_result_text("Income Elasticity Error", "Invalid inputs. Midpoint averages and the income change must not produce a zero denominator.");
            return;
        }
        result = ((b - a) / ((a + b) / 2.0)) / ((d - c) / ((c + d) / 2.0));
        if(!require_finite_result(result, "Income elasticity")) return;
        snprintf(body, sizeof body,
            "Formula used\n%s\n\n"
            "Result\nYED = %.4f\n\n"
            "Interpretation\n%s",
            formula->formula, result,
            (result >= 0.0) ? "Positive result suggests a normal good." : "Negative result suggests an inferior good.");
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "total_revenue") == 0) {
        if(!prompt_number(formula->title, "Price", &a)) return;
        if(!prompt_number(formula->title, "Quantity", &b)) return;
        if(!require_nonnegative(a, "Price") || !require_nonnegative(b, "Quantity")) return;
        result = a * b;
        if(!require_finite_result(result, "Total revenue")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nResult\nTR = %.4f", formula->formula, result);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "marginal_revenue") == 0 || strcmp(formula->helper, "marginal_cost") == 0) {
        if(!prompt_number(formula->title, strcmp(formula->helper, "marginal_revenue") == 0 ? "Initial total revenue" : "Initial total cost", &a)) return;
        if(!prompt_number(formula->title, strcmp(formula->helper, "marginal_revenue") == 0 ? "New total revenue" : "New total cost", &b)) return;
        if(!prompt_number(formula->title, "Initial quantity", &c)) return;
        if(!prompt_number(formula->title, "New quantity", &d)) return;
        if(!require_nonnegative(a, "Initial total value") || !require_nonnegative(b, "New total value") ||
            !require_nonnegative(c, "Initial quantity") || !require_nonnegative(d, "New quantity")) return;
        if((d - c) == 0.0) {
            set_result_text("Change Error", "Quantity change cannot be zero.");
            return;
        }
        result = (b - a) / (d - c);
        if(!require_finite_result(result, strcmp(formula->helper, "marginal_revenue") == 0 ? "Marginal revenue" : "Marginal cost")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nResult\n%s = %.4f", formula->formula, strcmp(formula->helper, "marginal_revenue") == 0 ? "MR" : "MC", result);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "total_cost") == 0) {
        if(!prompt_number(formula->title, "Fixed cost", &a)) return;
        if(!prompt_number(formula->title, "Variable cost", &b)) return;
        if(!require_nonnegative(a, "Fixed cost") || !require_nonnegative(b, "Variable cost")) return;
        result = a + b;
        if(!require_finite_result(result, "Total cost")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nResult\nTC = %.4f", formula->formula, result);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "profit") == 0) {
        double tr = 0.0;
        double tc = 0.0;
        if(!prompt_number(formula->title, "Price", &a)) return;
        if(!prompt_number(formula->title, "Quantity", &b)) return;
        if(!prompt_number(formula->title, "Average total cost", &c)) return;
        if(!require_nonnegative(a, "Price") || !require_nonnegative(b, "Quantity") || !require_nonnegative(c, "Average total cost")) return;
        tr = a * b;
        tc = c * b;
        result = tr - tc;
        if(!require_finite_result(tr, "Total revenue") || !require_finite_result(tc, "Total cost") || !require_finite_result(result, "Profit")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nTR = %.4f\nTC = %.4f\nProfit = %.4f", formula->formula, tr, tc, result);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "accounting_economic_profit") == 0) {
        if(!prompt_number(formula->title, "Total revenue", &a)) return;
        if(!prompt_number(formula->title, "Explicit costs", &b)) return;
        if(!prompt_number(formula->title, "Implicit costs", &c)) return;
        if(!require_nonnegative(a, "Total revenue") || !require_nonnegative(b, "Explicit costs") || !require_nonnegative(c, "Implicit costs")) return;
        snprintf(body, sizeof body,
            "Formula used\n%s\n\nAccounting profit = %.4f\nEconomic profit = %.4f",
            formula->formula, a - b, a - b - c);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "average_costs") == 0) {
        double afc = 0.0;
        double avc = 0.0;
        double atc = 0.0;
        if(!prompt_number(formula->title, "Fixed cost", &a)) return;
        if(!prompt_number(formula->title, "Variable cost", &b)) return;
        if(!prompt_number(formula->title, "Quantity", &c)) return;
        if(!require_nonnegative(a, "Fixed cost") || !require_nonnegative(b, "Variable cost")) return;
        if(!require_positive(c, "Quantity")) return;
        afc = a / c;
        avc = b / c;
        atc = (a + b) / c;
        if(!require_finite_result(afc, "AFC") || !require_finite_result(avc, "AVC") || !require_finite_result(atc, "ATC")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nAFC = %.4f\nAVC = %.4f\nATC = %.4f", formula->formula, afc, avc, atc);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "tax_revenue") == 0 || strcmp(formula->helper, "mrp") == 0) {
        if(!prompt_number(formula->title, strcmp(formula->helper, "tax_revenue") == 0 ? "Tax per unit" : "Marginal product", &a)) return;
        if(!prompt_number(formula->title, strcmp(formula->helper, "tax_revenue") == 0 ? "Quantity after tax" : "Marginal revenue", &b)) return;
        if(!require_nonnegative(a, strcmp(formula->helper, "tax_revenue") == 0 ? "Tax per unit" : "Marginal product") ||
            !require_nonnegative(b, strcmp(formula->helper, "tax_revenue") == 0 ? "Quantity after tax" : "Marginal revenue")) return;
        result = a * b;
        if(!require_finite_result(result, formula->title)) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nResult\n%s = %.4f", formula->formula, strcmp(formula->helper, "tax_revenue") == 0 ? "Tax revenue" : "MRP", result);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "hiring_rule") == 0) {
        const char *decision = "Labor is already at the profit-maximizing level.";
        if(!prompt_number(formula->title, "Marginal revenue product", &a)) return;
        if(!prompt_number(formula->title, "Marginal resource cost", &b)) return;
        if(!require_nonnegative(a, "Marginal revenue product") || !require_nonnegative(b, "Marginal resource cost")) return;
        result = a - b;
        if(!require_finite_result(result, "Hiring gap")) return;
        if(result > 0.000001) decision = "Hire more labor because MRP is greater than MRC.";
        else if(result < -0.000001) decision = "Hire less labor because MRP is below MRC.";
        snprintf(body, sizeof body,
            "Formula used\n%s\n\n"
            "MRP = %.4f\nMRC = %.4f\nGap = %.4f\n\n"
            "Decision\n%s\n\n"
            "AP reminder\nIn a competitive labor market, wage = MRC.",
            formula->formula, a, b, result, decision);
        set_result_text(formula->title, body);
        return;
    }

    if(strcmp(formula->helper, "dwl") == 0) {
        if(!prompt_number(formula->title, "Base", &a)) return;
        if(!prompt_number(formula->title, "Height", &b)) return;
        if(!require_nonnegative(a, "Base") || !require_nonnegative(b, "Height")) return;
        result = 0.5 * a * b;
        if(!require_finite_result(result, "Deadweight loss")) return;
        snprintf(body, sizeof body, "Formula used\n%s\n\nDWL = %.4f", formula->formula, result);
        set_result_text(formula->title, body);
        return;
    }

    set_result_text("Helper Unavailable", "No interactive helper is configured for this formula.");
}

static void handle_text_detail_key(TextScrollState *text, int key, const char *body)
{
    if(key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT) scroll_text(text, body, key);
}

static void handle_paged_detail_scroll(PagedTextState *detail, int key, const char *body)
{
    if(key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT) scroll_paged_text(detail, body, key);
}

static void handle_graph_visual_scroll(PagedTextState *detail, int key, const char *body)
{
    if(key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT) {
        ui_handle_text_view_input_for_size(body, &detail->scroll, &detail->x_offset, key, SCREEN_W - 24, 44);
    }
}

static void handle_key(int key)
{
    ViewState *view = current_view();
    if(!view) return;
    sanitize_view_state(view);

    if(key == KEY_EXIT) {
        if(view->type == VIEW_HOME) {
            g_app.depth = 0;
            return;
        }
        pop_view();
        return;
    }

    switch(view->type) {
        case VIEW_HOME: {
            HomeScreenState *state = &view->screen.home;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) {
                switch(state->cursor.selected) {
                    case 0: enter_units_view(); break;
                    case 1: enter_concept_list_view(); break;
                    case 2: enter_vocab_mode_view(); break;
                    case 3: enter_graph_list_view(); break;
                    case 4: enter_formula_list_view(); break;
                    case 5: enter_structure_list_view(); break;
                    case 6: enter_revision_menu_view(); break;
                    case 7: enter_recent_list_view(); break;
                    case 8: enter_about_view(); break;
                }
            }
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= HOME_COUNT) state->cursor.selected = HOME_COUNT - 1;
            break;
        }

        case VIEW_UNITS: {
            UnitsScreenState *state = &view->screen.units;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_unit_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= UNIT_COUNT) state->cursor.selected = UNIT_COUNT - 1;
            break;
        }

        case VIEW_UNIT_DETAIL: {
            UnitDetailScreenState *state = &view->screen.unit_detail;
            const UnitEntry *entry = unit_entry_at(state->unit_index);
            if(key == KEY_F1) {
                if(change_text_section(&state->text, UNIT_PAGE_COUNT, -1)) {
                    add_recent(VIEW_UNIT_DETAIL, state->unit_index, state->text.page, 0, 0, unit_title_at(state->unit_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_text_section(&state->text, UNIT_PAGE_COUNT, 1)) {
                    add_recent(VIEW_UNIT_DETAIL, state->unit_index, state->text.page, 0, 0, unit_title_at(state->unit_index));
                }
            }
            else if(key == KEY_EXE) enter_topic_list_view(state->unit_index);
            else if(key == KEY_F6) enter_vocab_list_view(0, state->unit_index, -1);
            else handle_text_detail_key(&state->text, key, entry ? build_unit_body(state->unit_index, state->text.page) : "This unit overview is unavailable.");
            break;
        }

        case VIEW_TOPIC_LIST: {
            TopicListScreenState *state = &view->screen.topic_list;
            int indices[MAX_FILTERED_RESULTS];
            int count = filtered_topics(state->unit_index, indices);
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE && count > 0) open_topic_detail(indices[state->cursor.selected]);
            if(count <= 0) state->cursor.selected = 0;
            else {
                if(state->cursor.selected < 0) state->cursor.selected = 0;
                if(state->cursor.selected >= count) state->cursor.selected = count - 1;
            }
            break;
        }

        case VIEW_TOPIC_DETAIL: {
            TopicDetailScreenState *state = &view->screen.topic_detail;
            const TopicEntry *entry = topic_entry_at(state->topic_index);
            if(key == KEY_F1) {
                if(change_text_section(&state->text, TOPIC_PAGE_COUNT, -1)) {
                    add_recent(VIEW_TOPIC_DETAIL, state->topic_index, state->text.page, 0, 0, topic_title_at(state->topic_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_text_section(&state->text, TOPIC_PAGE_COUNT, 1)) {
                    add_recent(VIEW_TOPIC_DETAIL, state->topic_index, state->text.page, 0, 0, topic_title_at(state->topic_index));
                }
            }
            else if(key == KEY_EXE) open_first_graph(entry ? entry->related_graphs : NULL, false);
            else if(key == KEY_F6) open_first_formula(entry ? entry->related_formulas : NULL);
            else handle_text_detail_key(&state->text, key, entry ? build_topic_body(state->topic_index, state->text.page) : "This topic page is unavailable.");
            break;
        }

        case VIEW_CONCEPT_LIST: {
            ConceptListScreenState *state = &view->screen.concept_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_concept_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= CONCEPT_COUNT) state->cursor.selected = CONCEPT_COUNT - 1;
            break;
        }

        case VIEW_CONCEPT_DETAIL: {
            ConceptDetailScreenState *state = &view->screen.concept_detail;
            const ConceptEntry *entry = concept_entry_at(state->concept_index);
            if(key == KEY_F1) {
                if(change_paged_section(&state->detail, CONCEPT_PAGE_COUNT, -1)) {
                    add_recent(VIEW_CONCEPT_DETAIL, state->concept_index, state->detail.page, 0, 0, concept_title_at(state->concept_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_paged_section(&state->detail, CONCEPT_PAGE_COUNT, 1)) {
                    add_recent(VIEW_CONCEPT_DETAIL, state->concept_index, state->detail.page, 0, 0, concept_title_at(state->concept_index));
                }
            }
            else if(key == KEY_EXE) {
                open_first_graph_with_focus(
                    (entry && entry->related_graph_ids_csv[0]) ? entry->related_graph_ids_csv : (entry ? entry->graph_id : NULL),
                    true,
                    entry ? entry->graph_element_id : NULL,
                    entry ? entry->related_point_ids_csv : NULL,
                    entry ? entry->related_curve_ids_csv : NULL,
                    entry ? entry->related_region_ids_csv : NULL,
                    entry ? entry->graph_focus_term : NULL,
                    entry ? entry->highlight_mode : NULL
                );
            }
            else if(key == KEY_F5) {
                open_first_vocab(entry ? entry->related_terms : NULL);
            }
            else if(key == KEY_F6) {
                open_first_formula(entry ? entry->related_formulas : NULL);
            }
            else if(state->detail.page != 2) handle_paged_detail_scroll(&state->detail, key, entry ? build_concept_body(state->concept_index, state->detail.page) : "This concept page is unavailable.");
            break;
        }

        case VIEW_VOCAB_MODE: {
            VocabModeScreenState *state = &view->screen.vocab_mode;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) {
                if(state->cursor.selected == 0) enter_vocab_unit_list_view();
                if(state->cursor.selected == 1) enter_vocab_list_view(1, -1, -1);
                if(state->cursor.selected == 2) enter_vocab_category_list_view();
            }
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= VOCAB_MODE_COUNT) state->cursor.selected = VOCAB_MODE_COUNT - 1;
            break;
        }

        case VIEW_VOCAB_UNIT_LIST: {
            VocabUnitListScreenState *state = &view->screen.vocab_unit_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) enter_vocab_list_view(0, state->cursor.selected, -1);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= UNIT_COUNT) state->cursor.selected = UNIT_COUNT - 1;
            break;
        }

        case VIEW_VOCAB_CATEGORY_LIST: {
            VocabCategoryListScreenState *state = &view->screen.vocab_category_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) enter_vocab_list_view(2, -1, state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= CATEGORY_COUNT) state->cursor.selected = CATEGORY_COUNT - 1;
            break;
        }

        case VIEW_VOCAB_LIST: {
            VocabListScreenState *state = &view->screen.vocab_list;
            int indices[MAX_FILTERED_RESULTS];
            int active_filter = (state->group_mode == 0) ? state->unit_filter_index : state->category_filter_index;
            int count = filtered_vocab(state->group_mode, active_filter, indices);
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE && count > 0) open_vocab_detail(indices[state->cursor.selected]);
            if(count <= 0) state->cursor.selected = 0;
            else {
                if(state->cursor.selected < 0) state->cursor.selected = 0;
                if(state->cursor.selected >= count) state->cursor.selected = count - 1;
            }
            break;
        }

        case VIEW_VOCAB_DETAIL: {
            VocabDetailScreenState *state = &view->screen.vocab_detail;
            const VocabularyEntry *entry = vocab_entry_at(state->vocab_index);
            if(key == KEY_F1) {
                if(change_paged_section(&state->detail, VOCAB_PAGE_COUNT, -1)) {
                    add_recent(VIEW_VOCAB_DETAIL, state->vocab_index, state->detail.page, 0, 0, vocab_term_at(state->vocab_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_paged_section(&state->detail, VOCAB_PAGE_COUNT, 1)) {
                    add_recent(VIEW_VOCAB_DETAIL, state->vocab_index, state->detail.page, 0, 0, vocab_term_at(state->vocab_index));
                }
            }
            else if(key == KEY_EXE) open_first_graph_with_focus(
                (entry && entry->related_graph_ids_csv[0]) ? entry->related_graph_ids_csv : ((entry && entry->graph_id[0]) ? entry->graph_id : ((entry && entry->graph_name[0]) ? entry->graph_name : (entry ? entry->related_graphs : NULL))),
                true,
                entry ? entry->graph_element_id : NULL,
                entry ? entry->related_point_ids_csv : NULL,
                entry ? entry->related_curve_ids_csv : NULL,
                entry ? entry->related_region_ids_csv : NULL,
                entry ? entry->term : NULL,
                entry ? entry->highlight_mode : NULL
            );
            else if(key == KEY_F5) open_first_formula(entry ? entry->related_formulas : NULL);
            else if(key == KEY_F6) open_first_vocab(entry ? entry->related_terms : NULL);
            else if(state->detail.page != 2) handle_paged_detail_scroll(&state->detail, key, entry ? build_vocab_body(state->vocab_index, state->detail.page) : "This vocabulary page is unavailable.");
            break;
        }

        case VIEW_GRAPH_LIST: {
            GraphListScreenState *state = &view->screen.graph_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_graph_detail(state->cursor.selected, 0);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= GRAPH_COUNT) state->cursor.selected = GRAPH_COUNT - 1;
            break;
        }

        case VIEW_GRAPH_DETAIL: {
            GraphDetailScreenState *state = &view->screen.graph_detail;
            const GraphEntry *graph = graph_entry_at(state->graph_index);
            const GraphElementEntry *focus = graph ? graphs_get_element(graph, state->focus_index) : NULL;
            int toggle_flag = 0;
            if(key == KEY_F1) {
                if(change_paged_section(&state->detail, GRAPH_PAGE_COUNT, -1)) {
                    add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_paged_section(&state->detail, GRAPH_PAGE_COUNT, 1)) {
                    add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
                }
            }
            else if(state->detail.page == 0 && key == KEY_F3) {
                int count = graph ? graphs_get_element_count(graph) : 0;
                if(count > 0) {
                    const GraphElementEntry *next_focus;
                    state->focus_index = (state->focus_index + 1) % count;
                    next_focus = graphs_get_element(graph, state->focus_index);
                    state->mode_flags = graph_focus_overlay_flags(graph, next_focus, NULL);
                    reset_paged_viewport(&state->detail);
                }
                add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
            }
            else if(state->detail.page == 0 && key == KEY_F4) {
                toggle_flag = graph_toggle_flag(graph, 0);
                if(toggle_flag) state->mode_flags ^= toggle_flag;
                state->mode_flags = sanitize_graph_mode_flags(graph, state->mode_flags);
                add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
            }
            else if(state->detail.page == 0 && key == KEY_F5) {
                toggle_flag = graph_toggle_flag(graph, 1);
                if(toggle_flag) state->mode_flags ^= toggle_flag;
                state->mode_flags = sanitize_graph_mode_flags(graph, state->mode_flags);
                add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
            }
            else if(state->detail.page == 0 && key == KEY_F6) {
                toggle_flag = graph_toggle_flag(graph, 2);
                if(toggle_flag) state->mode_flags ^= toggle_flag;
                state->mode_flags = sanitize_graph_mode_flags(graph, state->mode_flags);
                add_recent(VIEW_GRAPH_DETAIL, state->graph_index, state->detail.page, state->focus_index, state->mode_flags, graph_title_at(state->graph_index));
            }
            else if(state->detail.page == 0 && key == KEY_EXE) {
                open_graph_focus_term(graph, focus);
            }
            else if(state->detail.page == 0) {
                handle_graph_visual_scroll(&state->detail, key, build_graph_visual_body(state->graph_index, state->focus_index, state->mode_flags));
            }
            else if(state->detail.page > 0 && key == KEY_EXE) open_graph_focus_term(graph, focus);
            else if(state->detail.page > 0 && key == KEY_F6) open_graph_focus_formula(graph, focus);
            else if(state->detail.page > 0) handle_paged_detail_scroll(&state->detail, key, build_graph_text_page(state->graph_index, state->detail.page, state->focus_index));
            break;
        }

        case VIEW_FORMULA_LIST: {
            FormulaListScreenState *state = &view->screen.formula_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_formula_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= FORMULA_COUNT) state->cursor.selected = FORMULA_COUNT - 1;
            break;
        }

        case VIEW_FORMULA_DETAIL: {
            FormulaDetailScreenState *state = &view->screen.formula_detail;
            const FormulaEntry *entry = formula_entry_at(state->formula_index);
            if(key == KEY_F1) {
                if(change_text_section(&state->text, FORMULA_PAGE_COUNT, -1)) {
                    add_recent(VIEW_FORMULA_DETAIL, state->formula_index, state->text.page, 0, 0, formula_title_at(state->formula_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_text_section(&state->text, FORMULA_PAGE_COUNT, 1)) {
                    add_recent(VIEW_FORMULA_DETAIL, state->formula_index, state->text.page, 0, 0, formula_title_at(state->formula_index));
                }
            }
            else if(key == KEY_EXE && entry && entry->helper[0]) run_formula_helper(entry);
            else if(key == KEY_F6) open_first_graph(entry ? entry->related_graphs : NULL, false);
            else handle_text_detail_key(&state->text, key, build_formula_body(state->formula_index, state->text.page));
            break;
        }

        case VIEW_STRUCTURE_LIST: {
            StructureListScreenState *state = &view->screen.structure_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_structure_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= STRUCTURE_COUNT) state->cursor.selected = STRUCTURE_COUNT - 1;
            break;
        }

        case VIEW_STRUCTURE_DETAIL: {
            StructureDetailScreenState *state = &view->screen.structure_detail;
            const StructureEntry *entry = structure_entry_at(state->structure_index);
            if(key == KEY_F1) {
                if(change_text_section(&state->text, STRUCTURE_PAGE_COUNT, -1)) {
                    add_recent(VIEW_STRUCTURE_DETAIL, state->structure_index, state->text.page, 0, 0, structure_title_at(state->structure_index));
                }
            }
            else if(key == KEY_F2) {
                if(change_text_section(&state->text, STRUCTURE_PAGE_COUNT, 1)) {
                    add_recent(VIEW_STRUCTURE_DETAIL, state->structure_index, state->text.page, 0, 0, structure_title_at(state->structure_index));
                }
            }
            else if(key == KEY_EXE) open_first_graph(entry ? entry->related_graphs : NULL, true);
            else handle_text_detail_key(&state->text, key, build_structure_body(state->structure_index, state->text.page));
            break;
        }

        case VIEW_REVISION_MENU: {
            RevisionMenuScreenState *state = &view->screen.revision_menu;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) {
                switch(state->cursor.selected) {
                    case 0: enter_quick_list_view(); break;
                    case 1: enter_exam_cram_list_view(); break;
                    case 2: enter_reference_list_view(); break;
                    case 3: enter_structure_list_view(); break;
                    case 4: enter_graph_list_view(); break;
                    case 5: enter_formula_list_view(); break;
                }
            }
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= REVISION_ITEM_COUNT) state->cursor.selected = REVISION_ITEM_COUNT - 1;
            break;
        }

        case VIEW_QUICK_LIST: {
            QuickListScreenState *state = &view->screen.quick_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_quick_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= QUICK_REVIEW_COUNT) state->cursor.selected = QUICK_REVIEW_COUNT - 1;
            break;
        }

        case VIEW_QUICK_DETAIL:
            handle_text_detail_key(&view->screen.quick_detail.text, key, sheet_body_or(quick_entry_at(view->screen.quick_detail.entry_index)));
            break;

        case VIEW_EXAM_CRAM_LIST: {
            ExamCramListScreenState *state = &view->screen.exam_cram_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_exam_cram_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= EXAM_CRAM_COUNT) state->cursor.selected = EXAM_CRAM_COUNT - 1;
            break;
        }

        case VIEW_EXAM_CRAM_DETAIL:
            handle_text_detail_key(&view->screen.exam_cram_detail.text, key, sheet_body_or(exam_cram_entry_at(view->screen.exam_cram_detail.entry_index)));
            break;

        case VIEW_REFERENCE_LIST: {
            ReferenceListScreenState *state = &view->screen.reference_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE) open_reference_detail(state->cursor.selected);
            if(state->cursor.selected < 0) state->cursor.selected = 0;
            if(state->cursor.selected >= REFERENCE_COUNT) state->cursor.selected = REFERENCE_COUNT - 1;
            break;
        }

        case VIEW_REFERENCE_DETAIL:
            handle_text_detail_key(&view->screen.reference_detail.text, key, sheet_body_or(reference_entry_at(view->screen.reference_detail.entry_index)));
            break;

        case VIEW_RECENT_LIST: {
            RecentListScreenState *state = &view->screen.recent_list;
            if(key == KEY_UP) state->cursor.selected--;
            else if(key == KEY_DOWN) state->cursor.selected++;
            else if(key == KEY_EXE && g_app.recent_count > 0) open_recent_entry(&g_app.recent[state->cursor.selected]);
            if(g_app.recent_count <= 0) state->cursor.selected = 0;
            else {
                if(state->cursor.selected < 0) state->cursor.selected = 0;
                if(state->cursor.selected >= g_app.recent_count) state->cursor.selected = g_app.recent_count - 1;
            }
            break;
        }

        case VIEW_ABOUT:
            if(key == KEY_EXE) enter_audit_view();
            else handle_text_detail_key(&view->screen.about.text, key, k_about_text);
            break;

        case VIEW_AUDIT:
            handle_text_detail_key(&view->screen.audit.text, key, sheet_body_or(audit_entry_at(0)));
            break;

        case VIEW_RESULT_DETAIL:
            handle_text_detail_key(&view->screen.result_detail.text, key, g_result_body);
            break;
    }
}

int main(void)
{
    key_event_t event;

    enter_home_view();

    while(g_app.depth > 0) {
        render_current_view();
        event = getkey();
        handle_key(event.key);
    }

    return 1;
}
