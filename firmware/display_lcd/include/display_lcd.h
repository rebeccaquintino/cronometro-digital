#pragma once


/**
 * @file display_lcd.h
 * @brief Funções para controle de um display LCD
 */

#include "driver/gpio.h"
#include "esp_rom_sys.h"

/**
 * @brief	Estrutura de configuração dos pinos do display LCD
 *
 * @note	Indicar os pinos GPIO conectados a cada um dos pinos do display LCD
 *
 */
typedef struct {
    unsigned char D0, D1, D2, D3, D4, D5, D6, D7, E, RS;
} display_lcd_config_t;

/**
 * @brief	Configura os pinos conectados ao display LCD
 *
 * @param config	Ponteiro para uma estrutura de configuração do tipo display_lcd_config_t
 */
void lcd_config(const display_lcd_config_t *config);

/**
 * @brief	Envia um comando para o display LCD
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param comando	Comando a ser executado
 */
void lcd_comando(unsigned char comando);

/**
 * @brief	Escreve duas strings, uma em cada linha do display LCD
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param str1	String que será escrita na primeira linha
 * @param str2	String que será escrita na segunda linha
 */
void lcd_escreve_2_linhas(char str1[], char str2[]);

/**
 * @brief	Escreve uma string em uma linha do display
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param str	String que será escrita
 * @param linha	Linha que será escrita (1 ou 2)
 */
void lcd_escreve_1_linha(char str[], char linha);
