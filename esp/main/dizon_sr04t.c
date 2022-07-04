/*
  dizon_sr04t.c - ESP-IDF Native C Driver for SR04T Ultrasonic Distance Sensor
  Don McGarry
*/

#include "dizon_sr04t.h"

static const char *TAG = "DIZON_SR04T";

static portMUX_TYPE sr04_mutex = portMUX_INITIALIZER_UNLOCKED;

esp_err_t dizon_sr04t_init(dizon_sr04t_conf_t* conf)
{
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t errt_retval;

    if(conf->num_samples <= 0)
    {
        ESP_LOGE(TAG, "Number of samples for SR04T must be greater than 0");
        return ESP_ERR_INVALID_ARG;
    }
    else if(conf->init)
    {
        ESP_LOGE(TAG, "SR04T is already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_config_t gpio_conf;

    gpio_conf.pin_bit_mask = (1ULL << conf->trig_gpio);
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    errt_retval = gpio_config(&gpio_conf);

    return ESP_OK;

}

/*
 * PRIVATE STATIC.
 *
 * @brief Get ONE raw measurement
 *
 * @doc Speed of sound = 0.0342cm per microsecond
 * @doc The duration is the time it takes to transmit and receive the ultrasonic signal (so the distance is related to half that time measurement).
 * @doc Minimum stable distance =  25cm
 * @doc Maximum stable distance = 350cm
 * @doc Reserve a minimum period of 60 millisec between measurements (avoid signal overlap, do not process the retured ultrasonic signal of the previous measurement).
 * @doc If no obstacle is detected, the output pin will give a 38 millisec high level signal (38 millisec = 6.5 meter which is out of range).
 * @doc If the sensor does not receive an echo within 60 millisec (range too big or too close or no object range) then the signal goes LOW after 60 millisec.
 * @important Use ets_delay_us(). Do not use vTaskDelay() https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/freertos-smp.html?highlight=vtaskdelay
 *
 */
static esp_err_t _get_one_measurement(dizon_sr04t_conf_t* sr04_conf, dizon_sr04t_data_t* sr04_data) {
    
    esp_err_t retval;
    
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    bool do_trigger = true; // Set :=false to test use case that the trigger sequence is not executed.

    // Reset receive values
    sr04_data->rec_data = false;
    sr04_data->error = false;
    sr04_data->raw = 0;
    sr04_data->distance_cm = 0.0;

    /*
     * # SENSOR RMT
     */
    size_t rx_size = 0;
    rmt_item32_t* ptr_rx_item;
    bool is_ringbuffer_received = false;
    RingbufHandle_t rb = NULL;
    retval = rmt_get_ringbuf_handle(param_ptr_config->rmt_channel, &rb);
    if (retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. rmt_get_ringbuf_handle() | err %i (%s)", __FUNCTION__, f_retval,
                esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
    }

    retval = rmt_rx_start(param_ptr_config->rmt_channel, true);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. rmt_rx_start() | err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
    }

    /*
     * # SENSOR CMD Start Measurement:
     *      1. trigger_gpio level:=0 for at least 60 MILLIsec
     *      2. trigger_gpio level:=1 for at least 25 MICROsec
     */
    if (do_trigger == true) {
        portENTER_CRITICAL(&jsnsr04t_spinlock);
        gpio_set_level(param_ptr_config->trigger_gpio_num, 0);
        ets_delay_us(60 * 1000);
        gpio_set_level(param_ptr_config->trigger_gpio_num, 1);
        ets_delay_us(25);
        gpio_set_level(param_ptr_config->trigger_gpio_num, 0);
        portEXIT_CRITICAL(&jsnsr04t_spinlock);
    }

    /*
     * SENSOR Collect data:
     *   - Pull the data from the ring buffer.
     *
     *   @doc xRingbufferReceive() param#3 ticks_to_wait MAXIMUM nbr of Ticks to wait for items in the ringbuffer ELSE Timeout (NULL).
     */
    ptr_rx_item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, RTOS_DELAY_100MILLISEC);
    if (ptr_rx_item == NULL) {
        f_retval = ESP_ERR_TIMEOUT;
        ESP_LOGE(TAG, "%s(). ABORT. xRingbufferReceive() | err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
        // GOTO
        goto cleanup;
    }
    is_ringbuffer_received = true;
    int nbr_of_items = rx_size / sizeof(rmt_item32_t);

    // DEBUG Dump
    ESP_LOGD(TAG, "  nbr_of_items         = %i", nbr_of_items);
    ESP_LOGD(TAG, "  sizeof(rmt_item32_t) = %i", sizeof(rmt_item32_t));
    rmt_item32_t* temp_ptr = ptr_rx_item; // Use a temporary pointer (=pointing to the beginning of the item array)
    for (uint8_t i = 0; i < nbr_of_items; i++) {
        ESP_LOGD(TAG, "  %2i :: [level 0]: %1d - %5d microsec, [level 1]: %3d - %5d microsec",
                i,
                temp_ptr->level0, temp_ptr->duration0,
                temp_ptr->level1, temp_ptr->duration1);
        temp_ptr++;
    }

    // Check RMT nbr_of_items
    if (nbr_of_items != 1) {
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "%s(). ABORT. RMT nbr_of_items != 1 (%i) | err %i (%s)", __FUNCTION__, nbr_of_items, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    // Check RMT first_item level0
    if (ptr_rx_item->level0 != 1) {
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "%s(). ABORT. RMT ptr_rx_item->level0 != 1 (%u) | err %i (%s)", __FUNCTION__, ptr_rx_item->level0,
                f_retval,
                esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
        // GOTO
        goto cleanup;
    }

    // COMPUTE
    param_ptr_raw_data->data_received = true;
    param_ptr_raw_data->raw = ptr_rx_item->duration0; // Unit=microseconds
    param_ptr_raw_data->distance_cm = (param_ptr_raw_data->raw / 2) * 0.0342; // @uses Lightspeed

    if (param_ptr_raw_data->distance_cm < MJD_JSNSR04T_MINIMUM_SUPPORTED_DISTANCE_CM) {
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "%s(). ABORT. Out Of Range: distance_cm < %f (%f) | err %i (%s)", __FUNCTION__,
                MJD_JSNSR04T_MINIMUM_SUPPORTED_DISTANCE_CM, param_ptr_raw_data->distance_cm, f_retval,
                esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
        // GOTO
        goto cleanup;
    }
    if (param_ptr_raw_data->distance_cm > MJD_JSNSR04T_MAXIMUM_SUPPORTED_DISTANCE_CM) {
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "%s(). ABORT. Out Of Range: distance_cm > %f (%f) | err %i (%s)", __FUNCTION__,
                MJD_JSNSR04T_MAXIMUM_SUPPORTED_DISTANCE_CM, param_ptr_raw_data->distance_cm, f_retval,
                esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
        // GOTO
        goto cleanup;
    }

    // ADJUST with distance_sensor_to_artifact_cm (default 0cm).
    if (param_ptr_config->distance_sensor_to_artifact_cm != 0.0) {
        param_ptr_raw_data->distance_cm -= param_ptr_config->distance_sensor_to_artifact_cm;
        if (param_ptr_raw_data->distance_cm <= 0.0) {
            f_retval = ESP_ERR_INVALID_RESPONSE;
            ESP_LOGE(TAG,
                    "%s(). ABORT. Invalid value: adjusted distance <= 0 (subtracted sensor_artifact_cm) (%f) | err %i (%s)",
                    __FUNCTION__,
                    param_ptr_raw_data->distance_cm, f_retval, esp_err_to_name(f_retval));
            _mark_error_event(param_ptr_config, param_ptr_raw_data);
            // GOTO
            goto cleanup;
        }
    }

    // LABEL
    cleanup: ;

    if (is_ringbuffer_received == true) {
        // Compagnon func of xRingbufferReceive()
        // After parsing the data, return data to ringbuffer (as soon as possible).
        // Only return item when rigbuffer data was really received (error handling).
        vRingbufferReturnItem(rb, (void*) ptr_rx_item);
    }

    // @important Do NOT overwrite f_retval!
    if (rmt_rx_stop(param_ptr_config->rmt_channel) != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. rmt_rx_stop() | err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        _mark_error_event(param_ptr_config, param_ptr_raw_data);
    }

    return f_retval;
}

esp_err_t dizon_sr04t_get_data(dizon_sr04t_conf_t* conf, dizon_sr04t_data_t* data)
{
    esp_err_t f_retval = ESP_OK;

    // Reset receive values
    param_ptr_data->data_received = false;
    param_ptr_data->is_an_error = false;
    param_ptr_data->distance_cm = 0.0;

    // Samples
    uint32_t count_errors = 0;
    double sum_distance_cm = 0;
    double min_distance_cm = FLT_MAX;
    double max_distance_cm = FLT_MIN;
    mjd_jsnsr04t_raw_data_t samples[param_ptr_config->nbr_of_samples];

    for (uint32_t j = 1; j <= param_ptr_config->nbr_of_samples; j++) {
        f_retval = _get_one_measurement(param_ptr_config, &samples[j]);
        if (f_retval != ESP_OK) {
            ESP_LOGW(TAG, "[CONTINUE-LOOP] %s(). WARNING. _get_one_measurement() | err %i (%s)", __FUNCTION__, f_retval,
                    esp_err_to_name(f_retval));
            // Do not exit loop!
        }
        _log_raw_data(samples[j]);

        // COUNT ERRORS
        if (samples[j].is_an_error == true) {
            count_errors += 1;
        }
        // SUM
        sum_distance_cm += samples[j].distance_cm;
        // IDENTIFY MIN VALUE and MAX VALUE
        if (samples[j].distance_cm < min_distance_cm) {
            min_distance_cm = samples[j].distance_cm;
        }
        if (samples[j].distance_cm > max_distance_cm) {
            max_distance_cm = samples[j].distance_cm;
        }
    }

    // Validation
    if (count_errors > 0) {
        ++param_ptr_config->nbr_of_errors;
        param_ptr_data->is_an_error = true; // Mark error
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG, "%s(). ABORT. At least one measurement in error (%u) | err %i (%s)", __FUNCTION__, count_errors,
                f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    if ((max_distance_cm - min_distance_cm) > param_ptr_config->max_range_allowed_in_samples_cm) {
        ++param_ptr_config->nbr_of_errors;
        param_ptr_data->is_an_error = true; // Mark error
        f_retval = ESP_ERR_INVALID_RESPONSE;
        ESP_LOGE(TAG,
                "%s(). ABORT. Statistics: range min-max is too big %f (max_range_allowed_in_samples_cm: %f) | err %i (%s)",
                __FUNCTION__,
                max_distance_cm - min_distance_cm, param_ptr_config->max_range_allowed_in_samples_cm, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    // Deduct one weighted measurement
    param_ptr_data->data_received = true;
    param_ptr_data->distance_cm = sum_distance_cm / param_ptr_config->nbr_of_samples;

    // LABEL
    cleanup: ;

    return f_retval;

    return ESP_OK;
}