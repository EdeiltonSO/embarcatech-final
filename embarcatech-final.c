#include <stdio.h>
#include <stdlib.h>

// libs para o display
#include "libs/ssd1306.h"
#include "libs/font.h"

#define GREEN_LED_GPIO_PIN 11
#define BLUE_LED_GPIO_PIN 12
#define RED_LED_GPIO_PIN 13

// macros para os botões
#define A_BUTTON_GPIO_PIN 5
#define B_BUTTON_GPIO_PIN 6
#define DEBOUNCING_TIME_MS 200

// para o cálculo do debounce
int last_time = 0;

char time_str[20];
bool running = false;
uint32_t start_time = 0;
uint32_t elapsed_time = 0;

// estrutura de dados do display
ssd1306_t ssd;

// protótipos de funções
void display_setup(ssd1306_t *ssd);
void display_greetings(ssd1306_t *ssd);
void display_write_message(ssd1306_t *ssd, char *msg);
static void button_irq_handler(uint gpio, uint32_t events);

int main()
{
  stdio_init_all();

  // inicializa a estrutura de dados que será enviada para o display
  display_setup(&ssd);

  // inicializa LED RGB
  gpio_init(GREEN_LED_GPIO_PIN);
  gpio_set_dir(GREEN_LED_GPIO_PIN, GPIO_OUT);
  gpio_put(GREEN_LED_GPIO_PIN, false);

  gpio_init(BLUE_LED_GPIO_PIN);
  gpio_set_dir(BLUE_LED_GPIO_PIN, GPIO_OUT);
  gpio_put(BLUE_LED_GPIO_PIN, false);

  gpio_init(RED_LED_GPIO_PIN);
  gpio_set_dir(RED_LED_GPIO_PIN, GPIO_OUT);
  gpio_put(RED_LED_GPIO_PIN, true);

  // inicializa botão A
  gpio_init(A_BUTTON_GPIO_PIN);
  gpio_set_dir(A_BUTTON_GPIO_PIN, GPIO_IN);
  gpio_pull_up(A_BUTTON_GPIO_PIN);

  // inicializa botão B
  gpio_init(B_BUTTON_GPIO_PIN);
  gpio_set_dir(B_BUTTON_GPIO_PIN, GPIO_IN);
  gpio_pull_up(B_BUTTON_GPIO_PIN);

  // habilita interrupções no toque dos botões A e B
  gpio_set_irq_enabled_with_callback(A_BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);
  gpio_set_irq_enabled_with_callback(B_BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);
  
  while (true)
  {
    uint32_t current_time = running ? to_ms_since_boot(get_absolute_time()) - start_time : elapsed_time;
    uint32_t milliseconds = current_time % 1000;
    uint32_t seconds = (current_time / 1000) % 60;
    uint32_t minutes = (current_time / 60000) % 60;

    sprintf(time_str, "%02d:%02d.%03d", minutes, seconds, milliseconds);

    // limpa o display antes de cada iteração
    ssd1306_fill(&ssd, false);

    // desenha uma string
    ssd1306_draw_string(&ssd, time_str, 28, 28);

    // atualiza o display
    ssd1306_send_data(&ssd);

    sleep_us(500);
  }
}

void button_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // implementa debounce nos botões
    if (current_time - last_time > DEBOUNCING_TIME_MS)
    {
        last_time = current_time;

        if (gpio == A_BUTTON_GPIO_PIN) {
            gpio_put(RED_LED_GPIO_PIN, !gpio_get(RED_LED_GPIO_PIN));
            gpio_put(BLUE_LED_GPIO_PIN, !gpio_get(BLUE_LED_GPIO_PIN));

            running = !running;
            if (running) {
                start_time = to_ms_since_boot(get_absolute_time()) - elapsed_time;
                gpio_put(RED_LED_GPIO_PIN, 0);
                gpio_put(BLUE_LED_GPIO_PIN, 0);
                gpio_put(GREEN_LED_GPIO_PIN, 1);
            } else {
                elapsed_time = to_ms_since_boot(get_absolute_time()) - start_time;
                gpio_put(RED_LED_GPIO_PIN, 1);
                gpio_put(BLUE_LED_GPIO_PIN, 0);
                gpio_put(GREEN_LED_GPIO_PIN, 0);
            }
        } else if (gpio == B_BUTTON_GPIO_PIN) {
            running = false;
            elapsed_time = 0;
            gpio_put(RED_LED_GPIO_PIN, 0);
            gpio_put(BLUE_LED_GPIO_PIN, 1);
            gpio_put(GREEN_LED_GPIO_PIN, 0);
        }
    }
}

void display_setup(ssd1306_t *ssd) {
  // inicializa a interface I2C com frequência de 400Khz
  i2c_init(I2C_PORT, 400 * 1000);

  // configura os pinos GPIO que serão os SDA e SCL da interface
  gpio_set_function(I2C_SDA_GPIO_PIN, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL_GPIO_PIN, GPIO_FUNC_I2C);

  // adiciona resistores pull-up internos nos pinos SDA e SCL
  gpio_pull_up(I2C_SDA_GPIO_PIN);
  gpio_pull_up(I2C_SCL_GPIO_PIN);

  // inicializa o hardware do display
  ssd1306_init(ssd, WIDTH, HEIGHT, false, I2C_ADDRESS, I2C_PORT);

  // configura o display para receber os dados
  ssd1306_config(ssd);

  // envia os dados da estrutura para o display
  ssd1306_send_data(ssd);

  // limpa o display ao iniciar
  ssd1306_fill(ssd, false);
  ssd1306_send_data(ssd);
}

void display_write_message(ssd1306_t *ssd, char *msg) {
  ssd1306_fill(ssd, false);
  ssd1306_draw_string(ssd, msg, 0, 0);
  ssd1306_send_data(ssd);
}