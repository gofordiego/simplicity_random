#include "pebble.h"

Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
Layer *line_layer;

// TextLayer *debug_layer;

#define MAX_MIN_FORWARD 7 // Maximum number of minutes to forward time

static int min_forward = 0; // Random number of minutes used to forward time
static int min_forward_countdown = 0; // Minute countdown before fetching a new random number
static time_t forward_time = 0; // Used to override the default tick_time

/**
 * Updates forward_time with the current time forwarded to a random number of minutes ahead.
 * This random number is kept for a small time interval, before generating a new random number.
 */
void update_forward_time() {
  static char debug_text[] = "00, -00";

  if (min_forward_countdown > 1) {
    min_forward_countdown--;
  } else {
    min_forward = rand() % (MAX_MIN_FORWARD + 1);
    min_forward_countdown = MAX_MIN_FORWARD / 2;
  }

  forward_time = time(NULL) + (min_forward * 60);

//  snprintf(debug_text, sizeof(debug_text), "%d, -%d", min_forward, min_forward_countdown);
//  text_layer_set_text(debug_layer, debug_text);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";
  char *time_format;

  // Load forward time on tick_time
  update_forward_time();
  tick_time = localtime(&forward_time);

  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);

  text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  GRect line_frame = GRect(8, 97, 139, 1);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);
/*
  debug_layer = text_layer_create(GRect(29, 14, 144-40, 168-54));
  text_layer_set_text_color(debug_layer, GColorWhite);
  text_layer_set_background_color(debug_layer, GColorClear);
  text_layer_set_font(debug_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(debug_layer));
*/
  // Seeds the random number generator
  srand(time(NULL));

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}


int main(void) {
  handle_init();

  app_event_loop();

  handle_deinit();
}
