#ifndef APMICRO_APP_H
#define APMICRO_APP_H

#include <stdbool.h>
#include <stdint.h>

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>

#include "generated/apmicro_content.h"
#include "graph_model.h"

#ifndef KEY_CTRL_EXE
#define KEY_CTRL_EXE KEY_EXE
#endif
#ifndef KEY_CTRL_EXIT
#define KEY_CTRL_EXIT KEY_EXIT
#endif
#ifndef KEY_CTRL_DEL
#define KEY_CTRL_DEL KEY_DEL
#endif
#ifndef KEY_CTRL_F1
#define KEY_CTRL_F1 KEY_F1
#endif
#ifndef KEY_CTRL_F2
#define KEY_CTRL_F2 KEY_F2
#endif
#ifndef KEY_CTRL_F3
#define KEY_CTRL_F3 KEY_F3
#endif
#ifndef KEY_CTRL_F4
#define KEY_CTRL_F4 KEY_F4
#endif
#ifndef KEY_CTRL_F5
#define KEY_CTRL_F5 KEY_F5
#endif
#ifndef KEY_CTRL_F6
#define KEY_CTRL_F6 KEY_F6
#endif
#ifndef KEY_CTRL_LEFT
#define KEY_CTRL_LEFT KEY_LEFT
#endif
#ifndef KEY_CTRL_RIGHT
#define KEY_CTRL_RIGHT KEY_RIGHT
#endif
#ifndef KEY_CTRL_UP
#define KEY_CTRL_UP KEY_UP
#endif
#ifndef KEY_CTRL_DOWN
#define KEY_CTRL_DOWN KEY_DOWN
#endif

#define SCREEN_W 396
#define SCREEN_H 224

#define RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

#define COLOR_BG RGB565(245, 247, 250)
#define COLOR_PANEL RGB565(228, 236, 243)
#define COLOR_HEADER RGB565(17, 58, 92)
#define COLOR_HEADER_TEXT RGB565(255, 255, 255)
#define COLOR_ACCENT RGB565(30, 116, 179)
#define COLOR_ACCENT_SOFT RGB565(197, 224, 244)
#define COLOR_TEXT RGB565(22, 33, 43)
#define COLOR_MUTED RGB565(87, 100, 113)
#define COLOR_HIGHLIGHT RGB565(27, 102, 157)
#define COLOR_HIGHLIGHT_TEXT RGB565(255, 255, 255)
#define COLOR_LINE RGB565(158, 177, 194)
#define COLOR_WARN RGB565(197, 87, 41)
#define COLOR_GOOD RGB565(40, 134, 86)
#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_WHITE RGB565(255, 255, 255)

#define MAX_LINE_LEN 96
#define MAX_WRAPPED_LINES 1024
#define MAX_FILTERED_RESULTS 256
#define MAX_INPUT_LEN 31
#define STACK_DEPTH 20

typedef enum {
    VIEW_HOME = 0,
    VIEW_UNITS,
    VIEW_UNIT_DETAIL,
    VIEW_TOPIC_LIST,
    VIEW_TOPIC_DETAIL,
    VIEW_CONCEPT_LIST,
    VIEW_CONCEPT_DETAIL,
    VIEW_VOCAB_MODE,
    VIEW_VOCAB_UNIT_LIST,
    VIEW_VOCAB_CATEGORY_LIST,
    VIEW_VOCAB_LIST,
    VIEW_VOCAB_DETAIL,
    VIEW_GRAPH_LIST,
    VIEW_GRAPH_DETAIL,
    VIEW_FORMULA_LIST,
    VIEW_FORMULA_DETAIL,
    VIEW_STRUCTURE_LIST,
    VIEW_STRUCTURE_DETAIL,
    VIEW_REVISION_MENU,
    VIEW_QUICK_LIST,
    VIEW_QUICK_DETAIL,
    VIEW_EXAM_CRAM_LIST,
    VIEW_EXAM_CRAM_DETAIL,
    VIEW_REFERENCE_LIST,
    VIEW_REFERENCE_DETAIL,
    VIEW_RECENT_LIST,
    VIEW_ABOUT,
    VIEW_AUDIT,
    VIEW_RESULT_DETAIL
} ViewType;

typedef struct {
    int selected;
    int top;
} ListCursorState;

typedef struct {
    int page;
    int scroll;
    int x_offset;
} TextScrollState;

typedef struct {
    int page;
    int scroll;
    int x_offset;
} PagedTextState;

typedef struct {
    ListCursorState cursor;
} HomeScreenState;

typedef struct {
    ListCursorState cursor;
} UnitsScreenState;

typedef struct {
    int unit_index;
    TextScrollState text;
} UnitDetailScreenState;

typedef struct {
    int unit_index;
    ListCursorState cursor;
} TopicListScreenState;

typedef struct {
    int topic_index;
    TextScrollState text;
} TopicDetailScreenState;

typedef struct {
    ListCursorState cursor;
} ConceptListScreenState;

typedef struct {
    int concept_index;
    PagedTextState detail;
} ConceptDetailScreenState;

