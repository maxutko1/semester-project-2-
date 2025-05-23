#ifndef USART_H_INCLUDED
#define USART_H_INCLUDED

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);

void uart_init(void);
void io_redirect(void);

// New low-level UART functions
uint8_t uart_data_available(void);   // Non-blocking check
uint8_t uart_getchar_raw(void);      // Non-blocking raw read


#endif

