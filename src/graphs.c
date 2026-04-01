#include "app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool focus_is(const GraphElementEntry *focus, const char *id)
{
    return focus && id && strcmp(focus->id, id) == 0;
}

static uint16_t pick_color(bool highlighted, uint16_t base)
{
    return highlighted ? COLOR_HIGHLIGHT : base;
}

static int clamp_coord(int value, int min, int max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static void safe_dline_impl(int x1, int y1, int x2, int y2, uint16_t color)
{
    if((x1 < 0 && x2 < 0) || (x1 >= SCREEN_W && x2 >= SCREEN_W) ||
        (y1 < 0 && y2 < 0) || (y1 >= SCREEN_H && y2 >= SCREEN_H)) {
        return;
    }
    dline(
        clamp_coord(x1, 0, SCREEN_W - 1),
        clamp_coord(y1, 0, SCREEN_H - 1),
        clamp_coord(x2, 0, SCREEN_W - 1),
        clamp_coord(y2, 0, SCREEN_H - 1),
        color
    );
}

static void safe_drect_impl(int x1, int y1, int x2, int y2, uint16_t color)
{
    int left = x1;
    int top = y1;
    int right = x2;
    int bottom = y2;
    if((left < 0 && right < 0) || (left >= SCREEN_W && right >= SCREEN_W) ||
        (top < 0 && bottom < 0) || (top >= SCREEN_H && bottom >= SCREEN_H)) {
        return;
    }
    left = clamp_coord(left, 0, SCREEN_W - 1);
    top = clamp_coord(top, 0, SCREEN_H - 1);
    right = clamp_coord(right, 0, SCREEN_W - 1);
    bottom = clamp_coord(bottom, 0, SCREEN_H - 1);
    if(left > right) {
        int temp = left;
        left = right;
        right = temp;
    }
    if(top > bottom) {
        int temp = top;
        top = bottom;
        bottom = temp;
    }
    if(left == right && top == bottom) return;
    drect(left, top, right, bottom, color);
}

static void safe_dtext_impl(int x, int y, uint16_t color, const char *text)
{
    if(!text || !text[0]) return;
    if(x >= SCREEN_W || y >= SCREEN_H) return;
    if(x < -64 || y < -16) return;
    dtext(clamp_coord(x, 0, SCREEN_W - 1), clamp_coord(y, 0, SCREEN_H - 1), color, text);
}

#define dline safe_dline_impl
#define drect safe_drect_impl
#define dtext safe_dtext_impl

static void draw_line_thick(int x1, int y1, int x2, int y2, uint16_t color, bool highlighted)
{
    dline(x1, y1, x2, y2, color);
    if(highlighted) {
        dline(x1 + 1, y1, x2 + 1, y2, color);
        dline(x1, y1 + 1, x2, y2 + 1, color);
    }
}

static void draw_axes(int left, int top, int width, int height, const char *x_label, const char *y_label)
{
    int x0 = left;
    int y0 = top + height;
    int x_label_x = x0 + width - 34;
    int y_label_x = left - 16;
    if(x_label_x < x0 + 6) x_label_x = x0 + 6;
    if(y_label_x < 2) y_label_x = 2;
    dline(x0, top, x0, y0, COLOR_BLACK);
    dline(x0, y0, x0 + width, y0, COLOR_BLACK);
    dtext(x_label_x, y0 + 6, COLOR_MUTED, x_label);
    dtext(y_label_x, top - 2, COLOR_MUTED, y_label);
}

static void draw_dashed_line(int x1, int y1, int x2, int y2, uint16_t color)
{
    int steps = 22;
    int i = 0;
    for(i = 0; i < steps; i += 2) {
        int sx = x1 + (x2 - x1) * i / steps;
        int sy = y1 + (y2 - y1) * i / steps;
        int ex = x1 + (x2 - x1) * (i + 1) / steps;
        int ey = y1 + (y2 - y1) * (i + 1) / steps;
        dline(sx, sy, ex, ey, color);
    }
}

static void draw_point_marker(int x, int y, uint16_t color, bool highlighted)
{
    int size = highlighted ? 4 : 2;
    drect(x - size, y - size, x + size, y + size, color);
}

static void draw_point_annotation(int x, int y, const char *text, uint16_t color, bool enabled)
{
    if(!enabled || !text || !text[0]) return;
    dtext(x + 6, y - 10, color, text);
}

static void draw_concept_annotation(int x, int y, const char *text, uint16_t color, bool enabled)
{
    if(!enabled || !text || !text[0]) return;
    dtext(x, y, color, text);
}

static void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color)
{
    int min_y = y1;
    int max_y = y1;
    int y = 0;
    if(y2 < min_y) min_y = y2;
    if(y3 < min_y) min_y = y3;
    if(y2 > max_y) max_y = y2;
    if(y3 > max_y) max_y = y3;
    for(y = min_y; y <= max_y; y++) {
        int xs[3];
        int count = 0;
        if((y1 <= y && y2 >= y) || (y2 <= y && y1 >= y)) xs[count++] = x1 + (y - y1) * (x2 - x1) / ((y2 - y1) == 0 ? 1 : (y2 - y1));
        if((y2 <= y && y3 >= y) || (y3 <= y && y2 >= y)) xs[count++] = x2 + (y - y2) * (x3 - x2) / ((y3 - y2) == 0 ? 1 : (y3 - y2));
        if((y1 <= y && y3 >= y) || (y3 <= y && y1 >= y)) xs[count++] = x1 + (y - y1) * (x3 - x1) / ((y3 - y1) == 0 ? 1 : (y3 - y1));
        if(count >= 2) {
            int xa = xs[0];
            int xb = xs[1];
            if(xa > xb) {
                int temp = xa;
                xa = xb;
                xb = temp;
            }
            dline(xa, y, xb, y, color);
        }
    }
}

static void fill_quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, uint16_t color)
{
    fill_triangle(x1, y1, x2, y2, x3, y3, color);
    fill_triangle(x1, y1, x3, y3, x4, y4, color);
}

static void draw_info_panel_at(int x, int y, int w, int h, const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    char line1[28];
    char line2[28];
    char line3[28];
    char line4[28];
    char line5[28];
    char line6[28];

    ui_draw_panel(x, y, w, h, COLOR_PANEL, COLOR_LINE);
    ui_trimmed_copy(line1, sizeof line1, focus ? focus->name : graph->title);
    ui_trimmed_copy(line2, sizeof line2, focus ? focus->type : "Graph atlas");
    ui_trimmed_copy(line3, sizeof line3, focus ? focus->location : "F2 cycle focus");
    ui_trimmed_copy(line4, sizeof line4, focus ? focus->meaning : (options->show_shift ? "Shift overlay on" : "Shift overlay off"));
    ui_trimmed_copy(line5, sizeof line5, focus ? focus->used_for : (options->show_area ? "Region shading on" : "Region shading off"));
    ui_trimmed_copy(line6, sizeof line6, options->show_concepts ? "Concept notes on" : (options->show_points ? "Point labels on" : (options->show_labels ? "Curve labels on" : "Labels hidden")));

    dtext(x + 8, y + 10, COLOR_TEXT, line1);
    dtext(x + 8, y + 28, COLOR_MUTED, line2);
    dtext(x + 8, y + 50, COLOR_TEXT, line3);
    dtext(x + 8, y + 70, COLOR_TEXT, line4);
    dtext(x + 8, y + 90, COLOR_TEXT, line5);
    dtext(x + 8, y + 110, COLOR_MUTED, line6);
}

