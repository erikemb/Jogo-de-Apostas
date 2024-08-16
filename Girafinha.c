///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CÓDIGO JÁ TESTADO ABAIXO, FALTA A INTEGRAÇÃO COM O COMANDO DE VOZ E WIFI
//PARA TESTE COPIE E COLE A PARTIR DA IMPORTAÇÃO DAS BIBLIOTECAS ABAIXO
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FEITO
void ativação_e_conf_do_display(){
    // 
}
//FEITO
void display_ativação(char numeros_a_ser_recebidos [] ){
    // recebe um vetor do tipo [9,9,-,2]. Os dois primeiros dados representamo saldo e os dois ultimos o numero 
    //aleatorio gerado
    // Aqui vai fazer a exibição do display.
}

capitação_de_voz(){
    // captura os dados de entreda via mic i2s
    // os dados são armazenados no buffer
    // aqui também está a lógica de detecção do "barulho" (sempre que a freq for maior q 5x o 'ruido' do buffer anterior)
    // quando a detecção for acionada deve enviar 5xbuffer para o servidor
}

//FEITO
int gerador_de_numero_aleatorio(int num_tentativa ){ 
    //gera um numero aleatório que varia de -9 a 9.
    //recebe o numero de tentativa que é um variavel global e abaixo de 5 tentativas
    // a probabilidade de aparecer um número negativo é de 50% 
    // e o tempo de permanência de cada número é de 250 ms.
    //A partir da quinta jogada, a probabilidade de um número negativo aparcer 
    //é 70% e o tempo de permanência do negativo é de 250 ms enquanto o positivo é de 150 ms
    //nome da função dentro da bb é generate_number
    //num_tentativa é a chance de vir negativo
    // ela retorna um numero inteiro 
    
}
//FEITO
int gerenciador_de_saldo(int num_aleatorio,int saldo_altual, int opcao){
// o saldo pode variar de 00 a 99
// esse bloco incrementa e decrementa esse valor
// se opção for 0 o usuário pede crédito, se 1 o usuário quer pegar o numéro_aleatorio
// Se usuário for "Crédito", um credito de 30 é adicionado
}

void conexão_wifi(){
    // bloco de código responsavel por fazer a conexão wifi
}

//FEITO
char gerenciador_de_exibição (int num_aleatorio, int saldo){
    //
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CÓDIGO JÁ TESTADO FALTA A INTEGRAÇÃO COM O COMANDO DE VOZ E WIFI
//PARA TESTE COPIE E COLE A PARTIR DA IMPORTAÇÃO DAS BIBLIOTECAS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "driver/i2c.h"


//Bloco de código do display

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TAG utilizada para logar informações do I2C
static const char *TAG = "i2c-simple-example";

// Definição das configurações do I2C
#define I2C_MASTER_SCL_IO           GPIO_NUM_16      /*!< Número do pino GPIO utilizado para o clock do mestre I2C */
#define I2C_MASTER_SDA_IO           GPIO_NUM_4       /*!< Número do pino GPIO utilizado para os dados do mestre I2C */
#define I2C_MASTER_NUM              0                /*!< Número da porta I2C mestre */
#define I2C_MASTER_FREQ_HZ          100000           /*!< Frequência do clock do mestre I2C */
#define I2C_MASTER_TX_BUF_DISABLE   0                /*!< Mestre I2C não precisa de buffer de transmissão */
#define I2C_MASTER_RX_BUF_DISABLE   0                /*!< Mestre I2C não precisa de buffer de recepção */
#define I2C_MASTER_TIMEOUT_MS       1000             /*!< Timeout do mestre I2C em milissegundos */

// Endereço do dispositivo escravo
#define SLAVE_ADDR                  0x70    

// Endereços dos dígitos no display de 7 segmentos
#define DIG1                        0x00
#define DIG2                        0x02
#define DIG3                        0x06
#define DIG4                        0x08

// Registradores do sistema e do display
#define SYSTEM_SETUP_REG            0x20    // Registrador para configurar o sistema (S = 1 -> operação normal)
#define DISPLAY_SETUP_REG           0x80    // Registrador para configurar o display (D = 1 -> display ligado)
#define DIMMING_SETUP_REG           0xE0    // Registrador para configurar o brilho (dimming)

// Comandos para o VK16K33
#define VK16K33_DISPLAY_ON          0x01    // Comando para ligar o display
#define VK16K33_SYSTEM_OSC_ON       0x01    // Comando para habilitar o oscilador do sistema
#define VK16K33_MAX_BRIGHTNESS      0x0F    // Nível máximo de brilho

// Padrões de segmentos para os dígitos de 0 a 9, mais "-" e espaço em branco
uint8_t digit_patterns[] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111,  // 9
    0b01000000,  // - "10"
    0b00000000,   // + "11"
    0b01110001,   // F "12"
    0b00110000   // F "13"
};

// Função para escrever um byte em um registrador
static esp_err_t register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};  // Buffer de escrita contendo o endereço do registrador e os dados

    // Escreve os dados no dispositivo escravo
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SLAVE_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret;
}

