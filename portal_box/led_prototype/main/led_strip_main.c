/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"

static const char *TAG = "example";

#define RMT_TX_CHANNEL RMT_CHANNEL_0

#define STRIP_LED_NUM 60

#define EXAMPLE_CHASE_SPEED_MS (10)

//void led_pattern_authorized(void* p);
static led_strip_t *strip;

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

// set all leds to be one color
// adjust overall hsv values here?
void led_set_all(uint32_t r, uint32_t g, uint32_t b){
    uint32_t hue = 100;
    uint32_t sat = 100;
    uint32_t val = 60;
    for (int j = 0; j < STRIP_LED_NUM; j ++) {
        // Build RGB values
        //hue = j * 360 / STRIP_LED_NUM + 2;
        led_strip_hsv2rgb(hue, sat, val, &r, &g, &b);
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(strip->set_pixel(strip, j, r, g, b));
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
}

// pattern to display when the status is authorized/passed
// all green, then do a rainbow pattern
void led_pattern_authorized() {
    ESP_LOGI(TAG, "Status: Authorized");

    uint32_t red = 0;
    uint32_t green = 255;
    uint32_t blue = 0;
    led_set_all(red, green, blue);


    uint32_t hue = 0;
    uint32_t s_rgb = 0;
    green = 0;

    strip->clear(strip, 50);
    int cntr = 0;
    while (cntr < 20) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < STRIP_LED_NUM; j += 3) {
                // Build RGB values
                hue = j * 360 / STRIP_LED_NUM + s_rgb;
                led_strip_hsv2rgb(hue, 100, 60, &red, &green, &blue);
                // Write RGB values to strip driver
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(strip->refresh(strip, 100));
            vTaskDelay(pdMS_TO_TICKS(100));
            strip->clear(strip, 50);
            //vTaskDelay(pdMS_TO_TICKS(100));
        }
        s_rgb += 60;
        cntr++;
    }
    red = 255;
    green = 255;
    blue = 255;
    led_set_all(red, green, blue);
    ESP_LOGI(TAG, "GOING 1");

    
}


// refractored, might use it later
void led_rainbow_chase() {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    while (true) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < STRIP_LED_NUM; j += 3) {
                // Build RGB values
                hue = j * 360 / STRIP_LED_NUM + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                // Write RGB values to strip driver
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(strip->refresh(strip, 100));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            strip->clear(strip, 50);
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }

}
void app_main(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(STRIP_LED_NUM, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    led_pattern_authorized();
    // Show simple rainbow chasing pattern
   // led_rainbow_chase();
    
}
