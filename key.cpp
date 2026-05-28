#include "key.h"

void key_init(void)
{
    pinMode(KEY_INT_PIN, INPUT_PULLUP);
}