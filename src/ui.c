#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int k_header_h = 18;
static const int k_section_h = 12;
static const int k_footer_h = 16;
static const int k_status_h = 12;
static const int k_content_x = 12;
static const int k_content_y = 38;
static const int k_content_w = SCREEN_W - 24;
static const int k_content_h = SCREEN_H - k_content_y - k_status_h - k_footer_h - 4;
static const int k_content_pad_x = 6;
static const int k_content_pad_y = 5;
static const int k_char_w = 6;
static const int k_line_h = 10;
static const int k_pan_step = 4;
static WrappedText g_text_layout;

static void append_with_limit(char *dst, size_t size, const char *src)
{
    size_t len = 0;
    size_t remaining = (len < size) ? (size - len - 1) : 0;
    if(!dst || size == 0 || !src) return;
    len = strlen(dst);
    remaining = (len < size) ? (size - len - 1) : 0;
    if(remaining == 0) return;
    strncat(dst, src, remaining);
}

void ui_clear(void)
{
    dclear(COLOR_BG);
}

void ui_draw_panel(int x, int y, int w, int h, uint16_t fill, uint16_t border)
{
    int x1 = x;
    int y1 = y;
    int x2 = x + w;
    int y2 = y + h;
    if(w <= 0 || h <= 0) return;
    if(x2 < 0 || y2 < 0 || x1 >= SCREEN_W || y1 >= SCREEN_H) return;
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= SCREEN_W) x2 = SCREEN_W - 1;
    if(y2 >= SCREEN_H) y2 = SCREEN_H - 1;
    if(x2 <= x1 || y2 <= y1) return;
    drect(x1, y1, x2, y2, fill);
    dline(x1, y1, x2, y1, border);
    dline(x1, y2, x2, y2, border);
    dline(x1, y1, x1, y2, border);
    dline(x2, y1, x2, y2, border);
}

void ui_trimmed_copy(char *dst, int size, const char *src)
{
    int length = 0;
    if(!dst) return;
    if(size <= 0) return;
    if(!src) {
        dst[0] = 0;
        return;
    }

    while(src[length] && length < size - 1) {
        dst[length] = src[length];
        length++;
    }
    dst[length] = 0;

    if(src[length] && size > 4) {
        dst[size - 4] = '.';
        dst[size - 3] = '.';
        dst[size - 2] = '.';
        dst[size - 1] = 0;
    }
}

void ui_draw_header(const char *title, const char *subtitle)
{
    char title_line[40];
    char sub_line[32];
    ui_trimmed_copy(title_line, sizeof title_line, title);
    ui_trimmed_copy(sub_line, sizeof sub_line, subtitle ? subtitle : "");

    drect(0, 0, SCREEN_W - 1, k_header_h, COLOR_HEADER);
    dtext(8, 3, COLOR_HEADER_TEXT, title_line);
    if(sub_line[0]) {
        dtext(286, 3, COLOR_ACCENT_SOFT, sub_line);
    }
}

void ui_draw_footer(const char *left_hint, const char *right_hint)
{
    char left[44];
    char right[44];
    int y = SCREEN_H - k_footer_h;

    ui_trimmed_copy(left, sizeof left, left_hint ? left_hint : "");
    ui_trimmed_copy(right, sizeof right, right_hint ? right_hint : "");

    drect(0, y, SCREEN_W - 1, SCREEN_H - 1, COLOR_PANEL);
    dline(0, y, SCREEN_W - 1, y, COLOR_LINE);
    dtext(8, y + 3, COLOR_TEXT, left);
    dtext(236, y + 3, COLOR_MUTED, right);
}

void ui_draw_status_bar(const char *left, const char *right)
{
    char left_line[68];
    char right_line[24];
    int y = SCREEN_H - k_footer_h - k_status_h + 1;
    ui_trimmed_copy(left_line, sizeof left_line, left ? left : "");
    ui_trimmed_copy(right_line, sizeof right_line, right ? right : "");
    dtext(10, y, COLOR_MUTED, left_line);
    dtext(310, y, COLOR_MUTED, right_line);
}

