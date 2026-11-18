################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/bootstrap/app_validator.c \
../src/bootstrap/boot_button.c \
../src/bootstrap/jump_manager.c 

OBJS += \
./src/bootstrap/app_validator.o \
./src/bootstrap/boot_button.o \
./src/bootstrap/jump_manager.o 

C_DEPS += \
./src/bootstrap/app_validator.d \
./src/bootstrap/boot_button.d \
./src/bootstrap/jump_manager.d 


# Each subdirectory must supply rules for building sources it contributes
src/bootstrap/%.o: ../src/bootstrap/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/bootstrap/app_validator.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


