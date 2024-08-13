#include <time.h>

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