void ui_draw_list_item(int y, const char *text, bool selected, const char *suffix)
{
    char label[54];
    char tail[16];
    ui_trimmed_copy(label, sizeof label, text);
    ui_trimmed_copy(tail, sizeof tail, suffix ? suffix : "");

    if(selected) {
        drect(10, y - 2, SCREEN_W - 10, y + 13, COLOR_HIGHLIGHT);
        dtext(16, y, COLOR_HIGHLIGHT_TEXT, label);
        if(tail[0]) dtext(332, y, COLOR_HIGHLIGHT_TEXT, tail);
    }
    else {
        dtext(16, y, COLOR_TEXT, label);
        if(tail[0]) dtext(332, y, COLOR_MUTED, tail);
    }
}

void ui_draw_home_card(const char *headline, const char *details)
{
    ui_draw_panel(12, 28, SCREEN_W - 24, 42, COLOR_PANEL, COLOR_LINE);
    dtext(20, 36, COLOR_TEXT, headline);
    dtext(20, 50, COLOR_MUTED, details);
}

static void wrap_line(const char *line, WrappedText *wrapped, int limit)
{
    char current[MAX_LINE_LEN + 1];
    const char *cursor = line;
    if(!wrapped) return;
    if(limit < 12) limit = 12;
    if(limit > MAX_LINE_LEN) limit = MAX_LINE_LEN;

    if(!line || !line[0]) {
        if(wrapped->line_count < MAX_WRAPPED_LINES) {
            wrapped->lines[wrapped->line_count][0] = 0;
            wrapped->line_count++;
        }
        return;
    }

    current[0] = 0;
    while(*cursor && wrapped->line_count < MAX_WRAPPED_LINES) {
        char token[MAX_LINE_LEN + 1];
        int token_len = 0;

        while(*cursor == ' ') cursor++;
        if(!*cursor) break;
        while(*cursor && *cursor != ' ' && token_len < MAX_LINE_LEN) {
            token[token_len++] = *cursor++;
        }
        token[token_len] = 0;

        if((int)strlen(current) == 0) {
            ui_trimmed_copy(current, sizeof current, token);
        }
        else if((int)strlen(current) + 1 + token_len <= limit) {
            append_with_limit(current, sizeof current, " ");
            append_with_limit(current, sizeof current, token);
        }
        else {
            ui_trimmed_copy(wrapped->lines[wrapped->line_count], sizeof wrapped->lines[0], current);
            wrapped->line_count++;
            ui_trimmed_copy(current, sizeof current, token);
        }
    }
    if(current[0] && wrapped->line_count < MAX_WRAPPED_LINES) {
        ui_trimmed_copy(wrapped->lines[wrapped->line_count], sizeof wrapped->lines[0], current);
        wrapped->line_count++;
    }
}

static void ui_wrap_text_to_width(const char *body, WrappedText *wrapped, int limit)
{
    const char *cursor = body;

    if(!wrapped) return;
    wrapped->line_count = 0;
    if(!body) return;

    while(*cursor && wrapped->line_count < MAX_WRAPPED_LINES) {
        char line[2048];
        int line_len = 0;

        while(*cursor && *cursor != '\n' && line_len < (int)sizeof(line) - 1) {
            line[line_len++] = *cursor++;
        }
        line[line_len] = 0;
        wrap_line(line, wrapped, limit);
        if(*cursor == '\n') cursor++;
    }
}

void ui_wrap_text(const char *body, WrappedText *wrapped)
{
    ui_wrap_text_to_width(body, wrapped, MAX_LINE_LEN);
}

void ui_wrap_text_for_width(const char *body, WrappedText *wrapped, int columns)
{
    ui_wrap_text_to_width(body, wrapped, columns);
}

static int ui_visible_text_columns(void)
{
    int columns = (k_content_w - (k_content_pad_x * 2)) / k_char_w;
    if(columns < 24) columns = 24;
    if(columns > MAX_LINE_LEN) columns = MAX_LINE_LEN;
    return columns;
}

