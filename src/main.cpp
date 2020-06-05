#include <Arduino.h>

struct params_t {
  uint32_t from;
  uint32_t to;
};

const uint32_t ARRAY_SIZE = 1024 * 64;

uint8_t array[ARRAY_SIZE];
SemaphoreHandle_t semaphore;

uint8_t RTC_DATA_ATTR tasks = 0;

static void taskProducer(void *pvParam) {
  for (uint32_t i = ((params_t*)pvParam)->from; i < ((params_t*)pvParam)->to; ++i) {
    array[i] = random(256);
  }
  xSemaphoreGive(semaphore);
  vTaskDelete(NULL);
}

static void halt(const char *msg) {
  Serial.println(msg);
  Serial.println("System halted!");
  Serial.flush();
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  if (! tasks)
    tasks = 1;
  else
    tasks *= 2;

  if (tasks == 1)
    semaphore = xSemaphoreCreateBinary();
  else
    semaphore = xSemaphoreCreateCounting(tasks, 0);
  if (! semaphore)
    halt("Error creating semaphore!");

  Serial.printf("*** Creating %u task(s) ***\r\n", tasks);

  uint32_t time = micros();

  for (uint8_t i = 0; i < tasks; ++i) {
    params_t params;

    params.from = ARRAY_SIZE / tasks * i;
    params.to = params.from + ARRAY_SIZE / tasks;
    if (xTaskCreatePinnedToCore(&taskProducer, "producer", 1024, &params, 1, NULL, i & 0x01) != pdPASS)
      halt("Error creating task!");
//    Serial.printf("Task #%u (%u .. %u)\r\n", i, params.from, params.to - 1);
  }

  for (uint8_t i = 0; i < tasks; ++i) {
    xSemaphoreTake(semaphore, portMAX_DELAY);
  }
  time = micros() - time;
  Serial.printf("Execution time: %u us.\r\n", time);
  Serial.flush();
  if (tasks >= 8)
    esp_deep_sleep_start();
  else {
    esp_deep_sleep_disable_rom_logging();
    esp_deep_sleep(0);
  }
}

void loop() {}
