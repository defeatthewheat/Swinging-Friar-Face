/*
* The Swinging Friar watch face by defeatthewheat
* Based around the Simple Analog watch face
*	Last Edit: 3/6/2015
* Version 1.0
*/

#include <pebble.h>
#include "main_analog.h"

#include "pebble.h"

#include "string.h"
#include "stdlib.h"

Layer *simple_bg_layer;

Layer *date_layer;
TextLayer *day_label;
char day_buffer[6];
TextLayer *num_label;
char num_buffer[4];

static BitmapLayer *s_background_layer; 
static GBitmap *s_background_bitmap;

static RotBitmapLayer *s_friar_arm; 
static GBitmap *s_friar_arm_bitmap;

static GPath *minute_arrow;
static GPath *hour_arrow;
static GPath *tick_paths[NUM_CLOCK_TICKS];
Layer *hands_layer;
Window *window;
int offset = 0;

static void bg_update_proc(Layer *layer, GContext *ctx) {
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
	
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, tick_paths[i]);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
	GRect r;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
	
 	//Rotating hour hand
  gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, hour_arrow);
  gpath_draw_outline(ctx, hour_arrow);
		
	
	//Offsets depending on minute hand
	if((t->tm_min < 15) || (t->tm_min >=58)){
		offset = 16;
	}else if(t->tm_min >= 45){
		offset = 15;
	}else{
		offset = 14;
	}
	
	int32_t minuteAngle = (t->tm_min-offset) * TRIG_MAX_ANGLE / 60;
	//int32_t minuteAngle = 15 * TRIG_MAX_ANGLE / 60; //test values
	
	//Fixed rotational problem with this offsetting
	r = layer_get_frame((Layer *)s_friar_arm);
	r.origin.x = 72 - r.size.w/2 + 57 * cos_lookup((minuteAngle + 3 * TRIG_MAX_ANGLE / 4)%TRIG_MAX_ANGLE) / TRIG_MAX_RATIO;
	r.origin.y = 84 - r.size.h/2 + 57 * sin_lookup((minuteAngle + 3 * TRIG_MAX_ANGLE / 4)%TRIG_MAX_ANGLE) / TRIG_MAX_RATIO;
	layer_set_frame((Layer *)s_friar_arm, r);
	rot_bitmap_layer_set_angle(s_friar_arm, minuteAngle);
	
	//Friar Minute Hand
	//rot_bitmap_layer_set_angle(s_friar_arm, TRIG_MAX_ANGLE * ((t->tm_min)-offset) / 60 );
	//rot_bitmap_layer_set_angle(s_friar_arm, (TRIG_MAX_ANGLE * 15)/ 60 );
	
  // dot in center
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(day_buffer, sizeof(day_buffer), "%a", t);
  text_layer_set_text(day_label, day_buffer);

  strftime(num_buffer, sizeof(num_buffer), "%d", t);
  text_layer_set_text(num_label, num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // init layers
  simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, simple_bg_layer);
	
	
	// Background
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACK_MOVED);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	
	
	
  // init date layer -> a plain parent layer to create a date update proc
  date_layer = layer_create(bounds);
  layer_set_update_proc(date_layer, date_update_proc);
  layer_add_child(window_layer, date_layer);

  // init day
  day_label = text_layer_create(GRect(1, 131, 47, 30));
  text_layer_set_text(day_label, day_buffer);
  text_layer_set_background_color(day_label, GColorClear);
  text_layer_set_text_color(day_label, GColorWhite);
 	GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
	
	//GFont knuckle_d = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_KNUCKLE_DOWN_18)); a new font will be in an update.
	
  text_layer_set_font(day_label, norm18);

  layer_add_child(date_layer, text_layer_get_layer(day_label));

  // init month num
  num_label = text_layer_create(GRect(120, 129, 47, 40));

  text_layer_set_text(num_label, num_buffer);
  text_layer_set_background_color(num_label, GColorClear);
  text_layer_set_text_color(num_label, GColorWhite);
  GFont bold18 = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  text_layer_set_font(num_label, bold18);

  layer_add_child(date_layer, text_layer_get_layer(num_label));

	// Create Friar Minute Hand
	
	s_friar_arm = rot_bitmap_layer_create(s_friar_arm_bitmap);
	rot_bitmap_set_src_ic(s_friar_arm, GPoint(0,-48));
	
	rot_bitmap_set_compositing_mode(s_friar_arm, GCompOpClear);
	layer_add_child(window_layer, (Layer *) s_friar_arm);
	
  // init hands
  hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, hands_update_proc);
  layer_add_child(window_layer, hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(simple_bg_layer);
  layer_destroy(date_layer);
  text_layer_destroy(day_label);
  text_layer_destroy(num_label);
  layer_destroy(hands_layer);
	rot_bitmap_layer_destroy(s_friar_arm);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  day_buffer[0] = '\0';
  num_buffer[0] = '\0';

	
	
  // init hand paths
  minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = grect_center_point(&bounds);
  //gpath_move_to(minute_arrow, center);
 	
	//rot_bitmap_set_src_ic(s_friar_arm, center);
	s_friar_arm_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WHITEBAT2);
	gpath_move_to(hour_arrow,GPoint(center.x, center.y+8));

	
  // init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit(void) {
  gpath_destroy(minute_arrow);
  gpath_destroy(hour_arrow);
	gbitmap_destroy(s_friar_arm_bitmap);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
