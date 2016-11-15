#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "logging.h"

static const char *BITS[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

static const char *EVENTS[5] = {
    [0] = "Significant", [1] = "Movement", [2] = "Sleep",
    [3] = "Alert", [4] = "Heart Rate"
};

typedef struct {
    time_t t;
    HealthEventType e;
    HealthActivityMask m;
} Data;

static LinkedRoot *s_list_root;
static uint16_t s_list_size;

static Window *s_window;
static StatusBarLayer *s_status_bar_layer;
static TextLayer *s_text_layer;
static MenuLayer *s_menu_layer;

static void health_handler(HealthEventType event, void *context) {
    log_func();
    if (event != HealthEventHeartRateUpdate) {
        Data *data = malloc(sizeof(Data));
        if (data) {
            data->t = time(NULL);
            data->e = event;
            data->m = health_service_peek_current_activities();
            linked_list_append(s_list_root, data);
            s_list_size += 1;
            menu_layer_reload_data(s_menu_layer);
            menu_layer_set_selected_index(s_menu_layer, (MenuIndex) {
                .section = 0,
                .row = s_list_size
            }, MenuRowAlignBottom, false);

            bool sleeping = (data->m & HealthActivitySleep) || (data->m & HealthActivityRestfulSleep);
            text_layer_set_text(s_text_layer, sleeping ? "Asleep" : "Awake");
        } else {
            Data *data = (Data *) linked_list_get(s_list_root, 0);
            free(data);
            linked_list_remove(s_list_root, 0);
            health_handler(event, context);
        }
    }
}

static uint16_t menu_num_rows_callback(MenuLayer *this, uint16_t section_index, void *context) {
    log_func();
    return s_list_size;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *this, MenuIndex *index, void *context) {
    log_func();
    Data *data = (Data *) linked_list_get(s_list_root, index->row);

    char title[16];
    snprintf(title, sizeof(title), "%s", EVENTS[data->e]);

    char stime[16];
    strftime(stime, sizeof(stime), "%m-%d %H:%M:%S", localtime(&data->t));
    char subtitle[32];
    snprintf(subtitle, sizeof(subtitle), "%s - %s", stime, BITS[data->m & 0x0F]);

    menu_cell_basic_draw(ctx, this, title, subtitle, NULL);
}

static void window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    s_status_bar_layer = status_bar_layer_create();
    status_bar_layer_set_colors(s_status_bar_layer, GColorWhite, GColorBlack);
    status_bar_layer_set_separator_mode(s_status_bar_layer,StatusBarLayerSeparatorModeDotted);
    layer_add_child(root_layer, status_bar_layer_get_layer(s_status_bar_layer));

    s_text_layer = text_layer_create(GRect(2, -2, bounds.size.w - 2, STATUS_BAR_LAYER_HEIGHT));
    text_layer_set_text_color(s_text_layer, status_bar_layer_get_foreground_color(s_status_bar_layer));
    text_layer_set_background_color(s_text_layer, GColorClear);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(root_layer, text_layer_get_layer(s_text_layer));

    s_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = menu_num_rows_callback,
        .draw_row = menu_draw_row_callback
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(root_layer, menu_layer_get_layer(s_menu_layer));

    health_service_events_subscribe(health_handler, NULL);
}

static void window_unload(Window *window) {
    log_func();
    health_service_events_unsubscribe();

    menu_layer_destroy(s_menu_layer);
    text_layer_destroy(s_text_layer);
    status_bar_layer_destroy(s_status_bar_layer);
}

static void init(void) {
    log_func();
    s_list_root = linked_list_create_root();

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}

static bool list_destroy_callback(void *object, void *context) {
    log_func();
    free(object);
    return true;
}

static void deinit(void) {
    log_func();
    window_destroy(s_window);

    linked_list_foreach(s_list_root, list_destroy_callback, NULL);
    linked_list_clear(s_list_root);
    free(s_list_root);
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
