/* Bool.h
 *
 * Tipo de dados auxiliar para fazer verificações de
 * carácter booleano.
 *
 * Foi criado porque o stdbool.h faz parte do ISO C99,
 * que procede o que estou a utilizar, e também devido
 * aos meus planos prévios de escrever ficheiros bit a bit,
 */
#ifndef BOOL_H
#define BOOL_H

#define true 1
#define false 0
#include <stdint.h>
typedef uint8_t bool_t;

#endif
