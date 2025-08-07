

# HX711_IDF

An ESP-IDF based driver and application for the HX711 24-bit ADC from Avia Semiconductor, designed specifically for ESP32 systems. The HX711 is commonly used in weighing scales and industrial pressure sensor applications.

This implementation reads raw ADC values from the HX711 module, calibrates the values to real-world units (PSI in our case), and publishes them to a ThingsBoard IoT platform using MQTT.

## 📌 Features
- Reads 24-bit raw ADC data from HX711
- Calibrates ADC values to pressure in PSI
- Integrates with ThingsBoard using MQTT
- Built-in ESP-IDF logging for debugging
- Compatible with HX711-based pressure sensors or load cells

## 📷 Screenshots
**Raw ADC Values Displayed via idf.py monitor**  
  ![PRESSURE SENSOR](<pressure sensor counts.PNG>)

**PSI Data Displayed on ThingsBoard Dashboard**  
![Pressure Monitoring via Thingsboard Dashboard](<Pressure monitor.PNG>)

## ⚙️ Workflow Description

### 1️⃣ Reading Raw ADC Data
We start by configuring the HX711 interface using GPIO pins for DOUT and PD_SCK. The raw output is captured via bit-banging, as the HX711 uses a two-wire synchronous serial protocol (not I2C/SPI). The ESP32 reads the 24-bit signed data which represents the analog sensor output amplified by the HX711's internal PGA.

```c
int32_t reading = hx711_read_average(10);  // Read 10 samples and average
ESP_LOGI(TAG, "Raw ADC: %ld", reading);
```

### 2️⃣ Calibrating to PSI
Once raw data is retrieved, a calibration constant (based on known weight/pressure) is applied to map the digital value to a physical PSI measurement.

```c
float pressure_psi = (float)(reading - offset) / scale;
ESP_LOGI(TAG, "Pressure = %.2f PSI", pressure_psi);
```

Where:  
- `offset` is the raw value when no load is applied.  
- `scale` is determined experimentally (known PSI vs. ADC output).  

### 3️⃣ Sending Data to ThingsBoard (MQTT)
After calibration, the data is packaged into a JSON payload and published using ESP-IDF’s MQTT client.

```c
char payload[64];
snprintf(payload, sizeof(payload), "{\"pressure\": %.2f}", pressure_psi);
esp_mqtt_client_publish(client, "v1/devices/me/telemetry", payload, 0, 1, 0);
```

Ensure that:  
- Your ThingsBoard device is created.  
- The Access Token is used in the MQTT config.  
- Device is connected to the MQTT broker (`mqtt.thingsboard.cloud` or local).  

## 🧠 Additional Notes
- You may fine-tune calibration constants using real pressure data for improved accuracy.  
- Make sure your sensor is powered properly and that data lines are noise-filtered.  
- Use `idf.py monitor` for live logs during development/debugging.  
```

### Key Formatting Notes:
1. Used `#` headers for main title and `##` for sections
2. Code blocks wrapped with triple backticks (with language specifier `c` for C code)
3. Lists formatted with `-` for bullets
4. Emojis preserved for visual organization
5. Screenshot placeholders kept as-is (replace with actual image links in real usage)
6. Important terms/values highlighted with backticks (e.g., `` `offset` ``)
