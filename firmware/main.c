/**
 * @file cronometro.c
 * @brief Implementação de um cronômetro com marcação de voltas utilizando ESP32-C3.
 *
 * Este código utiliza GPTimer para contagem de tempo, um display LCD 16x2 para exibição
 * do tempo e botões com debounce via software para controle do cronômetro.
 *
 * @author Rebecca Quintino Do Ó
 * @date 2025-05-20
 */

#include "display_lcd.h"
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

gptimer_handle_t temporizador = NULL;
esp_timer_handle_t debounce_timer[4];

/// Configuração dos pinos do display LCD
display_lcd_config_t config_display = {
    .D4 = 5, .D5 = 6, .D6 = 7, .D7 = 8,
    .RS = 9, .E = 10
};

// Protótipos
static void configura_botao(void);
static void isr_botao_handler(void *arg); 
static void debounce_timer_callback(void *arg);
static bool funcao_tratamento_alarme(gptimer_handle_t temporizador, const gptimer_alarm_event_data_t *edata, void *user_ctx);
static gptimer_handle_t configura_temporizador(void);

/**
 * @brief Função principal do aplicativo.
 *
 * Inicializa o display, os botões e o temporizador. Atualiza o display continuamente com o tempo e a última volta.
 */
void app_main() {
    esp_task_wdt_deinit();  ///< Desativa o watchdog da tarefa principal.
    lcd_config(&config_display);  ///< Inicializa o display LCD.
    configura_botao();
    temporizador = configura_temporizador();  ///< Inicializa o temporizador.

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

        snprintf(str_time, sizeof(str_time), "Tempo %02u:%02u:%02u", minutos, segundos, centesimos); 
        snprintf(str_volta, sizeof(str_volta), "Volta %02u:%02u:%02u", min_volta, sec_volta, cent_volta); 
        lcd_escreve_2_linhas(str_time, str_volta);      
    }
}

/**
 * @brief Configura e inicializa o temporizador GPTimer.
 *
 * O temporizador é configurado para gerar interrupções a cada 10 ms (1 centésimo de segundo).
 *
 * @return Handle para o temporizador configurado
 */
static gptimer_handle_t configura_temporizador(void) {
    gptimer_handle_t temporizador_aux = NULL;

    gptimer_config_t config_temporizador = {
    .clk_src = GPTIMER_CLK_SRC_APB,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000,  ///< 1 MHz = 1 µs por tick
    };
    gptimer_new_timer(&config_temporizador, &temporizador_aux);

    gptimer_alarm_config_t config_alarme = {
    .alarm_count = 10000,  ///< 10.000 ticks = 10 ms = 1 centésimo de segundo
    .reload_count = 0,
    .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(temporizador_aux, &config_alarme);

    gptimer_event_callbacks_t config_callback = {
    .on_alarm = funcao_tratamento_alarme,
    };
    gptimer_register_event_callbacks(temporizador_aux, &config_callback, NULL);

    gptimer_enable(temporizador_aux); 
    return temporizador_aux;
}

/**
 * @brief Callback da interrupção do temporizador.
 *
 * Incrementa a contagem de centésimos.
 *
 * @param temporizador Handle do temporizador
 * @param edata Dados do evento
 * @param user_ctx Contexto do usuário
 * @return true Sempre retorna verdadeiro para continuar
 */
static bool funcao_tratamento_alarme(gptimer_handle_t temporizador, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    centesimos++;
    return true;
}

/**
 * @brief Callback do timer de debounce.
 *
 * Verifica qual botão foi pressionado e executa a ação correspondente:
 * iniciar, parar, marcar volta ou resetar o cronômetro.
 *
 * @param arg Número do pino associado ao botão
 */
static void debounce_timer_callback(void *arg) {
    int pino = (int)(intptr_t)arg;

    if (gpio_get_level(pino) == 0) { ///< Verifica se o botao ainda esta pressionado
        switch (pino) {
            case 0:  ///< Start
                if(!contando) {
                    gptimer_start(temporizador);
                    contando = true;
                    ESP_LOGI("BOTOES", "Start pressionado");
                }
                break;
            case 1:  ///< Stop
                if(contando) {
                    gptimer_stop(temporizador);
                    contando = false;
                    ESP_LOGI("BOTOES", "Stop pressionado");
                }
                break;
            case 2:  ///< Volta
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
            case 3:  ///< Reset
                if(!contando) {
                    gptimer_set_raw_count(temporizador, 0);  ///< Zera o contador
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
    gpio_intr_enable(pino); ///< Reabilita a interrupção
}

/**
 * @brief Handler de interrupção dos botões.
 *
 * Desabilita a interrupção do pino e inicia o timer de debounce correspondente.
 *
 * @param arg Número do pino associado ao botão
 */
static void isr_botao_handler(void *arg) {
    int pino = (int)(intptr_t)arg;
    gpio_intr_disable(pino); ///< Desabilita interrupções no pino
    esp_timer_start_once(debounce_timer[pino], 30000); ///< Inicia o timer de debounce
}

/**
 * @brief Configura os botões de entrada.
 *
 * Define os pinos como entrada com pull-up e configura as interrupções por borda de subida.
 */
static void configura_botao(void) {
    gpio_config_t io_cfg_button = { 
        .pin_bit_mask =  (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    gpio_config(&io_cfg_button);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    for (int i = 0; i < 4; i++) {
        esp_timer_create_args_t timer_args = {
            .callback = debounce_timer_callback,
            .arg = (void*)(intptr_t)i,
            .name = "debounce_timer"
        };
        esp_timer_create(&timer_args, &debounce_timer[i]);
        gpio_isr_handler_add(i, isr_botao_handler, (void *)(intptr_t)i);
    }
}