static void draw_supply_demand_base(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options, bool surplus_mode, bool tax_mode, bool subsidy_mode, bool ceiling_mode, bool floor_mode)
{
    int eq_x = left + 112;
    int eq_y = top + 70;
    bool show_points = options->show_points || focus_is(focus, "equilibrium") || focus_is(focus, "buyer-price") || focus_is(focus, "seller-price") || focus_is(focus, "quantity-demanded") || focus_is(focus, "quantity-supplied");
    bool hi_d = focus_is(focus, "demand");
    bool hi_s = focus_is(focus, "supply") || focus_is(focus, "taxed-supply") || focus_is(focus, "subsidized-supply");

    draw_axes(left, top, width, height, "Q", "P");
    if((options->show_area || focus_is(focus, "consumer-surplus")) && (surplus_mode || strcmp(focus ? focus->id : "", "consumer-surplus") == 0)) {
        fill_triangle(left + 10, top + 78, eq_x, top + 78, left + 12, top + 18, COLOR_ACCENT_SOFT);
    }
    if((options->show_area || focus_is(focus, "producer-surplus")) && (surplus_mode || strcmp(focus ? focus->id : "", "producer-surplus") == 0)) {
        fill_triangle(left + 12, top + 78, eq_x, top + 78, left + 8, top + height - 18, COLOR_PANEL);
    }
    draw_line_thick(left + 8, top + height - 18, left + width - 20, top + 18, pick_color(hi_s, COLOR_WARN), hi_s);
    draw_line_thick(left + 12, top + 18, left + width - 14, top + height - 20, pick_color(hi_d, COLOR_ACCENT), hi_d);

    if(options->show_shift) {
        if(tax_mode) draw_dashed_line(left + 12, top + 2, left + width - 14, top + height - 36, COLOR_GOOD);
        else if(subsidy_mode) draw_dashed_line(left + 12, top + 36, left + width - 14, top + height, COLOR_GOOD);
        else draw_dashed_line(left + 26, top + 8, left + width - 6, top + height - 32, COLOR_GOOD);
    }

    if(ceiling_mode) {
        dline(left + 2, top + 86, left + width - 10, top + 86, pick_color(focus_is(focus, "ceiling"), COLOR_GOOD));
        if(options->show_area || focus_is(focus, "shortage")) fill_quad(left + 88, top + 86, left + 148, top + 86, left + 148, top + height, left + 88, top + height, COLOR_ACCENT_SOFT);
        if(show_points) {
            dline(left + 88, top + 86, left + 88, top + height, COLOR_MUTED);
            dline(left + 148, top + 86, left + 148, top + height, COLOR_MUTED);
        }
    }
    else if(floor_mode) {
        dline(left + 2, top + 44, left + width - 10, top + 44, pick_color(focus_is(focus, "floor"), COLOR_GOOD));
        if(options->show_area || focus_is(focus, "surplus")) fill_quad(left + 78, top + 44, left + 156, top + 44, left + 156, top + height, left + 78, top + height, COLOR_ACCENT_SOFT);
        if(show_points) {
            dline(left + 78, top + 44, left + 78, top + height, COLOR_MUTED);
            dline(left + 156, top + 44, left + 156, top + height, COLOR_MUTED);
        }
    }
    else if(tax_mode) {
        if(options->show_area || focus_is(focus, "tax-revenue")) fill_quad(eq_x, top + 58, eq_x + 18, top + 58, eq_x + 18, top + 102, eq_x, top + 102, COLOR_ACCENT_SOFT);
        if(options->show_area || focus_is(focus, "deadweight-loss")) fill_triangle(eq_x + 18, top + 80, eq_x + 58, top + 62, eq_x + 58, top + 98, COLOR_PANEL);
        if(show_points) {
            dline(eq_x + 18, top + 58, eq_x + 18, top + 102, COLOR_MUTED);
            draw_point_marker(eq_x + 18, top + 58, pick_color(focus_is(focus, "buyer-price"), COLOR_ACCENT), focus_is(focus, "buyer-price"));
            draw_point_marker(eq_x + 18, top + 102, pick_color(focus_is(focus, "seller-price"), COLOR_WARN), focus_is(focus, "seller-price"));
        }
    }
    else if(subsidy_mode) {
        if(options->show_area || focus_is(focus, "government-spending")) fill_quad(eq_x + 18, top + 40, eq_x + 36, top + 40, eq_x + 36, top + 86, eq_x + 18, top + 86, COLOR_ACCENT_SOFT);
        if(options->show_area || focus_is(focus, "deadweight-loss")) fill_triangle(eq_x + 36, top + 58, eq_x + 72, top + 40, eq_x + 72, top + 86, COLOR_PANEL);
        if(show_points) {
            dline(eq_x + 36, top + 40, eq_x + 36, top + 86, COLOR_MUTED);
            draw_point_marker(eq_x + 36, top + 86, pick_color(focus_is(focus, "buyer-price"), COLOR_ACCENT), focus_is(focus, "buyer-price"));
            draw_point_marker(eq_x + 36, top + 40, pick_color(focus_is(focus, "seller-price"), COLOR_WARN), focus_is(focus, "seller-price"));
        }
    }
    else {
        if(show_points) draw_point_marker(eq_x, eq_y, pick_color(focus_is(focus, "equilibrium"), COLOR_BLACK), focus_is(focus, "equilibrium"));
        if(options->show_shift && show_points) draw_point_marker(eq_x + 26, eq_y - 20, COLOR_GOOD, false);
    }

    if(options->show_labels) {
        dtext(left + width - 36, top + 24, COLOR_WARN, (tax_mode || subsidy_mode) ? "S'" : "S");
        dtext(left + width - 42, top + height - 28, COLOR_ACCENT, "D");
        if(!ceiling_mode && !floor_mode && !tax_mode && !subsidy_mode && show_points) dtext(eq_x + 6, eq_y - 10, COLOR_BLACK, "E");
        if(ceiling_mode) dtext(left + width - 48, top + 78, COLOR_GOOD, "PC");
        if(floor_mode) dtext(left + width - 42, top + 36, COLOR_GOOD, "PF");
        if(surplus_mode) {
            dtext(left + 50, top + 46, COLOR_GOOD, "CS");
            dtext(left + 74, top + 104, COLOR_WARN, "PS");
        }
        if(tax_mode || subsidy_mode) {
            dtext(eq_x + 26, top + 50, COLOR_ACCENT, "Pb");
            dtext(eq_x + 26, top + 96, COLOR_WARN, "Ps");
        }
    }
    if(options->show_points) {
        if(ceiling_mode) {
            draw_point_annotation(left + 88, top + height - 12, "Qs", COLOR_MUTED, true);
            draw_point_annotation(left + 148, top + height - 12, "Qd", COLOR_MUTED, true);
        }
        else if(floor_mode) {
            draw_point_annotation(left + 78, top + height - 12, "Qd", COLOR_MUTED, true);
            draw_point_annotation(left + 156, top + height - 12, "Qs", COLOR_MUTED, true);
        }
        else if(!tax_mode && !subsidy_mode) {
            draw_point_annotation(eq_x, eq_y, "P*, Q*", COLOR_BLACK, true);
        }
    }
    if(options->show_concepts) {
        if(surplus_mode) {
            draw_concept_annotation(left + 34, top + 22, "Buyer gains", COLOR_GOOD, true);
            draw_concept_annotation(left + 44, top + 118, "Seller gains", COLOR_WARN, true);
        }
        else if(tax_mode) {
            draw_concept_annotation(left + 38, top + 20, "Tax wedge", COLOR_GOOD, true);
            draw_concept_annotation(eq_x + 46, top + 112, "Lower output", COLOR_WARN, true);
        }
        else if(subsidy_mode) {
            draw_concept_annotation(left + 36, top + 20, "Subsidy wedge", COLOR_GOOD, true);
            draw_concept_annotation(eq_x + 52, top + 108, "Overproduction", COLOR_WARN, true);
        }
        else if(ceiling_mode) {
            draw_concept_annotation(left + 32, top + 18, "Binding ceiling", COLOR_GOOD, true);
            draw_concept_annotation(left + 114, top + 120, "Shortage", COLOR_WARN, true);
        }
        else if(floor_mode) {
            draw_concept_annotation(left + 36, top + 18, "Binding floor", COLOR_GOOD, true);
            draw_concept_annotation(left + 110, top + 120, "Surplus", COLOR_WARN, true);
        }
        else {
            draw_concept_annotation(left + 28, top + 18, "Market equilibrium", COLOR_GOOD, true);
        }
    }
}

