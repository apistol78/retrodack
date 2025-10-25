#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lvgl.h>

#include <HAL/Interrupt.h>
#include <HAL/SPI.h>
#include <HAL/Sprite.h>

#include "Runtime/Audio.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Input.h"
#include "Runtime/Runtime.h"
#include "Runtime/Timer.h"
#include "Runtime/Kernel.h"
#include "Runtime/Video.h"

#include "Firmware/Cursor.h"

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (720 * 720 / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

	uint32_t* fb = (uint32_t*)rt_video_get_primary_target();

	fb += area->x1 + area->y1 * 720;

	uint32_t* px = (uint32_t*)px_map;
	for (uint32_t y = 0; y < h; ++y)
	{
		for (uint32_t x = 0; x < w; ++x)
		{
			fb[x] = *px++;
		}
		fb += 720;
	}

    lv_display_flush_ready(disp);
}

void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
	int32_t pos[2];
	rt_input_get_absolute_position(pos);

	data->point.x = pos[0];
	data->point.y = pos[1];

	if ((rt_input_get_state() & RT_INPUT_TB) != 0)
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED;

    /*For example  ("my_..." functions needs to be implemented by you)
    int32_t x, y;
    bool touched = my_get_touch( &x, &y );

    if(!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;

        data->point.x = x;
        data->point.y = y;
    }
     */
}

static uint32_t my_tick(void)
{
    return rt_timer_get_ms();
}


static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
}


int main()
{
	runtime_init();

    rt_video_set_mode(VMODE_720_720_32);
	rt_video_set_palette(2, 0xffffff);
	rt_video_set_palette(3, 0x000000);

	hal_sprite_set_visible(0, 0xff);
	hal_sprite_set_bits(0, c_mouseCursor, 32, 32);
	rt_input_set_hotspot(16, 16);

	lv_init();
	lv_tick_set_cb(my_tick);


    lv_display_t * disp = lv_display_create(720, 720);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);


    lv_obj_t * btn1 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
    lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_t * label = lv_label_create(btn1);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);
	

	for (;;)
	{
        lv_timer_handler();
        rt_kernel_sleep(1);
	}


	return 0;
}