// Função para inicializar o VK16K33
static esp_err_t vk16k33_init(void)
{
    esp_err_t ret;

    // Liga o oscilador do sistema
    ret = register_write_byte(SYSTEM_SETUP_REG | VK16K33_SYSTEM_OSC_ON, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ligar o oscilador do sistema");
        return ret;
    }

    // Liga o display sem piscar
    ret = register_write_byte(DISPLAY_SETUP_REG | VK16K33_DISPLAY_ON, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ligar o display");
        return ret;
    }

    // Define o brilho no máximo
    ret = register_write_byte(DIMMING_SETUP_REG | VK16K33_MAX_BRIGHTNESS, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao definir o brilho");
        return ret;
    }

    ESP_LOGI(TAG, "VK16K33 inicializado com sucesso");
    return ESP_OK;
}

// Função para inicializar o mestre I2C
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    // Configuração da estrutura do I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,               // Define o modo como mestre
        .sda_io_num = I2C_MASTER_SDA_IO,       // Pino de dados (SDA)
        .scl_io_num = I2C_MASTER_SCL_IO,       // Pino de clock (SCL)
        .sda_pullup_en = GPIO_PULLUP_ENABLE,   // Habilita pull-up no SDA
        .scl_pullup_en = GPIO_PULLUP_ENABLE,   // Habilita pull-up no SCL
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // Frequência do clock
    };

    // Configura os parâmetros do I2C
    i2c_param_config(i2c_master_port, &conf);

    // Instala o driver do I2C
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Bloco de gerenciamento do saldo

int gerenciador_de_saldo(int num_aleatorio, int saldo_atual, int opcao) {
   
    // Se a opção for 0, o usuário pede crédito
    if (opcao == 0) {
        saldo_atual += 30; // Adiciona 30 ao saldo
        if (saldo_atual >= 99) {
            saldo_atual = 99; // Garante que o saldo não ultrapasse 99
            }
    }
    // Se a opção for 1, o usuário quer pegar o número aleatório
    else if (opcao == 1) {
        saldo_atual += num_aleatorio; // Soma o número aleatório ao saldo
        if (saldo_atual <= 0) {
            saldo_atual = 0; // Garante que o saldo não seja negativo
        }else{
            if (saldo_atual >= 99) {
            saldo_atual = 99; // Garante que o saldo não ultrapasse 99
        }
        }
    }
    return saldo_atual; // Retorna o saldo atualizado
}
////////////////////////////////////////////////////////////////////////////////

//Bloco de geração de número aleatório

int generate_number(int negative_probability) {
    srand(time(0));
    float number;
    float random_value = (float)rand() / RAND_MAX;  // Gera um valor aleatório entre 0 e 1
    
    // Converte a probabilidade de 0-10 para 0-1
    float scaled_probability = negative_probability / 10.0;

    if (random_value < scaled_probability) {
        number = -(rand() % 10);  // Gera um número negativo entre -9 e -1
    } else {
        number = rand() % 10;  // Gera um número positivo entre 0 e 9
    }

    return number;
}

/////////////////////////////////////////////////////////////////////////////////////

// Função principal (ponto de entrada do aplicativo)
void app_main() {

    int saldo = 30; // Saldo inicial
    int cont=0;
    int probability=5;
    int num_aleatorio;
    
    int opcao=1;//para simulação somente 

    ESP_ERROR_CHECK(i2c_master_init());  // Inicializa o mestre I2C
   
    // Inicializa o VK16K33
    ESP_ERROR_CHECK(vk16k33_init());

    
    while (saldo>0 && saldo<99) {

        num_aleatorio = generate_number(probability);
        
        ESP_ERROR_CHECK(register_write_byte(DIG1, digit_patterns[saldo/10]));  // Exibe o dígito da dezena do saldo
        ESP_ERROR_CHECK(register_write_byte(DIG2, digit_patterns[saldo%10]));  // Exibe o dígito da unidade do saldo
        ESP_ERROR_CHECK(register_write_byte(DIG3, digit_patterns[((num_aleatorio<0)?10:11)])); // Exibe o sinal do número aleatório
        ESP_ERROR_CHECK(register_write_byte(DIG4, digit_patterns[abs(num_aleatorio)]));  // Exibe o numero aleatório de 0 a 9
        
        //esse bloco de código deve ir para interrupção que vai ser gerada com o comando de voz
        cont++; 
        probability=((cont<=5)?5:7);

        vTaskDelay(pdMS_TO_TICKS(1000));
        
        
        saldo = gerenciador_de_saldo (num_aleatorio, saldo, opcao);

        //Essa linha de código é só para teste após integrar o comando de voz ela deve sair.
        //a opção é indicada pelo comando de voz: 0 para credido e 1 para pega.
        if(cont/5>0){
         opcao=(opcao?0:1); 
        }
        opcao=(opcao?0:1);

        }
        //Trecho de código de finalização
        //Aparece o resultado final e o nome "FI" que indica o fim do jogo caso saldo seja 0 ou 99

        ESP_ERROR_CHECK(register_write_byte(DIG1, digit_patterns[saldo/10]));  // Exibe o dígito da dezena do saldo
        ESP_ERROR_CHECK(register_write_byte(DIG2, digit_patterns[saldo%10]));  // Exibe o dígito da unidade do saldo
        ESP_ERROR_CHECK(register_write_byte(DIG3, digit_patterns[12])); // Exibe o sinal do número aleatório
        ESP_ERROR_CHECK(register_write_byte(DIG4, digit_patterns[13]));  // Exibe o numero aleatório de 0 a 9
}



