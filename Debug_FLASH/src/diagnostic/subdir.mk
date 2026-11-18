################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/diagnostic/boot_statistic.c 

OBJS += \
./src/diagnostic/boot_statistic.o 

C_DEPS += \
./src/diagnostic/boot_statistic.d 


# Each subdirectory must supply rules for building sources it contributes
src/diagnostic/%.o: ../src/diagnostic/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/diagnostic/boot_statistic.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


