################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/srec/line_builder.c \
../src/srec/srec_parser.c 

OBJS += \
./src/srec/line_builder.o \
./src/srec/srec_parser.o 

C_DEPS += \
./src/srec/line_builder.d \
./src/srec/srec_parser.d 


# Each subdirectory must supply rules for building sources it contributes
src/srec/%.o: ../src/srec/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/srec/line_builder.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


