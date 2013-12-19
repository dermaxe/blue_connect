#include "pebble.h"
#include "num2words.h"
	
#define BUFFER_SIZE 86

static TextLayer *bluetoothstatus_layer;
static TextLayer *batterystatus_layer;

uint8_t pebble_batteryPercent;

static struct CommonWordsData {
  TextLayer *label;
  Window *window;
  char buffer[BUFFER_SIZE];
} s_data;

static void update_time(struct tm* t) {
  fuzzy_time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE);
  text_layer_set_text(s_data.label, s_data.buffer);
}

static const VibePattern custom_pattern_notconnected = {
  .durations = (uint32_t []) {100, 300, 300, 300, 100, 300, 300, 300},
  .num_segments = 8
};

static const VibePattern custom_pattern_connected = {
  .durations = (uint32_t []) {100, 300, 100, 300},
  .num_segments = 4
};

static void update_bluetooth(bool connected) {
  if (bluetooth_connection_service_peek()) {
	  vibes_enqueue_custom_pattern(custom_pattern_connected);
      text_layer_set_text(bluetoothstatus_layer,"Phone is connected!");
	  APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected!");
  }
  else {
      vibes_enqueue_custom_pattern(custom_pattern_notconnected);
	  text_layer_set_text(bluetoothstatus_layer, "Phone is not connected!");
	  APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected!");
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void bluetooth_connection_callback(bool connected) {
  APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth connected=%d", (int) connected);
  update_bluetooth(connected);
}

static void update_pebble_battery(BatteryChargeState pb_bat) {
  pebble_batteryPercent = pb_bat.charge_percent; 
  static char temp[20];
  //snprintf(target_string, size_of_target_string_in_bytes, "%d", source_int)
  snprintf(temp, sizeof(temp), "battery charge=%u%lc", pebble_batteryPercent, 0x0025);
  
  text_layer_set_text(batterystatus_layer, temp);
}

static void pebble_battery_callback(BatteryChargeState pb_bat) {
  pebble_batteryPercent = pb_bat.charge_percent;  
  APP_LOG(APP_LOG_LEVEL_INFO, "battery charge=%d", pebble_batteryPercent);
  update_pebble_battery(pb_bat);
}



static void do_init(void) {
  s_data.window = window_create();
  const bool animated = true;
  window_stack_push(s_data.window, animated);

  window_set_background_color(s_data.window, GColorBlack);
  GFont font = fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD);

  Layer *root_layer = window_get_root_layer(s_data.window);
  GRect frame = layer_get_frame(root_layer);

  s_data.label = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
  text_layer_set_background_color(s_data.label, GColorBlack);
  text_layer_set_text_color(s_data.label, GColorWhite);
  text_layer_set_font(s_data.label, font);
  text_layer_set_text_alignment(s_data.label, GTextAlignmentLeft);
  layer_add_child(root_layer, text_layer_get_layer(s_data.label));

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_minute_tick);


  bluetoothstatus_layer = text_layer_create(GRect(0, 100, frame.size.w, frame.size.h));
  text_layer_set_text_color(bluetoothstatus_layer, GColorWhite);
  text_layer_set_background_color(bluetoothstatus_layer, GColorBlack);
  text_layer_set_font(bluetoothstatus_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(bluetoothstatus_layer, GTextAlignmentCenter);	
  layer_add_child(root_layer, text_layer_get_layer(bluetoothstatus_layer));
  
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);	
 
  update_bluetooth(bluetooth_connection_service_peek());
	
  batterystatus_layer = text_layer_create(GRect(0, 120, frame.size.w, frame.size.h));
  text_layer_set_text_color(batterystatus_layer, GColorWhite);
  text_layer_set_background_color(batterystatus_layer, GColorBlack);
  text_layer_set_font(batterystatus_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(batterystatus_layer, GTextAlignmentCenter);	
  layer_add_child(root_layer, text_layer_get_layer(batterystatus_layer));
  
  BatteryChargeState pb_bat = battery_state_service_peek();
  pebble_batteryPercent = pb_bat.charge_percent;  
  battery_state_service_subscribe(pebble_battery_callback);
  update_pebble_battery(battery_state_service_peek());
}

static void do_deinit(void) {
  window_destroy(s_data.window);
  text_layer_destroy(s_data.label);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}