static void draw_cost_family(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options, bool pc_firm)
{
    int qx = left + 124;
    int qy = top + 66;
    draw_axes(left, top, width, height, "Q", pc_firm ? "P/Cost" : "Cost");
    if(pc_firm) {
        dline(left + 14, top + 66, left + width - 8, top + 66, pick_color(focus_is(focus, "price-mr"), COLOR_GOOD));
        if(options->show_shift) dline(left + 14, top + 54, left + width - 8, top + 54, COLOR_ACCENT);
        if(options->show_area || focus_is(focus, "profit")) fill_quad(left + 124, top + 66, left + 124, top + 88, left + 164, top + 88, left + 164, top + 66, COLOR_ACCENT_SOFT);
    }
    draw_line_thick(left + 10, top + 26, left + 56, top + 114, pick_color(focus_is(focus, "mc"), COLOR_WARN), focus_is(focus, "mc"));
    draw_line_thick(left + 56, top + 114, left + width - 14, top + 26, pick_color(focus_is(focus, "mc"), COLOR_WARN), focus_is(focus, "mc"));
    draw_line_thick(left + 6, top + 64, left + 68, top + 116, pick_color(focus_is(focus, "avc"), COLOR_ACCENT), focus_is(focus, "avc"));
    draw_line_thick(left + 68, top + 116, left + width - 18, top + 54, pick_color(focus_is(focus, "avc"), COLOR_ACCENT), focus_is(focus, "avc"));
    draw_line_thick(left + 10, top + 40, left + 84, top + 120, pick_color(focus_is(focus, "atc"), COLOR_GOOD), focus_is(focus, "atc"));
    draw_line_thick(left + 84, top + 120, left + width - 16, top + 72, pick_color(focus_is(focus, "atc"), COLOR_GOOD), focus_is(focus, "atc"));
    draw_line_thick(left + 10, top + 30, left + width - 12, top + 108, pick_color(focus_is(focus, "afc"), COLOR_MUTED), focus_is(focus, "afc"));
    if(options->show_shift && !pc_firm) draw_dashed_line(left + 12, top + 52, left + width - 16, top + 84, COLOR_HIGHLIGHT);
    if(pc_firm && options->show_points) {
        dline(qx, qy, qx, top + height, COLOR_MUTED);
        draw_point_marker(qx, qy, COLOR_BLACK, focus_is(focus, "profit-quantity"));
        draw_point_annotation(qx, qy, "Q*", COLOR_BLACK, true);
    }
    if(options->show_labels) {
        dtext(left + width - 28, top + 30, COLOR_WARN, "MC");
        dtext(left + width - 36, top + 58, COLOR_ACCENT, "AVC");
        dtext(left + width - 34, top + 78, COLOR_GOOD, "ATC");
        dtext(left + width - 34, top + 108, COLOR_MUTED, "AFC");
        if(pc_firm) dtext(left + 132, top + 58, COLOR_BLACK, "P=MR");
    }
    if(options->show_concepts) {
        if(pc_firm) {
            draw_concept_annotation(left + 118, top + 94, "Profit maximizing", COLOR_GOOD, true);
            if(options->show_area || focus_is(focus, "profit")) draw_concept_annotation(left + 124, top + 92, "Profit", COLOR_ACCENT, true);
        }
        else {
            draw_concept_annotation(left + 120, top + 16, "Rising MC", COLOR_WARN, true);
            draw_concept_annotation(left + 126, top + 130, "Shutdown uses AVC", COLOR_ACCENT, true);
        }
    }
}

static void draw_monopoly_family(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options, bool monopolistic)
{
    int qx = left + 122;
    int py = top + 46;
    draw_axes(left, top, width, height, "Q", "P/Cost");
    draw_line_thick(left + 10, top + 18, left + width - 18, top + (monopolistic ? 92 : 96), pick_color(focus_is(focus, "demand"), COLOR_ACCENT), focus_is(focus, "demand"));
    draw_dashed_line(left + 10, top + 18, left + width - 18, top + 136, pick_color(focus_is(focus, "mr"), COLOR_ACCENT));
    draw_line_thick(left + 8, top + (monopolistic ? 84 : 88), left + 74, top + 126, pick_color(focus_is(focus, "mc"), COLOR_WARN), focus_is(focus, "mc"));
    draw_line_thick(left + 74, top + 126, left + width - 18, top + (monopolistic ? 42 : 28), pick_color(focus_is(focus, "mc"), COLOR_WARN), focus_is(focus, "mc"));
    draw_line_thick(left + 8, top + 54, left + 84, top + 120, pick_color(focus_is(focus, "atc"), COLOR_MUTED), focus_is(focus, "atc"));
    draw_line_thick(left + 84, top + 120, left + width - 18, top + 76, pick_color(focus_is(focus, "atc"), COLOR_MUTED), focus_is(focus, "atc"));
    if(options->show_points) {
        dline(qx, top + 72, qx, top + height, COLOR_BLACK);
        dline(left, py, qx, py, COLOR_BLACK);
        draw_point_marker(qx, top + 72, COLOR_BLACK, focus_is(focus, "profit-max-q"));
        draw_point_annotation(qx, top + 72, monopolistic ? "Qmr=mc" : "Qm", COLOR_BLACK, true);
    }
    if(options->show_area || focus_is(focus, monopolistic ? "excess-capacity" : "deadweight-loss")) {
        fill_triangle(left + 122, top + 72, left + 166, top + 58, left + 166, top + 92, COLOR_ACCENT_SOFT);
    }
    if(options->show_shift && !monopolistic) {
        draw_dashed_line(left + 150, top + 52, left + 150, top + height, COLOR_GOOD);
        draw_dashed_line(left, top + 58, left + 150, top + 58, COLOR_GOOD);
    }
    if(options->show_labels) {
        dtext(left + width - 34, top + 92, COLOR_ACCENT, "D");
        dtext(left + width - 34, top + 136, COLOR_ACCENT, "MR");
        dtext(left + width - 30, top + 30, COLOR_WARN, "MC");
        dtext(left + width - 30, top + 76, COLOR_MUTED, monopolistic ? "ATC" : "ATC");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 110, top + 100, "Profit maximizing", COLOR_GOOD, true);
        if(monopolistic) draw_concept_annotation(left + 132, top + 118, "Excess capacity", COLOR_WARN, true);
        else {
            draw_concept_annotation(left + 150, top + 24, "Allocatively efficient", COLOR_GOOD, options->show_shift);
            draw_concept_annotation(left + 136, top + 114, "DWL", COLOR_WARN, options->show_area || focus_is(focus, "deadweight-loss"));
        }
    }
}

