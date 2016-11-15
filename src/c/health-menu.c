#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "logging.h"

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

static void health_handler(HealthEventType event, void *context) {
    log_func();
    if (event != HealthEventHeartRateUpdate) {
        HealthActivityMask mask = health_service_peek_current_activities();

        bool sleeping = (mask & HealthActivitySleep) || (mask & HealthActivityRestfulSleep);
        text_layer_set_text(s_text_layer, sleeping ? "Asleep" : "Awake");
    }
}

static void window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    s_status_bar_layer = status_bar_layer_create();
    layer_add_child(root_layer, status_bar_layer_get_layer(s_status_bar_layer));

    s_text_layer = text_layer_create(GRect(bounds.origin.x, bounds.origin.y - 2, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    text_layer_set_text_color(s_text_layer, GColorWhite);
    text_layer_set_background_color(s_text_layer, GColorClear);
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(root_layer, text_layer_get_layer(s_text_layer));

    health_service_events_subscribe(health_handler, NULL);
}

static void window_unload(Window *window) {
    log_func();
    health_service_events_unsubscribe();

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