typedef struct {
    ListCursorState cursor;
} VocabModeScreenState;

typedef struct {
    ListCursorState cursor;
} VocabUnitListScreenState;

typedef struct {
    ListCursorState cursor;
} VocabCategoryListScreenState;

typedef struct {
    int group_mode;
    int unit_filter_index;
    int category_filter_index;
    ListCursorState cursor;
} VocabListScreenState;

typedef struct {
    int vocab_index;
    PagedTextState detail;
} VocabDetailScreenState;

typedef struct {
    ListCursorState cursor;
} GraphListScreenState;

typedef struct {
    int graph_index;
    int focus_index;
    int mode_flags;
    PagedTextState detail;
} GraphDetailScreenState;

typedef struct {
    ListCursorState cursor;
} FormulaListScreenState;

typedef struct {
    int formula_index;
    TextScrollState text;
} FormulaDetailScreenState;

typedef struct {
    ListCursorState cursor;
} StructureListScreenState;

typedef struct {
    int structure_index;
    TextScrollState text;
} StructureDetailScreenState;

typedef struct {
    ListCursorState cursor;
} RevisionMenuScreenState;

typedef struct {
    ListCursorState cursor;
} QuickListScreenState;

typedef struct {
    int entry_index;
    TextScrollState text;
} QuickDetailScreenState;

typedef struct {
    ListCursorState cursor;
} ExamCramListScreenState;

typedef struct {
    int entry_index;
    TextScrollState text;
} ExamCramDetailScreenState;

typedef struct {
    ListCursorState cursor;
} ReferenceListScreenState;

typedef struct {
    int entry_index;
    TextScrollState text;
} ReferenceDetailScreenState;

typedef struct {
    ListCursorState cursor;
} RecentListScreenState;

typedef struct {
    TextScrollState text;
} AboutScreenState;

typedef struct {
    TextScrollState text;
} AuditScreenState;

typedef struct {
    TextScrollState text;
} ResultDetailScreenState;

typedef struct {
    ViewType type;
    union {
        HomeScreenState home;
        UnitsScreenState units;
        UnitDetailScreenState unit_detail;
        TopicListScreenState topic_list;
        TopicDetailScreenState topic_detail;
        ConceptListScreenState concept_list;
        ConceptDetailScreenState concept_detail;
        VocabModeScreenState vocab_mode;
        VocabUnitListScreenState vocab_unit_list;
        VocabCategoryListScreenState vocab_category_list;
        VocabListScreenState vocab_list;
        VocabDetailScreenState vocab_detail;
        GraphListScreenState graph_list;
        GraphDetailScreenState graph_detail;
        FormulaListScreenState formula_list;
        FormulaDetailScreenState formula_detail;
        StructureListScreenState structure_list;
        StructureDetailScreenState structure_detail;
        RevisionMenuScreenState revision_menu;
        QuickListScreenState quick_list;
        QuickDetailScreenState quick_detail;
        ExamCramListScreenState exam_cram_list;
        ExamCramDetailScreenState exam_cram_detail;
        ReferenceListScreenState reference_list;
        ReferenceDetailScreenState reference_detail;
        RecentListScreenState recent_list;
        AboutScreenState about;
        AuditScreenState audit;
        ResultDetailScreenState result_detail;
    } screen;
} ViewState;

typedef struct {
    int line_count;
    char lines[MAX_WRAPPED_LINES][MAX_LINE_LEN + 1];
} WrappedText;

void ui_clear(void);
void ui_draw_header(const char *title, const char *subtitle);
void ui_draw_footer(const char *left_hint, const char *right_hint);
void ui_draw_panel(int x, int y, int w, int h, uint16_t fill, uint16_t border);
void ui_draw_list_item(int y, const char *text, bool selected, const char *suffix);
void ui_draw_home_card(const char *headline, const char *details);
void ui_wrap_text(const char *body, WrappedText *wrapped);
void ui_wrap_text_for_width(const char *body, WrappedText *wrapped, int columns);
int ui_visible_text_lines(void);
void ui_clamp_text_view(const char *body, int *scroll, int *x_offset);
void ui_clamp_text_view_for_size(const char *body, int *scroll, int *x_offset, int width, int height);
void ui_handle_text_view_input(const char *body, int *scroll, int *x_offset, int key);
void ui_handle_text_view_input_for_size(const char *body, int *scroll, int *x_offset, int key, int width, int height);
void ui_draw_text_page(const char *title, const char *section, const char *body, int *scroll, int *x_offset, const char *extra_left_hint, const char *extra_right_hint);
void ui_draw_text_viewport(int x, int y, int w, int h, const char *section, const char *body, int *scroll, int *x_offset);
void ui_draw_status_bar(const char *left, const char *right);
void ui_trimmed_copy(char *dst, int size, const char *src);
bool ui_prompt_number(const char *title, const char *label, double *out_value);

void graphs_draw_diagram(const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options);
void graphs_draw_preview(const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options, int x, int y, int w, int h);

#endif