static void draw_kinked_demand(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int kink_x = left + 116;
    int kink_y = top + 72;
    draw_axes(left, top, width, height, "Q", "P/R");
    draw_line_thick(left + 18, top + 26, kink_x, kink_y, pick_color(focus_is(focus, "demand"), COLOR_ACCENT), focus_is(focus, "demand"));
    draw_line_thick(kink_x, kink_y, left + width - 18, top + 126, pick_color(focus_is(focus, "demand"), COLOR_ACCENT), focus_is(focus, "demand"));
    draw_line_thick(left + 18, top + 52, kink_x, top + 102, pick_color(focus_is(focus, "mr-gap"), COLOR_WARN), focus_is(focus, "mr-gap"));
    draw_line_thick(kink_x, top + 122, left + width - 18, top + 150, pick_color(focus_is(focus, "mr-gap"), COLOR_WARN), focus_is(focus, "mr-gap"));
    dline(kink_x, top + 102, kink_x, top + 122, pick_color(focus_is(focus, "mr-gap"), COLOR_WARN));
    draw_line_thick(left + 46, top + 132, left + width - 26, top + 36, pick_color(focus_is(focus, "price-rigidity"), COLOR_GOOD), focus_is(focus, "price-rigidity"));
    if(options->show_points) {
        draw_point_marker(kink_x, kink_y, pick_color(focus_is(focus, "price-rigidity"), COLOR_BLACK), focus_is(focus, "price-rigidity"));
        draw_point_annotation(kink_x, kink_y, "Kink", COLOR_BLACK, true);
    }
    if(options->show_shift) {
        draw_dashed_line(left + 54, top + 142, left + width - 20, top + 46, COLOR_HIGHLIGHT);
    }
    if(options->show_labels) {
        dtext(left + width - 34, top + 126, COLOR_ACCENT, "D");
        dtext(left + width - 38, top + 148, COLOR_WARN, "MR");
        dtext(left + width - 30, top + 40, COLOR_GOOD, "MC");
        dtext(kink_x + 6, kink_y - 12, COLOR_BLACK, "K");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 34, top + 18, "Price rigidity", COLOR_GOOD, true);
        draw_concept_annotation(left + 142, top + 112, "MR gap", COLOR_WARN, true);
    }
}

static void draw_game_matrix(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int grid_left = left + 34;
    int grid_top = top + 24;
    int cell_w = (width - 56) / 2;
    int cell_h = (height - 52) / 2;
    int row2_y = grid_top + cell_h;
    int col2_x = grid_left + cell_w;
    uint16_t collusion_color = (options->show_area || focus_is(focus, "collusive-outcome")) ? COLOR_ACCENT_SOFT : COLOR_PANEL;
    uint16_t nash_color = (options->show_area || focus_is(focus, "nash-equilibrium")) ? COLOR_PANEL : COLOR_WHITE;

    ui_draw_panel(grid_left, grid_top, cell_w * 2, cell_h * 2, COLOR_WHITE, COLOR_LINE);
    dline(col2_x, grid_top, col2_x, grid_top + cell_h * 2, COLOR_BLACK);
    dline(grid_left, row2_y, grid_left + cell_w * 2, row2_y, COLOR_BLACK);
    drect(grid_left + 1, grid_top + 1, col2_x - 1, row2_y - 1, collusion_color);
    drect(col2_x + 1, row2_y + 1, grid_left + cell_w * 2 - 1, grid_top + cell_h * 2 - 1, nash_color);
    if(focus_is(focus, "collusive-outcome")) {
        dline(grid_left + 2, grid_top + 2, col2_x - 2, grid_top + 2, COLOR_HIGHLIGHT);
        dline(grid_left + 2, row2_y - 2, col2_x - 2, row2_y - 2, COLOR_HIGHLIGHT);
        dline(grid_left + 2, grid_top + 2, grid_left + 2, row2_y - 2, COLOR_HIGHLIGHT);
        dline(col2_x - 2, grid_top + 2, col2_x - 2, row2_y - 2, COLOR_HIGHLIGHT);
    }
    if(focus_is(focus, "nash-equilibrium")) {
        int right = grid_left + cell_w * 2 - 2;
        int bottom = grid_top + cell_h * 2 - 2;
        dline(col2_x + 2, row2_y + 2, right, row2_y + 2, COLOR_HIGHLIGHT);
        dline(col2_x + 2, bottom, right, bottom, COLOR_HIGHLIGHT);
        dline(col2_x + 2, row2_y + 2, col2_x + 2, bottom, COLOR_HIGHLIGHT);
        dline(right, row2_y + 2, right, bottom, COLOR_HIGHLIGHT);
    }
    if(focus_is(focus, "dominant-strategy")) {
        dline(grid_left - 10, row2_y + cell_h / 2, grid_left + cell_w * 2 + 6, row2_y + cell_h / 2, COLOR_GOOD);
        dline(col2_x + cell_w / 2, grid_top - 10, col2_x + cell_w / 2, grid_top + cell_h * 2 + 6, COLOR_GOOD);
    }
    if(options->show_shift || focus_is(focus, "prisoners-dilemma")) {
        dline(grid_left + cell_w / 2, grid_top + cell_h / 2, col2_x + cell_w / 2, row2_y + cell_h / 2, COLOR_WARN);
        dline(col2_x + cell_w / 2, row2_y + cell_h / 2, col2_x + cell_w / 2 - 6, row2_y + cell_h / 2 - 4, COLOR_WARN);
        dline(col2_x + cell_w / 2, row2_y + cell_h / 2, col2_x + cell_w / 2 - 6, row2_y + cell_h / 2 + 4, COLOR_WARN);
    }
    if(options->show_labels) {
        dtext(grid_left + 10, grid_top - 16, COLOR_MUTED, "B: Cooperate");
        dtext(col2_x + 10, grid_top - 16, COLOR_MUTED, "B: Cheat");
        dtext(left, grid_top + 14, COLOR_MUTED, "A: Coop");
        dtext(left, row2_y + 14, COLOR_MUTED, "A: Cheat");
        dtext(grid_left + 8, grid_top + 12, COLOR_TEXT, "8,8");
        dtext(col2_x + 8, grid_top + 12, COLOR_TEXT, "2,10");
        dtext(grid_left + 8, row2_y + 12, COLOR_TEXT, "10,2");
        dtext(col2_x + 8, row2_y + 12, COLOR_TEXT, "4,4");
    }
    if(options->show_concepts) {
        dtext(grid_left + 12, row2_y + cell_h + 16, COLOR_WARN, "Nash");
        dtext(grid_left + 6, grid_top + cell_h + 16, COLOR_GOOD, "Joint profit");
    }
}

