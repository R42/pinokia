#ifndef __GPIO_H
#define __GPIO_H

int gpio_setup();
void gpio_shutdown();

#define inl(f) inline f __attribute__((always_inline))

inl(uint32_t gpio_word());
inl(uint32_t gpio_set_input(uint32_t pin));
inl(uint32_t gpio_set_output(uint32_t pin));
inl(uint32_t gpio_alternate_function(uint32_t pin, uint32_t alternate));
inl(uint32_t gpio_set(uint32_t pins));
inl(uint32_t gpio_clear(uint32_t pins));

#endif
