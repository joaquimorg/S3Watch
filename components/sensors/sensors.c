// QMI8658-based step counting and activity classification with raise-to-wake

#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "qmi8658.h"
#include "display_manager.h"
#include <math.h>
#include <time.h>
#include "esp_timer.h"

#define IMU_IRQ_GPIO           GPIO_NUM_21
#define IMU_ADDR_HIGH          QMI8658_ADDRESS_HIGH
#define IMU_ADDR_LOW           QMI8658_ADDRESS_LOW

static const char *TAG = "SENSORS";

static qmi8658_dev_t s_imu;
static bool s_imu_ready = false;
static volatile uint32_t s_step_count = 0; // daily steps
static sensors_activity_t s_activity = SENSORS_ACTIVITY_IDLE;
static SemaphoreHandle_t s_wom_sem = NULL; // wake-on-motion semaphore
static time_t s_last_midnight = 0;

static time_t get_midnight_epoch(time_t now)
{
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    tm_now.tm_hour = 0; tm_now.tm_min = 0; tm_now.tm_sec = 0;
    return mktime(&tm_now);
}

static void maybe_reset_daily_counter(void)
{
    time_t now = time(NULL);
    if (s_last_midnight == 0) {
        s_last_midnight = get_midnight_epoch(now);
    }
    time_t midnight_now = get_midnight_epoch(now);
    if (midnight_now > s_last_midnight) {
        s_last_midnight = midnight_now;
        s_step_count = 0;
        ESP_LOGI(TAG, "Daily step counter reset at midnight");
    }
}

static void IRAM_ATTR imu_irq_isr(void* arg)
{
    BaseType_t hp = pdFALSE;
    if (s_wom_sem) {
        xSemaphoreGiveFromISR(s_wom_sem, &hp);
    }
    if (hp) portYIELD_FROM_ISR();
}

static esp_err_t imu_setup_irq(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << IMU_IRQ_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    esp_err_t r = gpio_install_isr_service(0);
    if (r != ESP_OK && r != ESP_ERR_INVALID_STATE) {
        return r;
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(IMU_IRQ_GPIO, imu_irq_isr, NULL));
    return ESP_OK;
}

static bool imu_try_init_with_addr(uint8_t addr)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) return false;
    if (qmi8658_init(&s_imu, bus, addr) != ESP_OK) return false;
    // Configure for low-power step counting: accel only, ~62.5 Hz, 4g
    (void)qmi8658_enable_sensors(&s_imu, QMI8658_DISABLE_ALL);
    (void)qmi8658_set_accel_range(&s_imu, QMI8658_ACCEL_RANGE_4G);
    (void)qmi8658_set_accel_odr(&s_imu, QMI8658_ACCEL_ODR_62_5HZ);
    (void)qmi8658_enable_accel(&s_imu, true);
    qmi8658_set_accel_unit_mg(&s_imu, true); // mg units simplify magnitude
    return true;
}

void sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing sensors (QMI8658)");
    if (bsp_i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C not available");
        return;
    }
    // Try both possible I2C addresses
    s_imu_ready = imu_try_init_with_addr(IMU_ADDR_HIGH) || imu_try_init_with_addr(IMU_ADDR_LOW);
    if (!s_imu_ready) {
        ESP_LOGE(TAG, "QMI8658 init failed");
        return;
    }
    // Create semaphore and IRQ for wake-on-motion
    s_wom_sem = xSemaphoreCreateBinary();
    if (s_wom_sem) {
        imu_setup_irq();
        // Configure wake-on-motion threshold (LSB depends on FS/ODR; empirical)
        (void)qmi8658_enable_wake_on_motion(&s_imu, 12); // ~12 LSB ~ few tens of mg
    }
    maybe_reset_daily_counter();
}

uint32_t sensors_get_step_count(void)
{
    return s_step_count;
}

sensors_activity_t sensors_get_activity(void)
{
    return s_activity;
}

void sensors_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensors task started");
    const TickType_t sample_delay = pdMS_TO_TICKS(20); // ~50 Hz
    float lp = 0.0f; // filtered magnitude
    const float alpha = 0.90f; // LP filter smoothing
    uint32_t last_step_ms = 0;
    // Ring buffer for cadence (last 8 steps)
    uint32_t step_ts_ms[8] = {0};
    int step_ts_idx = 0, step_ts_num = 0;

    bool wom_enabled = true; // enabled in init
    bool ready_for_next_peak = true;
    while (1) {
        maybe_reset_daily_counter();

        // If display is off, rely on wake-on-motion interrupt to wake screen, skip heavy sampling
        if (!display_manager_is_on()) {
            if (!wom_enabled && s_imu_ready) {
                (void)qmi8658_enable_wake_on_motion(&s_imu, 12);
                wom_enabled = true;
            }
            if (s_wom_sem && xSemaphoreTake(s_wom_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "Wake-on-motion IRQ");
                display_manager_turn_on();
            }
            continue;
        }

        if (!s_imu_ready) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        // Display is on: prefer normal sampling, disable WoM to avoid extra interrupts
        if (wom_enabled) {
            (void)qmi8658_disable_wake_on_motion(&s_imu);
            wom_enabled = false;
        }

        float ax, ay, az;
        if (qmi8658_read_accel(&s_imu, &ax, &ay, &az) == ESP_OK) {
            // ax,ay,az in mg
            float mag = sqrtf(ax*ax + ay*ay + az*az); // mg
            float hp = mag - 1000.0f; // remove gravity
            lp = alpha * lp + (1.0f - alpha) * hp;

            // Peak detection
            const float THRESH = 120.0f; // mg
            uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
            uint32_t dt = now_ms - last_step_ms;
            if (lp > THRESH && dt > 280 && dt < 2000) {
                if (ready_for_next_peak) {
                    s_step_count++;
                    // cadence buffer
                    step_ts_ms[step_ts_idx] = now_ms;
                    step_ts_idx = (step_ts_idx + 1) & 7;
                    if (step_ts_num < 8) step_ts_num++;
                    last_step_ms = now_ms;
                    ready_for_next_peak = false;
                }
            } else if (lp < THRESH * 0.5f) {
                ready_for_next_peak = true;
            }

            // Classify activity by cadence (last N steps)
            if (step_ts_num >= 2) {
                uint32_t oldest = step_ts_ms[(step_ts_idx - step_ts_num + 8) & 7];
                uint32_t newest = step_ts_ms[(step_ts_idx - 1 + 8) & 7];
                uint32_t span_ms = newest - oldest;
                float spm = 0.0f;
                if (span_ms > 0) {
                    spm = 60000.0f * (float)(step_ts_num - 1) / (float)span_ms;
                }
                if (spm > 130.0f) s_activity = SENSORS_ACTIVITY_RUN;
                else if (spm > 60.0f) s_activity = SENSORS_ACTIVITY_WALK;
                else if (spm > 10.0f) s_activity = SENSORS_ACTIVITY_OTHER;
                else s_activity = SENSORS_ACTIVITY_IDLE;
            } else {
                s_activity = SENSORS_ACTIVITY_IDLE;
            }
        }

        vTaskDelay(sample_delay);
    }
}
