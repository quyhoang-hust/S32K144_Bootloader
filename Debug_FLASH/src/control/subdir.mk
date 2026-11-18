################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/control/boot_state_machine.c \
../src/control/error_handler.c 

OBJS += \
./src/control/boot_state_machine.o \
./src/control/error_handler.o 

C_DEPS += \
./src/control/boot_state_machine.d \
./src/control/error_handler.d 


# Each subdirectory must supply rules for building sources it contributes
src/control/%.o: ../src/control/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/control/boot_state_machine.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