static int ui_visible_columns_for_width(int width)
{
    int columns = (width - 12) / k_char_w;
    if(columns < 12) columns = 12;
    if(columns > MAX_LINE_LEN) columns = MAX_LINE_LEN;
    return columns;
}

static int ui_virtual_text_columns(void)
{
    int columns = ui_visible_text_columns() + 18;
    if(columns < ui_visible_text_columns()) columns = ui_visible_text_columns();
    if(columns > MAX_LINE_LEN) columns = MAX_LINE_LEN;
    return columns;
}

static int ui_virtual_columns_for_width(int width)
{
    int columns = ui_visible_columns_for_width(width) + 18;
    if(columns < ui_visible_columns_for_width(width)) columns = ui_visible_columns_for_width(width);
    if(columns > MAX_LINE_LEN) columns = MAX_LINE_LEN;
    return columns;
}

int ui_visible_text_lines(void)
{
    int lines = (k_content_h - (k_content_pad_y * 2)) / k_line_h;
    if(lines < 6) lines = 6;
    return lines;
}

static int ui_visible_lines_for_height(int height)
{
    int lines = (height - 10) / k_line_h;
    if(lines < 1) lines = 1;
    return lines;
}

static int ui_max_line_length(const WrappedText *wrapped)
{
    int i = 0;
    int max_length = 0;
    if(!wrapped) return 0;
    for(i = 0; i < wrapped->line_count; i++) {
        int length = (int)strlen(wrapped->lines[i]);
        if(length > max_length) max_length = length;
    }
    return max_length;
}

void ui_clamp_text_view(const char *body, int *scroll, int *x_offset)
{
    int visible_lines = ui_visible_text_lines();
    int visible_columns = ui_visible_text_columns();
    int max_scroll = 0;
    int max_x_offset = 0;

    if(!scroll || !x_offset) return;
    ui_wrap_text_to_width(body ? body : "", &g_text_layout, ui_virtual_text_columns());
    max_scroll = g_text_layout.line_count - visible_lines;
    if(max_scroll < 0) max_scroll = 0;
    max_x_offset = ui_max_line_length(&g_text_layout) - visible_columns;
    if(max_x_offset < 0) max_x_offset = 0;
    if(*scroll < 0) *scroll = 0;
    if(*scroll > max_scroll) *scroll = max_scroll;
    if(*x_offset < 0) *x_offset = 0;
    if(*x_offset > max_x_offset) *x_offset = max_x_offset;
}

void ui_clamp_text_view_for_size(const char *body, int *scroll, int *x_offset, int width, int height)
{
    int visible_lines = ui_visible_lines_for_height(height);
    int visible_columns = ui_visible_columns_for_width(width);
    int max_scroll = 0;
    int max_x_offset = 0;

    if(!scroll || !x_offset) return;
    ui_wrap_text_to_width(body ? body : "", &g_text_layout, ui_virtual_columns_for_width(width));
    max_scroll = g_text_layout.line_count - visible_lines;
    if(max_scroll < 0) max_scroll = 0;
    max_x_offset = ui_max_line_length(&g_text_layout) - visible_columns;
    if(max_x_offset < 0) max_x_offset = 0;
    if(*scroll < 0) *scroll = 0;
    if(*scroll > max_scroll) *scroll = max_scroll;
    if(*x_offset < 0) *x_offset = 0;
    if(*x_offset > max_x_offset) *x_offset = max_x_offset;
}

void ui_handle_text_view_input(const char *body, int *scroll, int *x_offset, int key)
{
    if(!scroll || !x_offset) return;
    ui_clamp_text_view(body, scroll, x_offset);
    if(key == KEY_UP) (*scroll)--;
    if(key == KEY_DOWN) (*scroll)++;
    if(key == KEY_LEFT) (*x_offset) -= k_pan_step;
    if(key == KEY_RIGHT) (*x_offset) += k_pan_step;
    ui_clamp_text_view(body, scroll, x_offset);
}