static void draw_labor_family(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options, bool monopsony)
{
    int eq_x = left + 132;
    int eq_y = top + 58;
    draw_axes(left, top, width, height, "Labor", "W/MRP");
    draw_line_thick(left + 10, top + 20, left + width - 20, top + 116, pick_color(focus_is(focus, monopsony ? "labor-supply" : "labor-supply"), COLOR_WARN), focus_is(focus, "labor-supply"));
    draw_line_thick(left + 12, top + 112, left + width - 18, top + 30, pick_color(focus_is(focus, monopsony ? "mrp" : "mrp"), COLOR_ACCENT), focus_is(focus, "mrp"));
    if(monopsony) draw_dashed_line(left + 18, top + 94, left + width - 20, top + 12, pick_color(focus_is(focus, "mfc"), COLOR_GOOD));
    if(options->show_points) {
        dline(eq_x, eq_y, eq_x, top + height, COLOR_BLACK);
        draw_point_marker(eq_x, eq_y, pick_color(focus_is(focus, monopsony ? "hire-quantity" : "labor-equilibrium") || focus_is(focus, monopsony ? "hire-quantity" : "equilibrium-wage"), COLOR_BLACK), focus_is(focus, monopsony ? "hire-quantity" : "labor-equilibrium") || focus_is(focus, monopsony ? "hire-quantity" : "equilibrium-wage"));
        if(monopsony) {
            dline(eq_x, eq_y, left, eq_y, COLOR_MUTED);
            dline(eq_x, top + 94, left, top + 94, COLOR_MUTED);
        }
    }
    if(options->show_shift) {
        if(monopsony) {
            draw_dashed_line(left + 10, top + 20, left + width - 20, top + 116, COLOR_GOOD);
            draw_point_marker(left + 158, top + 76, COLOR_GOOD, false);
            dline(left + 158, top + 76, left + 158, top + height, COLOR_GOOD);
            dline(left, top + 76, left + 158, top + 76, COLOR_GOOD);
        }
        else {
            draw_dashed_line(left + 22, top + 102, left + width - 10, top + 20, COLOR_GOOD);
        }
    }
    if(options->show_labels) {
        dtext(left + width - 54, top + 116, COLOR_WARN, monopsony ? "Labor S" : "Labor S");
        dtext(left + width - 44, top + 34, COLOR_ACCENT, monopsony ? "MRP" : "LD");
        if(monopsony) dtext(left + width - 44, top + 14, COLOR_GOOD, "MFC");
        if(!monopsony) {
            dtext(left + 138, top + 48, COLOR_BLACK, "W*");
            dtext(left + 138, top + height - 16, COLOR_BLACK, "L*");
        } else {
            dtext(left + 138, top + 52, COLOR_BLACK, "Lm");
            dtext(left + 138, top + 88, COLOR_WARN, "Wm");
            if(options->show_shift) {
                dtext(left + 164, top + 70, COLOR_GOOD, "Lc");
                dtext(left + 164, top + 20, COLOR_GOOD, "Wc");
            }
        }
    }
    if(options->show_points) {
        if(!monopsony) draw_point_annotation(eq_x, eq_y, "Competitive eq", COLOR_BLACK, true);
        else {
            draw_point_annotation(eq_x, eq_y, "Lm", COLOR_BLACK, true);
            draw_point_annotation(eq_x, top + 94, "Wm", COLOR_WARN, true);
        }
    }
    if(options->show_concepts) {
        if(!monopsony) {
            draw_concept_annotation(left + 44, top + 18, "Competitive labor market", COLOR_GOOD, true);
            draw_concept_annotation(left + 116, top + 100, "Equilibrium wage", COLOR_ACCENT, true);
        }
        else {
            draw_concept_annotation(left + 38, top + 18, "Monopsony hires where MRP = MFC", COLOR_GOOD, true);
            draw_concept_annotation(left + 140, top + 108, "Lower wage and labor", COLOR_WARN, true);
            draw_concept_annotation(left + 154, top + 28, "Competitive benchmark", COLOR_GOOD, options->show_shift);
        }
    }
}

static void draw_hiring_rule(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int hire_x = left + 132;
    int hire_y = top + 72;
    draw_axes(left, top, width, height, "Labor", "W/MRP");
    dline(left + 14, top + 72, left + width - 10, top + 72, pick_color(focus_is(focus, "wage-mrc"), COLOR_GOOD));
    draw_line_thick(left + 14, top + 108, left + width - 16, top + 26, pick_color(focus_is(focus, "mrp"), COLOR_ACCENT), focus_is(focus, "mrp"));
    if(options->show_points) {
        dline(hire_x, hire_y, hire_x, top + height, COLOR_BLACK);
        draw_point_marker(hire_x, hire_y, pick_color(focus_is(focus, "hire-quantity"), COLOR_BLACK), focus_is(focus, "hire-quantity"));
        draw_point_annotation(hire_x, hire_y, "Hire here", COLOR_BLACK, true);
    }
    if(options->show_shift) draw_dashed_line(left + 14, top + 94, left + width - 16, top + 12, COLOR_WARN);
    if(options->show_labels) {
        dtext(left + width - 42, top + 66, COLOR_GOOD, "W=MRC");
        dtext(left + width - 34, top + 28, COLOR_ACCENT, "MRP");
        dtext(left + 138, top + 62, COLOR_BLACK, "L*");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 42, top + 18, "Hire until MRP = MRC", COLOR_GOOD, true);
        draw_concept_annotation(left + 126, top + 92, "Profit-max labor", COLOR_WARN, true);
    }
}

static void draw_externality_family(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options, bool positive)
{
    int market_x = positive ? left + 96 : left + 128;
    int social_x = positive ? left + 128 : left + 96;
    int market_y = positive ? top + 78 : top + 62;
    int social_y = positive ? top + 62 : top + 78;
    bool show_policy = options->show_shift || focus_is(focus, positive ? "subsidy" : "corrective-tax");
    bool show_gap = options->show_area || focus_is(focus, positive ? "external-benefit" : "external-cost");
    bool show_dwl = options->show_area || focus_is(focus, "deadweight-loss");

    draw_axes(left, top, width, height, "Q", "Cost/Benefit");
    if(!positive) {
        draw_line_thick(left + 8, top + height - 20, left + width - 18, top + 24, pick_color(focus_is(focus, "mpc"), COLOR_ACCENT), focus_is(focus, "mpc"));
        draw_line_thick(left + 10, top + height - 52, left + width - 18, top - 8, pick_color(focus_is(focus, "msc"), COLOR_WARN), focus_is(focus, "msc"));
        draw_line_thick(left + 14, top + 20, left + width - 14, top + height - 18, pick_color(focus_is(focus, "msb"), COLOR_GOOD), focus_is(focus, "msb"));
        if(options->show_points) {
            dline(market_x, market_y, market_x, top + height, pick_color(focus_is(focus, "market-quantity"), COLOR_BLACK));
            dline(social_x, social_y, social_x, top + height, pick_color(focus_is(focus, "socially-optimal"), COLOR_BLACK));
            draw_point_marker(market_x, market_y, pick_color(focus_is(focus, "market-quantity"), COLOR_BLACK), focus_is(focus, "market-quantity"));
            draw_point_marker(social_x, social_y, pick_color(focus_is(focus, "socially-optimal"), COLOR_BLACK), focus_is(focus, "socially-optimal"));
        }
        if(show_gap) fill_quad(left + 150, top + 40, left + 170, top + 32, left + 170, top + 64, left + 150, top + 72, COLOR_ACCENT_SOFT);
        if(show_dwl) fill_triangle(social_x, top + 78, market_x, top + 62, market_x, top + 108, COLOR_PANEL);
        if(show_policy) draw_dashed_line(market_x, top + 40, social_x, top + 40, COLOR_GOOD);
    } else {
        draw_line_thick(left + 8, top + height - 20, left + width - 18, top + 24, pick_color(focus_is(focus, "mpb"), COLOR_GOOD), focus_is(focus, "mpb"));
        draw_line_thick(left + 10, top + height - 52, left + width - 18, top - 8, pick_color(focus_is(focus, "msb"), COLOR_ACCENT), focus_is(focus, "msb"));
        draw_line_thick(left + 14, top + 20, left + width - 14, top + height - 18, pick_color(focus_is(focus, "msc"), COLOR_WARN), focus_is(focus, "msc"));
        if(options->show_points) {
            dline(market_x, market_y, market_x, top + height, pick_color(focus_is(focus, "market-quantity"), COLOR_BLACK));
            dline(social_x, social_y, social_x, top + height, pick_color(focus_is(focus, "socially-optimal"), COLOR_BLACK));
            draw_point_marker(market_x, market_y, pick_color(focus_is(focus, "market-quantity"), COLOR_BLACK), focus_is(focus, "market-quantity"));
            draw_point_marker(social_x, social_y, pick_color(focus_is(focus, "socially-optimal"), COLOR_BLACK), focus_is(focus, "socially-optimal"));
        }
        if(show_gap) fill_quad(left + 150, top + 40, left + 170, top + 16, left + 170, top + 48, left + 150, top + 72, COLOR_ACCENT_SOFT);
        if(show_dwl) fill_triangle(market_x, top + 78, social_x, top + 62, social_x, top + 108, COLOR_PANEL);
        if(show_policy) draw_dashed_line(market_x, top + 40, social_x, top + 40, COLOR_GOOD);
    }
    if(options->show_labels) {
        dtext(left + width - 34, top + 18, positive ? COLOR_ACCENT : COLOR_WARN, positive ? "MSB" : "MSC");
        dtext(left + width - 34, top + 46, positive ? COLOR_GOOD : COLOR_ACCENT, positive ? "MPB" : "MPC");
        dtext(left + width - 30, top + height - 26, positive ? COLOR_WARN : COLOR_GOOD, positive ? "MSC" : "MSB");
        dtext(market_x - 10, top + height - 14, COLOR_BLACK, "Qm");
        dtext(social_x - 14, top + height - 28, COLOR_BLACK, "Qsoc");
        if(show_gap) dtext(left + 148, top + 18, COLOR_HIGHLIGHT, positive ? "Ext ben" : "Ext cost");
        if(show_policy) dtext((market_x + social_x) / 2 - 12, top + 28, COLOR_GOOD, positive ? "Sub" : "Tax");
    }
    if(options->show_points) {
        draw_point_annotation(market_x, market_y, "Qm", COLOR_BLACK, true);
        draw_point_annotation(social_x, social_y, "Qsoc", COLOR_BLACK, true);
    }
    if(options->show_concepts) {
        if(positive) {
            draw_concept_annotation(left + 40, top + 18, "Underproduction", COLOR_WARN, true);
            draw_concept_annotation(left + 126, top + 108, "Socially optimal output", COLOR_GOOD, true);
        }
        else {
            draw_concept_annotation(left + 44, top + 18, "Overproduction", COLOR_WARN, true);
            draw_concept_annotation(left + 94, top + 108, "Socially optimal output", COLOR_GOOD, true);
        }
    }
}

