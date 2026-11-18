################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/uart/input_filter.c \
../src/uart/rx_ring_buffer.c \
../src/uart/uart_driver.c 

OBJS += \
./src/uart/input_filter.o \
./src/uart/rx_ring_buffer.o \
./src/uart/uart_driver.o 

C_DEPS += \
./src/uart/input_filter.d \
./src/uart/rx_ring_buffer.d \
./src/uart/uart_driver.d 


# Each subdirectory must supply rules for building sources it contributes
src/uart/%.o: ../src/uart/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/uart/input_filter.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


