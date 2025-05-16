#include "display_lcd.c"

#include "driver/gptimer.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

// Variáveis globais
uint8_t centesimos = 0, segundos = 0, minutos = 0;
uint8_t cent_ant = 0, sec_ant = 0, min_ant = 0;
uint8_t cent_volta = 0, sec_volta = 0, min_volta = 0;
bool contando = false;
char str_time[18];
char str_volta[18];

// Manipuladores
esp_timer_handle_t debounce_timer[4]; 
gptimer_handle_t temporizador = NULL;

/// Configuração dos pinos do display LCD
display_lcd_config_t config_display = {
    .D4 = 5, .D5 = 6, .D6 = 7, .D7 = 8,
    .RS = 9, .E = 10
};

// Protótipos
static void debounce_timer_callback(void *arg);
static void isr_pino_handler(void *arg);
bool funcao_tratamento_alarme(gptimer_handle_t temporizador, const gptimer_alarm_event_data_t *edata, void *user_ctx);
gptimer_handle_t inicia_temporizador(void);
void escreve_time(uint8_t min, uint8_t sec, uint8_t cent);
void escreve_volta(uint8_t min, uint8_t sec, uint8_t cent);
void delay(int ciclos);

void app_main(){
    esp_task_wdt_deinit();  ///< Desativa o watchdog da tarefa principal.

    for (int i = 0; i < 4; i++){
        esp_timer_create_args_t timer_args = {
            .callback = debounce_timer_callback,
            .arg = (void*)(intptr_t)i,
            .name = "debounce_timer"
        };
        esp_timer_create(&timer_args, &debounce_timer[i]);
    }
  
    gpio_config_t io_cfg_button = { ///< Configura os pinos do botao
        .pin_bit_mask = (1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 3),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config(&io_cfg_button);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    // Configura interrupções para os botões
    for (int i = 0; i < 4; i++) {
        gpio_isr_handler_add(i, isr_pino_handler, (void*)(intptr_t)i);
    }

    lcd_config(&config_display);  ///< Inicializa o display LCD.

    gpio_config_t io_cfg_lcd = {
        .pin_bit_mask = 0x7E0,
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_cfg_lcd);  ///< Configura os GPIOs como saída.

    temporizador = inicia_temporizador();  ///< Inicializa o temporizador.

    while (1) {
        // Atualiza tempo
        if (centesimos >= 100) {
            segundos++;
            centesimos = 0;
        }
        if (segundos >= 60) {
            minutos++;
            segundos = 0;
        }
        if (minutos >= 60) {
            centesimos = segundos = minutos = 0;
        }

        escreve_time(minutos, segundos, centesimos);
        escreve_volta(min_volta, sec_volta, cent_volta);
        delay(5000);            
    }
}

gptimer_handle_t inicia_temporizador(void) {
    gptimer_handle_t timer;

    gptimer_config_t config_temporizador = {
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  ///< 1 MHz = 1 µs por tick
    };
    gptimer_new_timer(&config_temporizador, &timer);

    gptimer_alarm_config_t config_alarme = {
        .alarm_count = 10000,  ///< 10.000 ticks = 10 ms = 1 centésimo de segundo
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(timer, &config_alarme);

    gptimer_event_callbacks_t config_callback = {
        .on_alarm = funcao_tratamento_alarme,
    };
    gptimer_register_event_callbacks(timer, &config_callback, NULL);
    gptimer_enable(timer); 

    return timer;
}

bool funcao_tratamento_alarme(gptimer_handle_t temporizador, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    if (contando){
      centesimos++;  ///< Incrementa o valor de centésimos a cada interrupção.
    }
    return true;
}

static void debounce_timer_callback(void *arg) {
    int pino = (int)(intptr_t)arg;

    if (gpio_get_level(pino) == 0) { ///< Verifica se o botao ainda esta pressionado
        switch (pino) {
            case 0:
                if(!contando) {
                    gptimer_start(temporizador);
                    contando = true;
                    ESP_LOGI("BOTOES", "Start pressionado");
                }
                break;
            case 1:
                if(contando) {
                    gptimer_stop(temporizador);
                    contando = false;
                    ESP_LOGI("BOTOES", "Stop pressionado");
                }
                break;
            case 2:
                if(contando) {
                    min_volta = minutos - min_ant;
                    sec_volta = segundos - sec_ant;
                    cent_volta = centesimos - cent_ant;
                    min_ant = minutos;
                    sec_ant = segundos;
                    cent_ant = centesimos;
                    ESP_LOGI("BOTOES", "Volta pressionada");
                }
                break;
            case 3:
                if(!contando) {
                    gptimer_set_raw_count(temporizador, 0);  ///< zera o contador
                    minutos = segundos = centesimos = 0;
                    min_volta = sec_volta = cent_volta = 0;
                    min_ant = sec_ant = cent_ant = 0;
                    gptimer_start(temporizador);
                    contando = true;
                    ESP_LOGI("BOTOES", "Reset pressionado");
                }
                break;
            default:
                break;
        }
    }
    gpio_intr_enable(pino); ///< Reabilita a interrupcao
}

static void isr_pino_handler(void *arg) {
    int pino = (int)(intptr_t)arg;
    gpio_intr_disable(pino); ///< Desabilita interrupcoes no pino
    esp_timer_start_once(debounce_timer[pino], 30000); ///< Inicia o timer de debounce
}

void escreve_time(uint8_t min, uint8_t sec, uint8_t cent) {
    snprintf(str_time, sizeof(str_time), "Tempo %02u:%02u:%02u", min, sec, cent); 
    lcd_escreve_1_linha(str_time, 1); 
}

void escreve_volta(uint8_t min, uint8_t sec, uint8_t cent) {
    snprintf(str_volta, sizeof(str_volta), "Volta %02u:%02u:%02u", min, sec, cent); 
    lcd_escreve_1_linha(str_volta, 2); 
}

void delay(int ciclos) {
    for (volatile int i = 0; i < ciclos * 10000; i++);
}