static void draw_public_goods_common_resources(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int gap = 10;
    int box_w = (width - gap) / 2;
    int left_box = left + 4;
    int right_box = left + box_w + gap + 4;
    bool hi_public = focus_is(focus, "public-good") || focus_is(focus, "free-rider") || focus_is(focus, "free-rider-problem");
    bool hi_common = focus_is(focus, "common-resource") || focus_is(focus, "tragedy-of-the-commons");

    ui_draw_panel(left_box, top + 10, box_w - 6, height - 20, hi_public ? COLOR_ACCENT_SOFT : COLOR_WHITE, COLOR_LINE);
    ui_draw_panel(right_box, top + 10, box_w - 6, height - 20, hi_common ? COLOR_PANEL : COLOR_WHITE, COLOR_LINE);

    if(options->show_labels) {
        dtext(left_box + 8, top + 18, COLOR_ACCENT, "Public");
        dtext(left_box + 8, top + 34, COLOR_ACCENT, "Non-rival");
        dtext(left_box + 8, top + 48, COLOR_ACCENT, "Non-excludable");
        dtext(left_box + 8, top + 72, pick_color(focus_is(focus, "free-rider"), COLOR_WARN), "Free rider");
        dtext(left_box + 8, top + 88, pick_color(focus_is(focus, "free-rider-problem"), COLOR_WARN), "Underprovide");

        dtext(right_box + 8, top + 18, COLOR_GOOD, "Commons");
        dtext(right_box + 8, top + 34, COLOR_GOOD, "Rival");
        dtext(right_box + 8, top + 48, COLOR_GOOD, "Non-excludable");
        dtext(right_box + 8, top + 72, pick_color(focus_is(focus, "common-resource"), COLOR_BLACK), "Open access");
        dtext(right_box + 8, top + 88, pick_color(focus_is(focus, "tragedy-of-the-commons"), COLOR_WARN), "Overuse");
    }

    dline(left_box + 92, top + 28, left_box + 92, top + 92, COLOR_BLACK);
    dline(left_box + 92, top + 92, left_box + box_w - 18, top + 92, COLOR_BLACK);
    dline(left_box + 96, top + 84, left_box + box_w - 24, top + 48, COLOR_ACCENT);
    dline(left_box + 96, top + 84, left_box + box_w - 24, top + 64, COLOR_WARN);

    dline(right_box + 14, top + 92, right_box + box_w - 18, top + 92, COLOR_BLACK);
    dline(right_box + 20, top + 34, right_box + box_w - 26, top + 84, COLOR_GOOD);
    if(options->show_shift || focus_is(focus, "tragedy-of-the-commons")) draw_dashed_line(right_box + 34, top + 76, right_box + box_w - 30, top + 52, COLOR_WARN);
    if(options->show_shift || focus_is(focus, "free-rider-problem")) draw_dashed_line(left_box + 106, top + 48, left_box + box_w - 24, top + 34, COLOR_GOOD);

    if(options->show_points) {
        draw_point_marker(left_box + box_w - 28, top + 64, pick_color(focus_is(focus, "free-rider"), COLOR_WARN), focus_is(focus, "free-rider"));
        draw_point_marker(right_box + box_w - 28, top + 68, pick_color(focus_is(focus, "tragedy-of-the-commons"), COLOR_WARN), focus_is(focus, "tragedy-of-the-commons"));
    }
    if(options->show_concepts) {
        draw_concept_annotation(left_box + 8, top + 106, "Market underprovides", COLOR_GOOD, true);
        draw_concept_annotation(right_box + 8, top + 106, "Open access overuses", COLOR_WARN, true);
    }
}