void ui_handle_text_view_input_for_size(const char *body, int *scroll, int *x_offset, int key, int width, int height)
{
    if(!scroll || !x_offset) return;
    ui_clamp_text_view_for_size(body, scroll, x_offset, width, height);
    if(key == KEY_UP) (*scroll)--;
    if(key == KEY_DOWN) (*scroll)++;
    if(key == KEY_LEFT) (*x_offset) -= k_pan_step;
    if(key == KEY_RIGHT) (*x_offset) += k_pan_step;
    ui_clamp_text_view_for_size(body, scroll, x_offset, width, height);
}

void ui_draw_text_viewport(int x, int y, int w, int h, const char *section, const char *body, int *scroll, int *x_offset)
{
    int section_h = (section && section[0]) ? 12 : 0;
    int visible_lines = 0;
    int visible_columns = 0;
    int start = 0;
    int x_start = 0;
    int text_x = x + 6;
    int text_y = y + section_h + 5;
    int content_h = h - section_h;
    int max_scroll = 0;
    int max_x_offset = 0;
    bool can_pan_left = false;
    bool can_pan_right = false;
    bool can_scroll_up = false;
    bool can_scroll_down = false;
    int i = 0;
    char slice[MAX_LINE_LEN + 1];

    if(!scroll || !x_offset) return;
    if(w <= 0 || h <= 0) return;

    ui_draw_panel(x, y, w, h, COLOR_WHITE, COLOR_LINE);
    if(section_h > 0) {
        ui_draw_panel(x, y, w, section_h, COLOR_ACCENT_SOFT, COLOR_LINE);
        dtext(x + 6, y + 2, COLOR_ACCENT, section);
    }

    if(content_h <= 10) return;

    ui_clamp_text_view_for_size(body, scroll, x_offset, w, content_h);
    visible_lines = ui_visible_lines_for_height(content_h);
    visible_columns = ui_visible_columns_for_width(w);
    ui_wrap_text_to_width(body ? body : "", &g_text_layout, ui_virtual_columns_for_width(w));
    max_scroll = g_text_layout.line_count - visible_lines;
    if(max_scroll < 0) max_scroll = 0;
    max_x_offset = ui_max_line_length(&g_text_layout) - visible_columns;
    if(max_x_offset < 0) max_x_offset = 0;
    start = *scroll;
    x_start = *x_offset;

    for(i = 0; i < visible_lines && start + i < g_text_layout.line_count; i++) {
        const char *source = g_text_layout.lines[start + i];
        int length = (int)strlen(source);
        int copy_len = 0;
        if(x_start < length) {
            copy_len = length - x_start;
            if(copy_len > visible_columns) copy_len = visible_columns;
            memcpy(slice, source + x_start, (size_t)copy_len);
            slice[copy_len] = 0;
            dtext(text_x, text_y + i * k_line_h, COLOR_TEXT, slice);
        }
    }

    can_pan_left = x_start > 0;
    can_pan_right = x_start < max_x_offset;
    can_scroll_up = start > 0;
    can_scroll_down = start < max_scroll;
    if(can_pan_left) dtext(x + 2, y + section_h + 4, COLOR_MUTED, "<");
    if(can_pan_right) dtext(x + w - 8, y + section_h + 4, COLOR_MUTED, ">");
    if(can_scroll_up) dtext(x + w - 18, y + section_h + 1, COLOR_MUTED, "^");
    if(can_scroll_down) dtext(x + w - 18, y + h - 12, COLOR_MUTED, "v");
}

