#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"

#define SERVER_IP   "192.168.0.1" //se tiver no windows, abre o cmd e digita 'ipconfig' sem aspas. Endereço IPv4. . . . . . . .  . . . . . . . : 192.168.0.1
#define SERVER_PORT 65432
#define BUTTON_1_PIN GPIO_NUM_32
#define BUTTON_2_PIN GPIO_NUM_33
#define ESP_WIFI_SSID      "COLOCA O NOME DA REDE AQUI"
#define ESP_WIFI_PASS      "A SENHA DA REDE AQUI"
#define MAXIMUM_RETRY      5
#define BUTTON_PIN GPIO_NUM_4  // Escolha o pino que você está usando

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG = "WIFI";

// Definições de pinos e configurações do display
#define I2C_MASTER_SCL_IO           GPIO_NUM_16
#define I2C_MASTER_SDA_IO           GPIO_NUM_4
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define SLAVE_ADDR                  0x70
#define DIG1                        0x00
#define DIG2                        0x02
#define DIG3                        0x06
#define DIG4                        0x08

#define SYSTEM_SETUP_REG            0x20
#define DISPLAY_SETUP_REG           0x80
#define DIMMING_SETUP_REG           0xE0
#define VK16K33_DISPLAY_ON          0x01
#define VK16K33_SYSTEM_OSC_ON       0x01
#define VK16K33_MAX_BRIGHTNESS      0x0F

static const char *TAG2 = "i2c-simple-example";


void configure_button() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;   // Desabilitar interrupção
    io_conf.mode = GPIO_MODE_INPUT;           // Configurar como entrada
    io_conf.pin_bit_mask = (1ULL << BUTTON_1_PIN);  // Máscara do pino do botão
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Desabilitar pull-down
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // Habilitar pull-up
    gpio_config(&io_conf);
}

void configure_button2() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;   // Desabilitar interrupção
    io_conf.mode = GPIO_MODE_INPUT;           // Configurar como entrada
    io_conf.pin_bit_mask = (1ULL << BUTTON_2_PIN);  // Máscara do pino do botão
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Desabilitar pull-down
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // Habilitar pull-up
    gpio_config(&io_conf);
}

uint8_t digit_patterns[] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111,
    0b01000000, 0b00000000, 0b01110001, 0b00110000
};

static esp_err_t register_write_byte(uint8_t reg_addr, uint8_t data) {
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SLAVE_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}

static esp_err_t vk16k33_init(void) {
    esp_err_t ret;
    ret = register_write_byte(SYSTEM_SETUP_REG | VK16K33_SYSTEM_OSC_ON, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG2, "Falha ao ligar o oscilador do sistema");
        return ret;
    }

    ret = register_write_byte(DISPLAY_SETUP_REG | VK16K33_DISPLAY_ON, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG2, "Falha ao ligar o display");
        return ret;
    }

    ret = register_write_byte(DIMMING_SETUP_REG | VK16K33_MAX_BRIGHTNESS, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG2, "Falha ao definir o brilho");
        return ret;
    }

    ESP_LOGI(TAG2, "VK16K33 inicializado com sucesso");
    return ESP_OK;
}

static esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "got ip:");
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

int gerenciador_de_saldo(int num_aleatorio, int saldo_atual, int opcao) {
    if (opcao == 0) {
        saldo_atual += 30;
        if (saldo_atual >= 99) {
            saldo_atual = 99;
        }
    } else if (opcao == 1) {
        saldo_atual += num_aleatorio;
        if (saldo_atual <= 0) {
            saldo_atual = 0;
        } else if (saldo_atual >= 99) {
            saldo_atual = 99;
        }
    }
    return saldo_atual;
}

int generate_number(int negative_probability) {
    srand(time(0));
    float number;
    float random_value = (float)rand() / RAND_MAX;
    float scaled_probability = negative_probability / 10.0;

    if (random_value < scaled_probability) {
        number = -(rand() % 10);
    } else {
        number = rand() % 10;
    }

    return number;
}

int num_aleatorio;

void send_message_to_server(int opcao, char* response) {
    struct sockaddr_in server_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Falha ao criar o socket");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

      if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Falha ao conectar ao servidor");
        close(sock);
        return;
    }

    char opcao_str[2];
    snprintf(opcao_str, sizeof(opcao_str), "%d", opcao);

    if (send(sock, opcao_str, strlen(opcao_str), 0) < 0) {
        ESP_LOGE(TAG, "Falha ao enviar a mensagem");
        close(sock);
        return;
    }

    int len = recv(sock, response, 64, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "Falha ao receber a resposta");
        close(sock);
        return;
    }

    response[len] = '\0';
    ESP_LOGI(TAG, "Resposta recebida do servidor: %s", response);

    close(sock);
}

void update_display(int saldo) {
    uint8_t digits[4];
    digits[0] = digit_patterns[(saldo / 10) % 10];
    digits[1] = digit_patterns[saldo % 10];

    register_write_byte(DIG1, digits[0]);
    register_write_byte(DIG2, digits[1]);
}

void update_random_digits_task(void *pvParameters) {
    while (1) {
        num_aleatorio = generate_number(5);  // Ajuste a probabilidade conforme necessário

        // Atualiza os dígitos 3 e 4 com o número aleatório
        register_write_byte(DIG3, digit_patterns[(num_aleatorio < 0) ? 10 : 11]);  // Exibe o sinal
        register_write_byte(DIG4, digit_patterns[abs(num_aleatorio)]);  // Exibe o número aleatório

        vTaskDelay(pdMS_TO_TICKS(250));  //taxa de atuazlicação??
    }
}


void app_main(void) {
    esp_err_t ret;
    configure_button();
    configure_button2();

    // Inicializa I2C e o display
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG2, "Falha ao inicializar I2C");
        return;
    }

    ret = vk16k33_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG2, "Falha ao inicializar o VK16K33");
        return;
    }

    // Inicializa Wi-Fi
    esp_err_t ret_wifi = nvs_flash_init();
    if (ret_wifi == ESP_ERR_NVS_NO_FREE_PAGES || ret_wifi == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret_wifi = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret_wifi);

    wifi_init_sta();

    int saldo = 30;
    
     xTaskCreate(&update_random_digits_task, "update_random_digits_task", 4096, NULL, 5, NULL);

    while (saldo>1 && saldo <99) {
        // Verifica o estado dos botões
        if (gpio_get_level(BUTTON_1_PIN) == 0) {
            char response[64];
            send_message_to_server(0, response);

            if (strcmp(response, "Credito") == 0) {
                saldo = gerenciador_de_saldo(num_aleatorio, saldo, 0);
                update_display(saldo);
                ESP_LOGI(TAG, "Saldo atualizado: %d", saldo);
            }
        }

        if (gpio_get_level(BUTTON_2_PIN) == 0) {
            char response[64];
            send_message_to_server(1, response);

            if (strcmp(response, "Pega") == 0) {
                saldo = gerenciador_de_saldo(num_aleatorio, saldo, 1);
                update_display(saldo);
                ESP_LOGI(TAG, "Saldo atualizado: %d", saldo);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(250));  // Aguarda um pouco antes de verificar os botões novamente
    }
}