static void draw_ppc(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int prev_x = left;
    int prev_y = top + 8;
    int i = 0;
    bool hi_frontier = focus_is(focus, "frontier");
    bool hi_oc = focus_is(focus, "opportunity-cost");
    bool hi_shift = focus_is(focus, "growth-shift");
    draw_axes(left, top, width, height, "Good B", "Good A");
    for(i = 1; i <= 24; i++) {
        double t = (double)i / 24.0;
        int x = left + (int)(width * t);
        int y = top + (int)(height * pow(t, 1.6));
        draw_line_thick(prev_x, prev_y, x, y, pick_color(hi_frontier, COLOR_ACCENT), hi_frontier);
        prev_x = x;
        prev_y = y;
    }
    if(options->show_shift || hi_shift) {
        prev_x = left + 8;
        prev_y = top - 6;
        for(i = 1; i <= 24; i++) {
            double t = (double)i / 24.0;
            int x = left + 8 + (int)(width * t);
            int y = top - 6 + (int)(height * pow(t, 1.5));
            draw_dashed_line(prev_x, prev_y, x, y, pick_color(hi_shift, COLOR_GOOD));
            prev_x = x;
            prev_y = y;
        }
    }
    if(options->show_area || hi_oc) {
        fill_triangle(left + 120, top + 82, left + 156, top + 82, left + 156, top + 58, COLOR_ACCENT_SOFT);
        dline(left + 120, top + 82, left + 156, top + 58, COLOR_HIGHLIGHT);
        dline(left + 120, top + 82, left + 156, top + 82, COLOR_HIGHLIGHT);
    }
    if(options->show_points) {
        draw_point_marker(left + 60, top + 110, pick_color(focus_is(focus, "inefficient"), COLOR_WARN), focus_is(focus, "inefficient"));
        draw_point_marker(left + 120, top + 82, pick_color(focus_is(focus, "efficient"), COLOR_BLACK), focus_is(focus, "efficient"));
        draw_point_marker(left + 182, top + 18, pick_color(focus_is(focus, "unattainable"), COLOR_GOOD), focus_is(focus, "unattainable"));
        draw_point_annotation(left + 60, top + 110, "Inefficient", COLOR_WARN, true);
        draw_point_annotation(left + 120, top + 82, "Efficient", COLOR_BLACK, true);
        draw_point_annotation(left + 182, top + 18, "Unattainable", COLOR_GOOD, true);
    }
    if(options->show_labels) {
        dtext(left + 18, top + 6, COLOR_ACCENT, "PPC");
        dtext(left + 66, top + 102, COLOR_WARN, "In");
        dtext(left + 126, top + 74, COLOR_BLACK, "Ef");
        dtext(left + 188, top + 12, COLOR_GOOD, "Out");
        if(options->show_shift || hi_shift) dtext(left + width - 34, top + 6, COLOR_GOOD, "Growth");
        if(options->show_area || hi_oc) dtext(left + 146, top + 64, COLOR_HIGHLIGHT, "OC");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 24, top + 18, "Productively efficient", COLOR_GOOD, true);
        draw_concept_annotation(left + 138, top + 102, "Trade-off", COLOR_HIGHLIGHT, options->show_area || hi_oc);
    }
}

static void draw_trade_table(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int table_left = left + 18;
    int table_top = top + 18;
    int row_h = 26;
    int col1 = table_left + 64;
    int col2 = table_left + 126;
    int col3 = table_left + 188;
    int right = left + width - 12;
    uint16_t abs_color = focus_is(focus, "absolute-advantage") ? COLOR_ACCENT_SOFT : COLOR_WHITE;
    uint16_t comp_color = focus_is(focus, "comparative-advantage") ? COLOR_PANEL : COLOR_WHITE;
    uint16_t spec_color = focus_is(focus, "specialization") ? COLOR_ACCENT_SOFT : COLOR_WHITE;
    uint16_t gain_color = (options->show_area || focus_is(focus, "gains-from-trade")) ? COLOR_PANEL : COLOR_WHITE;

    ui_draw_panel(table_left, table_top, width - 30, height - 34, COLOR_WHITE, COLOR_LINE);
    dline(col1, table_top, col1, table_top + row_h * 5, COLOR_BLACK);
    dline(col2, table_top, col2, table_top + row_h * 5, COLOR_BLACK);
    dline(col3, table_top, col3, table_top + row_h * 5, COLOR_BLACK);
    dline(table_left, table_top + row_h, right, table_top + row_h, COLOR_BLACK);
    dline(table_left, table_top + row_h * 2, right, table_top + row_h * 2, COLOR_BLACK);
    dline(table_left, table_top + row_h * 3, right, table_top + row_h * 3, COLOR_BLACK);
    dline(table_left, table_top + row_h * 4, right, table_top + row_h * 4, COLOR_BLACK);

    drect(table_left + 1, table_top + row_h + 1, right - 1, table_top + row_h * 2 - 1, abs_color);
    drect(table_left + 1, table_top + row_h * 2 + 1, right - 1, table_top + row_h * 3 - 1, comp_color);
    drect(table_left + 1, table_top + row_h * 3 + 1, right - 1, table_top + row_h * 4 - 1, spec_color);
    drect(table_left + 1, table_top + row_h * 4 + 1, right - 1, table_top + row_h * 5 - 1, gain_color);

    if(options->show_labels) {
        dtext(table_left + 6, table_top + 8, COLOR_MUTED, "Prod");
        dtext(col1 + 8, table_top + 8, COLOR_MUTED, "A");
        dtext(col2 + 8, table_top + 8, COLOR_MUTED, "B");
        dtext(col3 + 6, table_top + 8, COLOR_MUTED, "Result");

        dtext(table_left + 6, table_top + row_h + 8, COLOR_TEXT, "Abs");
        dtext(col1 + 8, table_top + row_h + 8, COLOR_TEXT, "10/6");
        dtext(col2 + 8, table_top + row_h + 8, COLOR_TEXT, "8/4");
        dtext(col3 + 6, table_top + row_h + 8, COLOR_TEXT, "More output");

        dtext(table_left + 6, table_top + row_h * 2 + 8, COLOR_TEXT, "Comp");
        dtext(col1 + 8, table_top + row_h * 2 + 8, COLOR_TEXT, "1A=0.6B");
        dtext(col2 + 8, table_top + row_h * 2 + 8, COLOR_TEXT, "1A=2B");
        dtext(col3 + 6, table_top + row_h * 2 + 8, COLOR_TEXT, "Lower OC");

        dtext(table_left + 6, table_top + row_h * 3 + 8, COLOR_TEXT, "Spec");
        dtext(col1 + 8, table_top + row_h * 3 + 8, COLOR_TEXT, "Prod A");
        dtext(col2 + 8, table_top + row_h * 3 + 8, COLOR_TEXT, "Prod B");
        dtext(col3 + 6, table_top + row_h * 3 + 8, COLOR_TEXT, "Trade");

        dtext(table_left + 6, table_top + row_h * 4 + 8, COLOR_TEXT, "Gain");
        dtext(col1 + 8, table_top + row_h * 4 + 8, COLOR_TEXT, "Both");
        dtext(col2 + 8, table_top + row_h * 4 + 8, COLOR_TEXT, "gain");
        dtext(col3 + 6, table_top + row_h * 4 + 8, COLOR_TEXT, "ToT ok");
    }

    if(focus_is(focus, "specialization")) {
        dline(col1 + 6, table_top + row_h * 3 + 4, col1 + 46, table_top + row_h * 3 + 4, COLOR_GOOD);
        dline(col2 + 6, table_top + row_h * 3 + 20, col2 + 46, table_top + row_h * 3 + 20, COLOR_GOOD);
    }
    if(options->show_shift || focus_is(focus, "gains-from-trade")) {
        dline(col3 + 8, table_top + row_h * 4 + 18, right - 12, table_top + row_h * 4 + 18, COLOR_HIGHLIGHT);
        dline(right - 12, table_top + row_h * 4 + 18, right - 18, table_top + row_h * 4 + 14, COLOR_HIGHLIGHT);
        dline(right - 12, table_top + row_h * 4 + 18, right - 18, table_top + row_h * 4 + 22, COLOR_HIGHLIGHT);
    }
    if(options->show_concepts) {
        dtext(col3 + 4, table_top + row_h * 2 + 20, COLOR_GOOD, "Specialize by lower OC");
    }
}