void ui_draw_text_page(const char *title, const char *section, const char *body, int *scroll, int *x_offset, const char *extra_left_hint, const char *extra_right_hint)
{
    int visible_lines = ui_visible_text_lines();
    int visible_columns = ui_visible_text_columns();
    int start = 0;
    int x_start = 0;
    int panel_y = k_content_y;
    int text_y = panel_y + k_content_pad_y;
    int text_x = k_content_x + k_content_pad_x;
    int i = 0;
    int max_scroll = 0;
    int max_x_offset = 0;
    char page_hint[24];
    char status_left[72];
    char footer_right[52];
    char slice[MAX_LINE_LEN + 1];
    bool can_pan_left = false;
    bool can_pan_right = false;
    bool can_scroll_up = false;
    bool can_scroll_down = false;

    if(!scroll || !x_offset) return;

    ui_clear();
    ui_draw_header(title, "");
    ui_draw_panel(8, k_header_h + 3, SCREEN_W - 16, k_section_h, COLOR_ACCENT_SOFT, COLOR_LINE);
    dtext(14, k_header_h + 5, COLOR_ACCENT, section ? section : "Section");

    ui_clamp_text_view(body, scroll, x_offset);
    ui_wrap_text_to_width(body ? body : "", &g_text_layout, ui_virtual_text_columns());
    max_scroll = g_text_layout.line_count - visible_lines;
    if(max_scroll < 0) max_scroll = 0;
    max_x_offset = ui_max_line_length(&g_text_layout) - visible_columns;
    if(max_x_offset < 0) max_x_offset = 0;
    start = *scroll;
    x_start = *x_offset;

    ui_draw_panel(k_content_x, panel_y, k_content_w, k_content_h, COLOR_WHITE, COLOR_LINE);

    for(i = 0; i < visible_lines && start + i < g_text_layout.line_count; i++) {
        const char *source = g_text_layout.lines[start + i];
        int length = (int)strlen(source);
        int copy_len = 0;
        if(x_start < length) {
            copy_len = length - x_start;
            if(copy_len > visible_columns) copy_len = visible_columns;
            memcpy(slice, source + x_start, (size_t)copy_len);
            slice[copy_len] = 0;
            dtext(text_x, text_y + i * k_line_h, COLOR_TEXT, slice);
        }
    }

    can_pan_left = x_start > 0;
    can_pan_right = x_start < max_x_offset;
    can_scroll_up = start > 0;
    can_scroll_down = start < max_scroll;
    if(can_pan_left) dtext(k_content_x + 2, panel_y + 6, COLOR_MUTED, "<");
    if(can_pan_right) dtext(k_content_x + k_content_w - 8, panel_y + 6, COLOR_MUTED, ">");
    if(can_scroll_up) dtext(k_content_x + k_content_w - 18, panel_y + 2, COLOR_MUTED, "^");
    if(can_scroll_down) dtext(k_content_x + k_content_w - 18, panel_y + k_content_h - 12, COLOR_MUTED, "v");

    snprintf(page_hint, sizeof page_hint, "V%d/%d H%d/%d", start + 1, max_scroll + 1, x_start, max_x_offset);
    snprintf(status_left, sizeof status_left, "Arrows read  EXIT back  %s%s", can_pan_left ? "<" : "", can_pan_right ? ">" : "");
    snprintf(footer_right, sizeof footer_right, "%s%s%s",
        extra_left_hint && extra_left_hint[0] ? extra_left_hint : "",
        (extra_left_hint && extra_left_hint[0] && extra_right_hint && extra_right_hint[0]) ? "  " : "",
        extra_right_hint && extra_right_hint[0] ? extra_right_hint : "");
    ui_draw_status_bar(status_left, page_hint);
    ui_draw_footer("F1<Sec  F2Sec>", footer_right);
    dupdate();
}

