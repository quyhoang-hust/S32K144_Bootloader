################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/flash/flash_driver.c \
../src/flash/flash_engine.c \
../src/flash/flash_job_queue.c 

OBJS += \
./src/flash/flash_driver.o \
./src/flash/flash_engine.o \
./src/flash/flash_job_queue.o 

C_DEPS += \
./src/flash/flash_driver.d \
./src/flash/flash_engine.d \
./src/flash/flash_job_queue.d 


# Each subdirectory must supply rules for building sources it contributes
src/flash/%.o: ../src/flash/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/flash/flash_driver.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