static void draw_production_function(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int prev_x = left + 8;
    int prev_y = top + height - 12;
    int i = 0;
    draw_axes(left, top, width, height, "Labor", "Output");
    for(i = 1; i <= 24; i++) {
        double t = (double)i / 24.0;
        int x = left + 8 + (int)((width - 18) * t);
        int y = top + height - 12 - (int)((height - 28) * (1.0 - exp(-2.4 * t)));
        draw_line_thick(prev_x, prev_y, x, y, pick_color(focus_is(focus, "tp"), COLOR_ACCENT), focus_is(focus, "tp"));
        prev_x = x;
        prev_y = y;
    }
    draw_line_thick(left + 10, top + height - 18, left + 52, top + 88, pick_color(focus_is(focus, "mp"), COLOR_WARN), focus_is(focus, "mp"));
    draw_line_thick(left + 52, top + 88, left + 88, top + 56, pick_color(focus_is(focus, "mp"), COLOR_WARN), focus_is(focus, "mp"));
    draw_line_thick(left + 88, top + 56, left + 126, top + 74, pick_color(focus_is(focus, "mp"), COLOR_WARN), focus_is(focus, "mp"));
    draw_line_thick(left + 126, top + 74, left + 168, top + 112, pick_color(focus_is(focus, "mp"), COLOR_WARN), focus_is(focus, "mp"));
    if(options->show_points) {
        draw_point_marker(left + 126, top + 74, pick_color(focus_is(focus, "diminishing-returns"), COLOR_BLACK), focus_is(focus, "diminishing-returns"));
        draw_point_annotation(left + 126, top + 74, "Diminishing returns", COLOR_BLACK, true);
    }
    if(options->show_shift) draw_dashed_line(left + 8, top + height - 28, left + width - 8, top + 20, COLOR_GOOD);
    if(options->show_labels) {
        dtext(left + width - 32, top + 14, COLOR_ACCENT, "TP");
        dtext(left + 130, top + 58, COLOR_WARN, "MP");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 34, top + 18, "MP eventually falls", COLOR_GOOD, true);
    }
}

static void draw_lorenz(int left, int top, int width, int height, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    int prev_x = left;
    int prev_y = top + height;
    int i = 0;
    draw_axes(left, top, width, height, "Pop", "Income");
    dline(left, top + height, left + width, top, pick_color(focus_is(focus, "equality"), COLOR_MUTED));
    for(i = 1; i <= 24; i++) {
        double t = (double)i / 24.0;
        int x = left + (int)(width * t);
        int y = top + height - (int)(height * pow(t, 2.2));
        draw_line_thick(prev_x, prev_y, x, y, pick_color(focus_is(focus, "lorenz"), COLOR_ACCENT), focus_is(focus, "lorenz"));
        if(options->show_area || focus_is(focus, "gini-area")) fill_triangle(left, top + height, x, y, x, top + height - (int)(height * t), COLOR_ACCENT_SOFT);
        prev_x = x;
        prev_y = y;
    }
    if(options->show_labels) {
        dtext(left + width - 50, top + 10, COLOR_MUTED, "Equality");
        dtext(left + width - 38, top + 58, COLOR_ACCENT, "Lorenz");
    }
    if(options->show_concepts) {
        draw_concept_annotation(left + 34, top + 18, "More bowed = more unequal", COLOR_WARN, true);
        draw_concept_annotation(left + 86, top + 94, "Gini area", COLOR_ACCENT, options->show_area || focus_is(focus, "gini-area"));
    }
}

static void draw_graph_in_rect(const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options, int panel_x, int panel_y, int panel_w, int panel_h, bool include_info)
{
    int left = panel_x + 18;
    int top = panel_y + 12;
    int width = panel_w - 36;
    int height = panel_h - 24;
    GraphRenderOptions safe = {true, true, false, false, false, true};
    bool show_panel = false;
    bool compact = panel_w < 220 || panel_h < 130;

    if(!graph) return;
    if(options) safe = *options;
    if(panel_w < 80 || panel_h < 60) return;

    if(include_info && safe.show_info && panel_w >= 220) {
        int info_w = 112;
        show_panel = true;
        width = panel_w - info_w - 54;
    }
    if(width < 96) width = panel_w - 28;
    if(width < 64) width = 64;
    if(height < 56) height = 56;
    if(compact) {
        left = panel_x + 12;
        top = panel_y + 10;
    }

    ui_draw_panel(panel_x, panel_y, panel_w, panel_h, COLOR_WHITE, COLOR_LINE);

    if(strcmp(graph->id, "supply-demand") == 0 || strcmp(graph->id, "surplus-welfare") == 0) {
        draw_supply_demand_base(left, top, width, height, focus, &safe, strcmp(graph->id, "surplus-welfare") == 0, false, false, false, false);
    }
    else if(strcmp(graph->id, "tax") == 0) draw_supply_demand_base(left, top, width, height, focus, &safe, false, true, false, false, false);
    else if(strcmp(graph->id, "subsidy") == 0) draw_supply_demand_base(left, top, width, height, focus, &safe, false, false, true, false, false);
    else if(strcmp(graph->id, "price-ceiling") == 0) draw_supply_demand_base(left, top, width, height, focus, &safe, false, false, false, true, false);
    else if(strcmp(graph->id, "price-floor") == 0) draw_supply_demand_base(left, top, width, height, focus, &safe, false, false, false, false, true);
    else if(strcmp(graph->id, "cost-curves") == 0) draw_cost_family(left, top, width, height, focus, &safe, false);
    else if(strcmp(graph->id, "perfect-competition") == 0) draw_cost_family(left, top, width, height, focus, &safe, true);
    else if(strcmp(graph->id, "perfect-competition-market") == 0) draw_supply_demand_base(left, top, width, height, focus, &safe, false, false, false, false, false);
    else if(strcmp(graph->id, "monopoly") == 0) draw_monopoly_family(left, top, width, height, focus, &safe, false);
    else if(strcmp(graph->id, "monopolistic-competition") == 0) draw_monopoly_family(left, top, width, height, focus, &safe, true);
    else if(strcmp(graph->id, "kinked-demand") == 0) draw_kinked_demand(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "game-theory-matrix") == 0) draw_game_matrix(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "monopsony") == 0) draw_labor_family(left, top, width, height, focus, &safe, true);
    else if(strcmp(graph->id, "production-function") == 0) draw_production_function(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "labor-market") == 0) draw_labor_family(left, top, width, height, focus, &safe, false);
    else if(strcmp(graph->id, "hiring-rule") == 0) draw_hiring_rule(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "negative-externality") == 0) draw_externality_family(left, top, width, height, focus, &safe, false);
    else if(strcmp(graph->id, "positive-externality") == 0) draw_externality_family(left, top, width, height, focus, &safe, true);
    else if(strcmp(graph->id, "public-goods-common-resources") == 0) draw_public_goods_common_resources(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "ppc") == 0) draw_ppc(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "trade-table") == 0) draw_trade_table(left, top, width, height, focus, &safe);
    else if(strcmp(graph->id, "lorenz") == 0) draw_lorenz(left, top, width, height, focus, &safe);
    else {
        draw_axes(left, top, width, height, "Q", "P");
        dtext(left + 12, top + height / 2, COLOR_TEXT, "No specialized renderer");
    }

    if(show_panel) {
        draw_info_panel_at(panel_x + panel_w - 120, panel_y + 8, 112, panel_h - 16, graph, focus, &safe);
    }
}

void graphs_draw_diagram(const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options)
{
    if(options && !options->show_info) {
        draw_graph_in_rect(graph, focus, options, 12, 28, SCREEN_W - 24, 110, false);
        return;
    }
    draw_graph_in_rect(graph, focus, options, 20, 30, 240, 152, true);
}

void graphs_draw_preview(const GraphEntry *graph, const GraphElementEntry *focus, const GraphRenderOptions *options, int x, int y, int w, int h)
{
    GraphRenderOptions safe = {true, false, false, false, false, false};
    if(options) safe = *options;
    safe.show_labels = safe.show_labels && (w >= 176 && h >= 104);
    safe.show_points = safe.show_points && (w >= 176 && h >= 104);
    safe.show_concepts = false;
    safe.show_shift = false;
    safe.show_info = false;
    draw_graph_in_rect(graph, focus, &safe, x, y, w, h, false);
}