static char key_to_char(int key)
{
    (void)key;
#ifdef KEY_0
    if(key == KEY_0) return '0';
#endif
#ifdef KEY_1
    if(key == KEY_1) return '1';
#endif
#ifdef KEY_2
    if(key == KEY_2) return '2';
#endif
#ifdef KEY_3
    if(key == KEY_3) return '3';
#endif
#ifdef KEY_4
    if(key == KEY_4) return '4';
#endif
#ifdef KEY_5
    if(key == KEY_5) return '5';
#endif
#ifdef KEY_6
    if(key == KEY_6) return '6';
#endif
#ifdef KEY_7
    if(key == KEY_7) return '7';
#endif
#ifdef KEY_8
    if(key == KEY_8) return '8';
#endif
#ifdef KEY_9
    if(key == KEY_9) return '9';
#endif
#ifdef KEY_CHAR_0
    if(key == KEY_CHAR_0) return '0';
#endif
#ifdef KEY_CHAR_1
    if(key == KEY_CHAR_1) return '1';
#endif
#ifdef KEY_CHAR_2
    if(key == KEY_CHAR_2) return '2';
#endif
#ifdef KEY_CHAR_3
    if(key == KEY_CHAR_3) return '3';
#endif
#ifdef KEY_CHAR_4
    if(key == KEY_CHAR_4) return '4';
#endif
#ifdef KEY_CHAR_5
    if(key == KEY_CHAR_5) return '5';
#endif
#ifdef KEY_CHAR_6
    if(key == KEY_CHAR_6) return '6';
#endif
#ifdef KEY_CHAR_7
    if(key == KEY_CHAR_7) return '7';
#endif
#ifdef KEY_CHAR_8
    if(key == KEY_CHAR_8) return '8';
#endif
#ifdef KEY_CHAR_9
    if(key == KEY_CHAR_9) return '9';
#endif
#ifdef KEY_DOT
    if(key == KEY_DOT) return '.';
#endif
#ifdef KEY_CHAR_DP
    if(key == KEY_CHAR_DP) return '.';
#endif
#ifdef KEY_MINUS
    if(key == KEY_MINUS) return '-';
#endif
#ifdef KEY_NEG
    if(key == KEY_NEG) return '-';
#endif
#ifdef KEY_CHAR_MINUS
    if(key == KEY_CHAR_MINUS) return '-';
#endif
#ifdef KEY_CHAR_PMINUS
    if(key == KEY_CHAR_PMINUS) return '-';
#endif
    return 0;
}

bool ui_prompt_number(const char *title, const char *label, double *out_value)
{
    char buffer[MAX_INPUT_LEN + 1] = "";
    key_event_t event;
    const char *safe_label = label ? label : "";

    if(!out_value) return false;

    while(1) {
        ui_clear();
        ui_draw_header(title, "Numeric input");
        ui_draw_panel(26, 60, SCREEN_W - 52, 82, COLOR_PANEL, COLOR_LINE);
        dtext(38, 74, COLOR_TEXT, safe_label);
        ui_draw_panel(38, 94, SCREEN_W - 76, 24, COLOR_WHITE, COLOR_ACCENT);
        dtext(46, 101, COLOR_TEXT, buffer[0] ? buffer : "0");
        dtext(38, 128, COLOR_MUTED, "Digits / . / -   EXE confirm   EXIT cancel");
        ui_draw_footer("EXE Confirm", "EXIT Cancel");
        dupdate();

        event = getkey();
        if(event.key == KEY_EXE) {
            if(!buffer[0]) {
                buffer[0] = '0';
                buffer[1] = 0;
            }
            *out_value = strtod(buffer, NULL);
            return true;
        }
        if(event.key == KEY_EXIT) {
            return false;
        }
        if(event.key == KEY_DEL) {
            size_t len = strlen(buffer);
            if(len > 0) buffer[len - 1] = 0;
            continue;
        }

        {
            char input = key_to_char(event.key);
            size_t len = strlen(buffer);
            if(!input || len >= MAX_INPUT_LEN) continue;
            if(input == '.' && strchr(buffer, '.')) continue;
            if(input == '-') {
                if(len == 0) buffer[len++] = '-';
                else if(buffer[0] == '-') memmove(buffer, buffer + 1, len);
                else {
                    memmove(buffer + 1, buffer, len + 1);
                    buffer[0] = '-';
                }
                continue;
            }
            buffer[len] = input;
            buffer[len + 1] = 0;
        }
    }
}
