#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

// Definições de pinos
#define JOYSTICK_Y_PIN 26      // Pino ADC para eixo Y do joystick
#define LED_R_PIN 13           // Pino para LED Vermelho
#define LED_G_PIN 11           // Pino para LED Verde
#define LED_B_PIN 12           // Pino para LED Azul
#define BUZZER_PIN 21         // Pino para o buzzer

// Limiares para os estados do joystick
#define LIMIAR_BAIXO 1400      // Y <= 1400 -> atividade baixa
#define LIMIAR_MODERADO 2100   // Y < 2100 e Y > 1400 -> atividade moderada
#define LIMIAR_ALTO 3200       // Y >= 3200 -> atividade alta

// Estados do sistema
#define ESTADO_BAIXO 1         // LED Verde
#define ESTADO_MODERADO 2      // LED Azul
#define ESTADO_ALTO 3          // LED Vermelho + Buzzer

// Defina a frequência do PWM de forma mais adequada
#define PWM_CLK_DIV 100.0f
#define PWM_WRAP 12500  // Para 100Hz: (125MHz / 100) / 100 = 12500

// Variável compartilhada entre os núcleos
volatile int flag_estado = ESTADO_BAIXO;

// FIFO para comunicação entre núcleos
static int fifo_value;

// Função para configurar o ADC
void adc_init_pin(uint pin) {
    adc_init();
    adc_gpio_init(pin);
    adc_select_input(pin - 26);
}

// Função para ler o joystick
uint16_t ler_joystick_y() {
    return adc_read();
}

// Função para configurar o LED RGB
void led_rgb_init() {
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    
    // Inicia com todos desligados
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
}

void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    
    // Configuração do PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, PWM_CLK_DIV);
    pwm_config_set_wrap(&config, PWM_WRAP);
    pwm_init(slice_num, &config, true);
    
    // Inicia com duty cycle de 50% e desligado
    pwm_set_chan_level(slice_num, channel, PWM_WRAP / 2);
    pwm_set_enabled(slice_num, false);
}


// Função para atualizar o LED conforme o estado
void atualizar_led(int estado) {
    // Desliga todos os LEDs primeiro
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
    
    switch(estado) {
        case ESTADO_BAIXO:
            gpio_put(LED_G_PIN, 1); // Verde
            break;
        case ESTADO_MODERADO:
            gpio_put(LED_B_PIN, 1); // Azul
            break;
        case ESTADO_ALTO:
            gpio_put(LED_R_PIN, 1); // Vermelho
            break;
        default:
            // Nada ligado para estados inválidos
            break;
    }
}

// E controlar_buzzer por:
void controlar_buzzer(int estado) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    if (estado == ESTADO_ALTO) {
        pwm_set_enabled(slice_num, true);
    } else {
        pwm_set_enabled(slice_num, false);
    }
}

// Callback do alarme periódico
int64_t alarme_callback(alarm_id_t id, void *user_data) {
    // Envia o estado atual para o Core 1 via FIFO
    multicore_fifo_push_blocking(flag_estado);
    
    // Agenda o próximo alarme em 2 segundos
    return 2000000; // 2 segundos em microssegundos
}

// Função principal do Core 0
void core0_entry() {
    // Inicializa hardware
    adc_init_pin(JOYSTICK_Y_PIN);
    
    // Configura o alarme periódico
    add_alarm_in_ms(2000, alarme_callback, NULL, true);
    
    // Loop principal do Core 0
    while (true) {
        // Lê o joystick e atualiza o estado
        uint16_t y = ler_joystick_y();
        
        if (y <= LIMIAR_BAIXO) {
            flag_estado = ESTADO_BAIXO;
        } else if (y > LIMIAR_BAIXO && y < LIMIAR_MODERADO) {
            flag_estado = ESTADO_MODERADO;
        } else if (y >= LIMIAR_ALTO) {
            flag_estado = ESTADO_ALTO;
        }
        
        // Pequena pausa para evitar leituras muito rápidas
        sleep_ms(100);
    }
}

// Função principal do Core 1
void core1_entry() {
    // Inicializa hardware
    led_rgb_init();
    buzzer_init();
    
    // Loop principal do Core 1
    while (true) {
        // Espera por um valor na FIFO
        fifo_value = multicore_fifo_pop_blocking();
        
        // Atualiza os dispositivos conforme o estado recebido
        atualizar_led(fifo_value);
        controlar_buzzer(fifo_value);
    }
}

int main() {
    // Inicializa stdio (para debug se necessário)
    stdio_init_all();
    
    // Inicia o Core 1
    multicore_launch_core1(core1_entry);
    
    // O Core 0 executa sua função principal
    core0_entry();
    
    return 0;
}