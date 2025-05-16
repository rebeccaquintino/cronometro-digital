#include "display_lcd.h"
#define GPIO_OUT_W1TS_REG 0x60004008
#define GPIO_OUT_W1TC_REG 0x6000400C

static const display_lcd_config_t *config_local;
static bool modo_4_bits = false;
static int *ptr_out_w1ts = (int *)GPIO_OUT_W1TS_REG;
static int *ptr_out_w1tc = (int *)GPIO_OUT_W1TC_REG;

static int mascara_dado(unsigned char dado);
static void escreve_dado(unsigned char dado);
static void escreve_caractere(unsigned char caractere);

/**
 * @brief	Configura os pinos conectados ao display LCD
 *
 * @param config	Ponteiro para uma estrutura de configuração do tipo display_lcd_config_t
 */
void lcd_config(const display_lcd_config_t *config){
	config_local = config;

	if (!config->D0 && !config->D1 && !config->D2 && !config->D3){
		modo_4_bits = true;
	}

	int pin_mask = mascara_dado(0xFF) | (1 << config_local->E) | (1 << config_local->RS);
	gpio_config_t gpio_cfg = {
		.pin_bit_mask = pin_mask,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = true,
		.pull_down_en = false,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&gpio_cfg);

	if (modo_4_bits){
		modo_4_bits = false;
		lcd_comando(0x20); // Ativa o modo 4 bits (precisa estar em modo 8 bits para funcionar corretamente)
		modo_4_bits = true; // Retorna o indicativo do modo 4 bits
	}

	lcd_comando(0x0C);
	modo_4_bits ? lcd_comando(0x28) : lcd_comando(0x3C);
	lcd_comando(0x01);
	lcd_comando(0x02);
}

/**
 * @brief	Realiza o mascaramento dos dados de acordo com os pinos conectados ao barramento
 *
 * @return	Dado mascarado considerando as posições de cada um dos pinos do barramento em relação aos pinos GPIO
 */
static int mascara_dado(unsigned char dado){
	if (!modo_4_bits) {
		return 	(((dado >> 0) & 1) << config_local->D0) |
				(((dado >> 1) & 1) << config_local->D1) |
				(((dado >> 2) & 1) << config_local->D2) |
				(((dado >> 3) & 1) << config_local->D3) |
				(((dado >> 4) & 1) << config_local->D4) |
				(((dado >> 5) & 1) << config_local->D5) |
				(((dado >> 6) & 1) << config_local->D6) |
				(((dado >> 7) & 1) << config_local->D7);
	} else {
		return 	(((dado >> 0) & 1) << config_local->D4) |
				(((dado >> 1) & 1) << config_local->D5) |
				(((dado >> 2) & 1) << config_local->D6) |
				(((dado >> 3) & 1) << config_local->D7);
	}
};

/**
 * @brief	Realiza o pulso do pino Enable, mantendo em alta por 500us
 */
static void pulso_enable(void ){
	*ptr_out_w1ts = (1 << config_local->E);
	esp_rom_delay_us(500);
	*ptr_out_w1tc = (1 << config_local->E);
}

/**
 * @brief	Escreve o dado no display considerando o modo 8-bits
 */
static void escreve_dado(unsigned char dado){
	*ptr_out_w1tc = mascara_dado(0xFF);
	*ptr_out_w1ts = mascara_dado(dado);
	pulso_enable();
}

/**
 * @brief	Escreve o dado no display considerando o modo 4-bits
 * Primeiro são escritos os bits mais significativos, seguido dos bits menos significativos
 */
static void escreve_dado_4_bits(unsigned char dado){
	*ptr_out_w1tc = mascara_dado(0xF);
	*ptr_out_w1ts = mascara_dado((dado >> 4)& 0xF);
	pulso_enable();
	*ptr_out_w1tc = mascara_dado(0xF);
	*ptr_out_w1ts = mascara_dado((dado >> 0)& 0xF);
	pulso_enable();
}

/**
 * @brief	Envia um comando para o display LCD
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param comando	Comando a ser executado
 */
void lcd_comando(unsigned char comando){
	*ptr_out_w1tc = (1 << config_local->RS);
	modo_4_bits ? escreve_dado_4_bits(comando) : escreve_dado(comando);
}

/**
 * @brief	Escreve um caractere no display LCD
 *
 * @param caractere	Caractere a ser escrito codificado em ASCII
 */
static void escreve_caractere(unsigned char caractere){
	*ptr_out_w1ts = (1 << config_local->RS);
	modo_4_bits ? escreve_dado_4_bits(caractere) : escreve_dado(caractere);
}

/**
 * @brief	Escreve uma string no display LCD
 *
 * @note	A função escreve a string até o limite de 16 caracteres.
 * 			Se a string possuir menos que 16 caracteres, preenche o restante com espaços.
 * 			Isso garante que uma string anterior maior que a atual seja apagada.
 *
 * @param str String que será escrita no display
 */
static void lcd_escreve_str(char str[]){
    int i = 0;

    while ((str[i] != 0x0) && (i < 16)) {
        escreve_caractere(str[i]);
        i++;
    };

    while ((i < 16)) {
        escreve_caractere(' ');
        i++;
    };
};

/**
 * @brief	Escreve duas strings, uma em cada linha do display LCD
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param str1	String que será escrita na primeira linha
 * @param str2	String que será escrita na segunda linha
 */
void lcd_escreve_2_linhas(char str1[], char str2[]){
	lcd_comando(0x80);
	lcd_escreve_str(str1);
    lcd_comando(0xC0);
    lcd_escreve_str(str2);
}


/**
 * @brief	Escreve uma string em uma linha do display
 *
 * @note	Só deve ser chamada após a configuração com lcd_config
 *
 * @param str	String que será escrita
 * @param linha	Linha que será escrita (1 ou 2)
 */
void lcd_escreve_1_linha(char str[], char linha){
	linha == 2 ? lcd_comando(0xC0): lcd_comando(0x80);
	lcd_escreve_str(str);
}
